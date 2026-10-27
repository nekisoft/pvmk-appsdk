//disasm.cpp
//ARM instruction decoder for the simulator's debugger
//Bryan E. Topp <betopp@betopp.com> 2025

//Nemul, the Neki32 Simulator, Copyright 2025 Nekisoft Pty Ltd, ACN 680 583 251
//This program is free software: you can redistribute it and/or modify
//it under the terms of the GNU General Public License as published by
//the Free Software Foundation, either version 3 of the License, or
//(at your option) any later version.

//This decodes the instruction set the Neki32's ARMv5TE actually has, in ARM state, and spells it
//the way GNU objdump spells it. The spelling matters: it is what people read in listings, and it
//means this decoder can be checked against the objdump the SDK already ships instruction by
//instruction instead of by eye.
//
//The one place it deliberately differs: 0xE7F009F2 is the Neki32's system call. The architecture
//calls that encoding undefined and objdump prints "udf", but in this machine it is a trap, so it
//is printed as the trap it is.

#include "disasm.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

static const char *const CondNames[16] =
{
	"eq", "ne", "cs", "cc", "mi", "pl", "vs", "vc",
	"hi", "ls", "ge", "lt", "gt", "le", "", "nv"
};

static const char *const RegNames[16] =
{
	"r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7",
	"r8", "r9", "sl", "fp", "ip", "sp", "lr", "pc"
};

static const char *const ShiftNames[4] = { "lsl", "lsr", "asr", "ror" };

const char *disasm_regname(int reg)
{
	if(reg < 0 || reg > 15) return "?";
	return RegNames[reg];
}

//Small append-only text builder, so that no decoder has to work out its own buffer sizes
typedef struct txt_s
{
	char *buf;
	int len;
	int cap;
} txt_t;

static void put(txt_t *t, const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	int space = t->cap - t->len;
	if(space > 1)
	{
		int wrote = vsnprintf(t->buf + t->len, space, fmt, args);
		if(wrote > 0)
		{
			t->len += wrote;
			if(t->len >= t->cap) t->len = t->cap - 1;
		}
	}
	va_end(args);
}

//Writes the ", lsl #4" tail of an operand shifted by an immediate, or nothing when unshifted
static void put_shift_imm(txt_t *t, int type, int amount)
{
	if(type == 0 && amount == 0)
		return;               //lsl #0 is no shift at all
	if(type == 3 && amount == 0)
	{
		put(t, ", rrx");      //rotate right by one through the carry
		return;
	}
	if(amount == 0)
	{
		//A zero amount means thirty-two for the shifts that empty the register
		amount = 32;
	}
	put(t, ", %s #%d", ShiftNames[type], amount);
}

//Writes a rotated immediate. binutils prints the plain value when the encoding in the instruction
//is the simplest one that produces it, and otherwise prints the piece and the rotation, which is
//what an assembler would need to reproduce the encoding.
static void put_imm(txt_t *t, uint32_t ir)
{
	const uint32_t imm8 = ir & 0xFF;
	const int rot = (ir >> 8) & 0xF;
	const uint32_t value = rot ? ((imm8 >> (rot * 2)) | (imm8 << (32 - rot * 2))) : imm8;

	int simplest = 0;
	for(int r = 0; r < 16; r++)
	{
		const uint32_t piece = (r == 0) ? value : ((value << (r * 2)) | (value >> (32 - r * 2)));
		if(piece <= 0xFF)
		{
			simplest = r;
			break;
		}
	}

	if(simplest == rot)
		put(t, "#%d", (int)value);
	else
		put(t, "#%u, %d", (unsigned)imm8, rot * 2);
}

//Writes one offset, which differs between the word transfers and the halfword ones
//(shift is 0 for no shift, 1 for a shift by an immediate, 2 for a shift by a register)
static void put_offset(txt_t *t, uint32_t ir, int up, int split_imm, int by_reg, int shift)
{
	if(!by_reg)
	{
		const uint32_t offset = split_imm ? (((ir >> 4) & 0xF0) | (ir & 0xF)) : (ir & 0xFFF);
		put(t, "#%s%u", up ? "" : "-", (unsigned)offset);
		return;
	}

	put(t, "%s%s", up ? "" : "-", RegNames[ir & 0xF]);
	if(shift == 1)
		put_shift_imm(t, (ir >> 5) & 3, (ir >> 7) & 0x1F);
	else if(shift == 2)
		put(t, ", %s %s", ShiftNames[(ir >> 5) & 3], RegNames[(ir >> 8) & 0xF]);
}

//Writes the operand a load or store applies to: the address and offset in all their forms
static void put_address(txt_t *t, uint32_t ir, int rn, int shift, int split_imm)
{
	const int pre = (ir >> 24) & 1;
	const int up = (ir >> 23) & 1;
	const int writeback = (ir >> 21) & 1;
	const int by_reg = (ir >> 25) & 1;

	//Pre-indexed puts the offset inside the bracket; post-indexed puts it after, and always
	//writes back whether or not the writeback bit is set
	put(t, "[%s", RegNames[rn]);

	if(pre)
	{
		if(!by_reg)
		{
			//A writeback with no offset still says so, so that it reads as the write it is
			const uint32_t offset = split_imm ? (((ir >> 4) & 0xF0) | (ir & 0xF)) : (ir & 0xFFF);
			if(offset != 0 || !up || writeback)
				put(t, ", #%s%u", up ? "" : "-", (unsigned)offset);
		}
		else
		{
			put(t, ", ");
			put_offset(t, ir, up, split_imm, by_reg, shift);
		}

		put(t, "]");
		if(writeback)
			put(t, "!");
		return;
	}

	put(t, "]");
	put(t, ", ");
	put_offset(t, ir, up, split_imm, by_reg, shift);
}

//Data processing: and/eor/sub/rsb/add/adc/sbc/rsc/tst/teq/cmp/cmn/orr/mov/bic/mvn
static int do_data_processing(txt_t *t, uint32_t ir)
{
	static const char *const names[16] =
	{
		"and", "eor", "sub", "rsb", "add", "adc", "sbc", "rsc",
		"tst", "teq", "cmp", "cmn", "orr", "mov", "bic", "mvn"
	};
	//TST, TEQ, CMP and CMN set the flags and have no destination register
	static const int implicit_s[16] = { 0,0,0,0, 0,0,0,0, 1,1,1,1, 0,0,0,0 };

	const int cond = (ir >> 28) & 0xF;
	const int imm = (ir >> 25) & 1;
	const int op = (ir >> 21) & 0xF;
	const int s = (ir >> 20) & 1;
	const int rn = (ir >> 16) & 0xF;
	const int rd = (ir >> 12) & 0xF;

	//TST/TEQ/CMP/CMN always set the flags, so bit 20 is not a suffix there. With bit 20 clear
	//and the PC as the first operand, binutils calls these the pointer-checking forms.
	const int ptr_check = implicit_s[op] && !s && rn == 15;

	//A move that shifts is written as the shift it does, and a move of r0 to r0 is the no-op it is
	if(op == 13 && !imm)
	{
		const int rm = ir & 0xF;
		const int shtype = (ir >> 5) & 3;
		const int shutdown = (ir >> 7) & 0x1F;
		const int by_reg = (ir >> 4) & 1;
		const int shifts = by_reg ? 1 : ((shtype == 0 && shutdown == 0) ? 0 : 1);

		if(!shifts && rd == 0 && rm == 0 && !s && cond == 14)
		{
			put(t, "nop");
			return 0;
		}

		if(shifts)
		{
			if(!by_reg && shtype == 3 && shutdown == 0)
			{
				//Rotate right through the carry has no amount to give
				put(t, "rrx%s%s	%s, %s", s ? "s" : "", CondNames[cond], RegNames[rd], RegNames[rm]);
				return 0;
			}

			put(t, "%s%s%s	%s, %s, ", ShiftNames[shtype], s ? "s" : "", CondNames[cond],
				RegNames[rd], RegNames[rm]);
			if(by_reg)
				put(t, "%s", RegNames[(ir >> 8) & 0xF]);
			else
				put(t, "#%d", shutdown ? shutdown : 32);
			return 0;
		}
	}

	put(t, "%s%s%s%s", names[op], (s && !implicit_s[op]) ? "s" : "", CondNames[cond],
		ptr_check ? "p" : "");
	put(t, "\t");

	if(!implicit_s[op])
	{
		put(t, "%s, ", RegNames[rd]);

		//MOV and MVN take no first operand
		if(op == 13 || op == 15)
		{
			if(imm)
			{
				put_imm(t, ir);
			}
			else
			{
				const int rm = ir & 0xF;
				put(t, "%s", RegNames[rm]);
				if((ir >> 4) & 1)
					put(t, ", %s %s", ShiftNames[(ir >> 5) & 3], RegNames[(ir >> 8) & 0xF]);
				else
					put_shift_imm(t, (ir >> 5) & 3, (ir >> 7) & 0x1F);
			}
			return 0;
		}
	}

	put(t, "%s, ", RegNames[rn]);

	if(imm)
	{
		put_imm(t, ir);
	}
	else
	{
		const int rm = ir & 0xF;
		const int type = (ir >> 5) & 3;
		put(t, "%s", RegNames[rm]);
		if((ir >> 4) & 1)
			put(t, ", %s %s", ShiftNames[type], RegNames[(ir >> 8) & 0xF]);
		else
			put_shift_imm(t, type, (ir >> 7) & 0x1F);
	}

	return 0;
}

//LDR/STR/LDRB/STRB, immediate and register offsets, pre and post indexed
static int do_single_transfer(txt_t *t, uint32_t ir)
{
	const int cond = (ir >> 28) & 0xF;
	const int load = (ir >> 20) & 1;
	const int byte = (ir >> 22) & 1;
	const int rn = (ir >> 16) & 0xF;
	const int rd = (ir >> 12) & 0xF;

	//Post-indexed with writeback is the form that runs in the user's registers rather than the
	//processor's, which objdump spells with a trailing t
	const int user_form = !((ir >> 24) & 1) && ((ir >> 21) & 1) && ((ir >> 28) & 0xF) != 0xF;

	put(t, "%s%s%s%s	%s, ", load ? "ldr" : "str", byte ? "b" : "", user_form ? "t" : "",
		CondNames[cond], RegNames[rd]);
	put_address(t, ir, rn, ((ir >> 4) & 1) ? 2 : 1, 0);
	return 0;
}

//Halfword and signed-byte transfers: LDRH/STRH/LDRSB/LDRSH
static int do_halfword(txt_t *t, uint32_t ir)
{
	const int cond = (ir >> 28) & 0xF;
	const int imm = (ir >> 22) & 1;
	const int load = (ir >> 20) & 1;
	const int rn = (ir >> 16) & 0xF;
	const int rd = (ir >> 12) & 0xF;
	const int sh = (ir >> 5) & 3;
	const char *name = NULL;

	//The two bits between the halves of the offset are the size and the sign
	switch(sh)
	{
		case 1: name = load ? "ldrh" : "strh"; break;
		case 2: if(!load) return -1; name = "ldrsb"; break;
		case 3: if(!load) return -1; name = "ldrsh"; break;
		default: return -1;
	}

	put(t, "%s%s\t%s, ", name, CondNames[cond], RegNames[rd]);
	if(!imm && (ir & 0xF) == 15) return -1;
	put_address(t, ir, rn, 0, 1);
	return 0;
}

//LDRD/STRD: two words in one go, ARMv5TE
static int do_dual(txt_t *t, uint32_t ir)
{
	const int cond = (ir >> 28) & 0xF;
	const int load = (ir >> 20) & 1;
	const int rn = (ir >> 16) & 0xF;
	const int rd = (ir >> 12) & 0xF;

	//The encoding has bit 22 set and both bits 6 and 5 clear
	if(!((ir >> 22) & 1)) return -1;
	if((ir >> 6) & 1) return -1;

	put(t, "%s%s\t%s, %s, ", load ? "ldrd" : "strd", CondNames[cond],
		RegNames[rd], RegNames[(rd + 1) & 0xF]);

	put_address(t, ir, rn, 0, 1);
	return 0;
}

//Multiply and multiply-accumulate, both widths
static int do_multiply(txt_t *t, uint32_t ir)
{
	const int cond = (ir >> 28) & 0xF;
	const int s = (ir >> 20) & 1;
	const int rd = (ir >> 16) & 0xF;
	const int rn = (ir >> 12) & 0xF;
	const int rs = (ir >> 8) & 0xF;
	const int rm = ir & 0xF;

	if((ir & 0x0FC000F0) == 0x00000090)
	{
		const int accumulate = (ir >> 21) & 1;
		put(t, "%s%s%s	%s, %s, %s", accumulate ? "mla" : "mul", s ? "s" : "", CondNames[cond],
			RegNames[rd], RegNames[rm], RegNames[rs]);
		if(accumulate)
			put(t, ", %s", RegNames[rn]);
		return 0;
	}

	if((ir & 0x0F8000F0) == 0x00800090)
	{
		//Bit 22 is the signedness of a long multiply: set means signed, clear means unsigned
		const int uns = !((ir >> 22) & 1);
		const int accumulate = (ir >> 21) & 1;
		const char *name;
		if(uns) name = accumulate ? "umlal" : "umull";
		else name = accumulate ? "smlal" : "smull";
		//The low half of the result is the first operand, as it is written in assembly
		put(t, "%s%s%s	%s, %s, %s, %s", name, s ? "s" : "", CondNames[cond],
			RegNames[rn], RegNames[rd], RegNames[rm], RegNames[rs]);
		return 0;
	}

	return -1;
}

//Swap: SWP and SWPB
static int do_swap(txt_t *t, uint32_t ir)
{
	const int cond = (ir >> 28) & 0xF;
	const int byte = (ir >> 22) & 1;
	const int rn = (ir >> 16) & 0xF;
	const int rd = (ir >> 12) & 0xF;
	const int rm = ir & 0xF;
	if((ir & 0xFF0) != 0x090) return -1;
	put(t, "swp%s%s	%s, %s, [%s]", CondNames[cond], byte ? "b" : "",
		RegNames[rd], RegNames[rm], RegNames[rn]);
	return 0;
}

//The DSP extended multiplies: SMLAxy, SMLAWy, SMULWy, SMLALxy, SMULxy
static int do_media_multiply(txt_t *t, uint32_t ir)
{
	const int cond = (ir >> 28) & 0xF;
	const int rd = (ir >> 16) & 0xF;
	const int rn = (ir >> 12) & 0xF;
	const int rs = (ir >> 8) & 0xF;
	const int rm = ir & 0xF;
	const int shape = (ir >> 21) & 3;
	const char x = ((ir >> 5) & 1) ? 't' : 'b';
	const char y = ((ir >> 6) & 1) ? 't' : 'b';

	switch(shape)
	{
		case 0:	//SMLAxy
			put(t, "smla%c%c%s\t%s, %s, %s, %s", x, y, CondNames[cond],
				RegNames[rd], RegNames[rm], RegNames[rs], RegNames[rn]);
			return 0;

		case 1:	//SMLAWy, or SMULWy when there is no accumulator
			if(rd == 0)
				put(t, "smulw%c%s\t%s, %s, %s", y, CondNames[cond],
					RegNames[rd], RegNames[rm], RegNames[rs]);
			else
				put(t, "smlaw%c%s\t%s, %s, %s, %s", y, CondNames[cond],
					RegNames[rd], RegNames[rm], RegNames[rs], RegNames[rn]);
			return 0;

		case 2:	//SMLALxy
			put(t, "smlal%c%c%s\t%s, %s, %s, %s", x, y, CondNames[cond],
				RegNames[rn], RegNames[rd], RegNames[rm], RegNames[rs]);
			return 0;

		default:	//SMULxy
			put(t, "smul%c%c%s\t%s, %s, %s", x, y, CondNames[cond],
				RegNames[rd], RegNames[rm], RegNames[rs]);
			return 0;
	}
}

//The saturating arithmetic: QADD, QSUB, QDADD, QDSUB
static int do_saturating(txt_t *t, uint32_t ir)
{
	const int cond = (ir >> 28) & 0xF;
	const int doubl = (ir >> 21) & 1;
	const int sub = (ir >> 20) & 1;
	const int rn = (ir >> 16) & 0xF;
	const int rd = (ir >> 12) & 0xF;
	const int rm = ir & 0xF;
	const char *name;
	if(sub) name = doubl ? "qdsub" : "qsub";
	else name = doubl ? "qdadd" : "qadd";
	put(t, "%s%s\t%s, %s, %s", name, CondNames[cond], RegNames[rd], RegNames[rm], RegNames[rn]);
	return 0;
}

//Block transfer: LDM/STM in all four addressing modes, with the push and pop aliases
static int do_block_transfer(txt_t *t, uint32_t ir)
{
	const int cond = (ir >> 28) & 0xF;
	const int pre = (ir >> 24) & 1;
	const int up = (ir >> 23) & 1;
	const int s = (ir >> 22) & 1;
	const int writeback = (ir >> 21) & 1;
	const int load = (ir >> 20) & 1;
	const int rn = (ir >> 16) & 0xF;
	const uint16_t list = ir & 0xFFFF;
	const char *name;

	if(list == 0) return -1;

	if(pre && !up) name = load ? "ldmdb" : "stmdb";
	else if(pre && up) name = load ? "ldmib" : "stmib";
	else if(!pre && up) name = load ? "ldm" : "stm";
	else name = load ? "ldmda" : "stmda";

	//The stack forms are what people write
	if(writeback && rn == 13 && !s)
	{
		if(!load && pre && !up) name = "push";
		if(load && !pre && up) name = "pop";
	}

	if(!strcmp(name, "push") || !strcmp(name, "pop"))
		put(t, "%s%s	{", name, CondNames[cond]);
	else
		put(t, "%s%s	%s%s, {", name, CondNames[cond], RegNames[rn], writeback ? "!" : "");
	int first = 1;
	for(int rr = 0; rr < 16; rr++)
	{
		if(list & (1 << rr))
		{
			if(!first) put(t, ", ");
			put(t, "%s", RegNames[rr]);
			first = 0;
		}
	}
	put(t, "}%s", s ? "^" : "");

	if(!writeback && (list & (1 << rn)))
	{
		//Writing the base in the list without writeback: the architecture calls it unpredictable,
		//but it assembles and runs, so show it rather than hide it
	}
	return 0;
}

//B and BL
static int do_branch(txt_t *t, uint32_t addr, uint32_t ir, uint32_t *flow)
{
	const int cond = (ir >> 28) & 0xF;
	const int link = (ir >> 24) & 1;
	const int32_t offset = ((int32_t)(ir << 8)) >> 6;
	const uint32_t target = addr + 8u + (uint32_t)offset;
	put(t, "%s%s	0x%x", link ? "bl" : "b", CondNames[cond], (unsigned)target);
	if(flow) *flow = target;
	return 0;
}

//Coprocessor loads and stores, register transfers and data processing
static int do_coprocessor(txt_t *t, uint32_t ir)
{
	const int cond = (ir >> 28) & 0xF;
	const int cp = (ir >> 8) & 0xF;

	if((ir & 0x0E000000) == 0x0C000000)
	{
		//LDC/STC: memory to and from a coprocessor
		const int load = (ir >> 20) & 1;
		const int longform = (ir >> 22) & 1;
		const int rn = (ir >> 16) & 0xF;
		const int crd = (ir >> 12) & 0xF;
		put(t, "%sc%s%s\tp%d, c%d, ", load ? "ld" : "st", longform ? "l" : "",
			CondNames[cond], cp, crd);
		put_address(t, ir, rn, 1, 0);
		return 0;
	}

	if((ir & 0x0E000000) == 0x0E000000)
	{
		const int op1 = (ir >> 21) & 7;
		const int crn = (ir >> 16) & 0xF;
		const int crd = (ir >> 12) & 0xF;
		const int op2 = (ir >> 5) & 7;
		const int to_cp = (ir >> 4) & 1;

		if((ir & 0x0FE00000) == 0x0C400000)
		{
			//Two registers at once: MCRR and MRRC
			const int load = (ir >> 20) & 1;
			put(t, "%s%s\tp%d, %d, %s, %s, c%d", load ? "mrrc" : "mcrr",
				CondNames[cond], cp, op1, RegNames[(ir >> 12) & 0xF],
				RegNames[(ir >> 16) & 0xF], (int)(ir & 0xF));
			return 0;
		}

		if(!to_cp)
		{
			//CDP: coprocessor to coprocessor
			put(t, "cdp%s\tp%d, %d, c%d, c%d, c%d, %d", CondNames[cond],
				cp, op1, crd, crn, (int)(ir & 0xF), op2);
			return 0;
		}
		else
		{
			//MCR and MRC: between an ARM register and a coprocessor
			const int load = (ir >> 20) & 1;
			put(t, "%sc%s\tp%d, %d, %s, c%d, c%d, %d", load ? "mr" : "mc",
				CondNames[cond], cp, op1, RegNames[crd], crn, (int)(ir & 0xF), op2);
			return 0;
		}
	}

	return -1;
}

//Everything in the unconditional part of the encoding space
static int do_unconditional(txt_t *t, uint32_t addr, uint32_t ir, uint32_t *flow)
{
	if((ir & 0x0E000000) == 0x0A000000)
	{
		//BLX with a 24-bit offset, half-word aligned
		const int32_t offset = ((int32_t)(ir << 8)) >> 6;
		const uint32_t target = addr + 8u + (uint32_t)offset + (((ir >> 24) & 1) << 1);
		put(t, "blx	0x%x", (unsigned)target);
		if(flow) *flow = target;
		return 0;
	}

	if((ir & 0x0FD0F000) == 0xF550F000)
	{
		//Preload hint
		const int rn = (ir >> 16) & 0xF;
		put(t, "pld\t[%s", RegNames[rn]);
		const uint32_t offset = ir & 0xFFF;
		if(offset) put(t, ", #%u", (unsigned)offset);
		put(t, "]");
		return 0;
	}

	return -1;
}

int disasm_arm(uint32_t addr, uint32_t ir, char *out, size_t outsz, uint32_t *flow)
{
	txt_t t;
	t.buf = out;
	t.len = 0;
	t.cap = (int)outsz;
	if(outsz) out[0] = 0;
	if(flow) *flow = (uint32_t)-1;

	const int cond = (ir >> 28) & 0xF;

	//The Neki32's system call, which the architecture calls an undefined instruction
	if(ir == 0xE7F009F2)
	{
		put(&t, "svc\t#0x92");
		return 0;
	}

	//The breakpoint instruction the debugger plants
	if(ir == 0xE7FFDEFE || (ir & 0x0FF000F0) == 0x01200070)
	{
		put(&t, "bkpt	0x%04x", (unsigned)(((ir >> 4) & 0xFFF0) | (ir & 0xF)));
		return 0;
	}

	if(cond == 0xF)
		return do_unconditional(&t, addr, ir, flow);

	//Branch, the simplest group and worth testing before anything else
	if((ir & 0x0E000000) == 0x0A000000)
		return do_branch(&t, addr, ir, flow);

	//Block transfer
	if((ir & 0x0E000000) == 0x08000000)
		return do_block_transfer(&t, ir);

	//Single data transfer: every load and store of a word or byte
	if((ir & 0x0C000000) == 0x04000000)
		return do_single_transfer(&t, ir);

	//Coprocessor and SWI
	if((ir & 0x0F000000) == 0x0F000000)
	{
		put(&t, "svc%s	0x%08x", CondNames[cond], (unsigned)(ir & 0xFFFFFF));
		return 0;
	}
	if((ir & 0x0C000000) == 0x0C000000)
		return do_coprocessor(&t, ir);

	if((ir & 0x0C000000) != 0x00000000)
		return -1;

	//From here down it is the miscellaneous side of the data processing space
	if((ir & 0x0FC000F0) == 0x00000090 || (ir & 0x0F8000F0) == 0x00800090)
		return do_multiply(&t, ir);

	if((ir & 0x0FB00FF0) == 0x01000090)
		return do_swap(&t, ir);

	if((ir & 0x0E000090) == 0x00000090)
	{
		//bit 7 and bit 4 set: multiplies, swaps, halfword transfers and the doubleword ones
		if((ir & 0x60) != 0)
			return do_halfword(&t, ir);
		if((ir & 0xF0) == 0xD0)
			return do_dual(&t, ir);
		return -1;
	}

	//The DSP multiplies sit where a data processing instruction would put its S bit, and always
	//have it clear
	if((ir & 0x0F900090) == 0x01000080)
		return do_media_multiply(&t, ir);

	//QADD and its friends, which share their shape with data processing and are told apart by
	//the zero in bits 11-8
	if((ir & 0x0F900FF0) == 0x01000050)
		return do_saturating(&t, ir);

	if((ir & 0x0FFFFFF0) == 0x012FFF10 || (ir & 0x0FFFFFF0) == 0x012FFF30)
	{
		//BX and BLX to a register, which is how the machine changes mode and calls through a pointer
		put(&t, "%s%s	%s", ((ir >> 5) & 1) ? "blx" : "bx", CondNames[cond], RegNames[ir & 0xF]);
		return 0;
	}

	if((ir & 0x0FFF0FF0) == 0x016F0F10)
	{
		//CLZ
		put(&t, "clz%s\t%s, %s", CondNames[cond], RegNames[(ir >> 12) & 0xF], RegNames[ir & 0xF]);
		return 0;
	}

	if((ir & 0x0FBF0FFF) == 0x010F0000)
	{
		//MRS
		put(&t, "mrs%s	%s, %s", CondNames[cond], RegNames[(ir >> 12) & 0xF],
			((ir >> 22) & 1) ? "SPSR" : "CPSR");
		return 0;
	}

	if((ir & 0x0FB0FFF0) == 0x0120F000 || (ir & 0x0FB0F000) == 0x0320F000)
	{
		//MSR, from a register or an immediate
		const int spsr = (ir >> 22) & 1;
		const int mask = (ir >> 16) & 0xF;
		char fields[5] = "";
		int at = 0;
		if(mask & 1) fields[at++] = 'c';
		if(mask & 2) fields[at++] = 'x';
		if(mask & 4) fields[at++] = 's';
		if(mask & 8) fields[at++] = 'f';
		if(at == 0) return -1;
		put(&t, "msr%s\t%s_%s, ", CondNames[cond], spsr ? "spsr" : "cpsr", fields);
		if((ir >> 25) & 1)
		{
			uint32_t val = ir & 0xFF;
			const int rot = (ir >> 8) & 0xF;
			if(rot) val = (val >> (rot * 2)) | (val << (32 - rot * 2));
			put(&t, "#%d", (int)val);
		}
		else
		{
			put(&t, "%s", RegNames[ir & 0xF]);
		}
		return 0;
	}

	//What is left of this space is data processing
	return do_data_processing(&t, ir);
}
