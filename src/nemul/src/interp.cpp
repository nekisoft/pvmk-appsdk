//interp.cpp
//ARM v5TE interpreter
//Bryan E. Topp <betopp@betopp.com> 2025

//Nemul, the Neki32 Simulator, Copyright 2025 Nekisoft Pty Ltd, ACN 680 583 251
//This program is free software: you can redistribute it and/or modify
//it under the terms of the GNU General Public License as published by
//the Free Software Foundation, either version 3 of the License, or
//(at your option) any later version.

#define FILE_TRACE_CAT TRACE_CAT_INTERP
#include "trace.h"

#include "interp.h"

//ARM flag register contents
#define FLAG_V (1u << 28)
#define FLAG_C (1u << 29)
#define FLAG_Z (1u << 30)
#define FLAG_N (1u << 31)
#define FLAG_Q (1u << 27)
#define FLAG_GE(n) (1u << (16 + n))

static interp_result_t interp_store_d(uint32_t *mem, uint32_t memsz, uint32_t addr, uint64_t data)
{
	if(addr & 7)
	{
		TWARNING("Misaligned doubleword store of %16.16lX to addr %8.8X\n", data, addr);
		return INTERP_RESULT_AC;
	}
	
	if(addr + 8 > memsz || addr < 0x1000)
	{
		TWARNING("Out-of-bounds doubleword store to %8.8X\n", addr);
		return INTERP_RESULT_ABT;
	}
	
	mem[ (addr/4) + 0 ] = data >>  0;
	mem[ (addr/4) + 1 ] = data >> 32;
	return INTERP_RESULT_OK;
}

static interp_result_t interp_store_w(uint32_t *mem, uint32_t memsz, uint32_t addr, uint32_t data)
{
	if(addr & 3)
	{
		TWARNING("Misaligned word store of %8.8X to addr %8.8X\n", data, addr);
		return INTERP_RESULT_AC;
	}
	
	if(addr + 4 > memsz || addr < 0x1000)
	{
		TWARNING("Out-of-bounds word store to %8.8X\n", addr);
		return INTERP_RESULT_ABT;
	}
	
	mem[addr/4] = data;
	return INTERP_RESULT_OK;
}

static interp_result_t interp_store_h(uint32_t *mem, uint32_t memsz, uint32_t addr, uint16_t data)
{
	if(addr % 1)
	{
		TWARNING("Misaligned halfword store of %4.4X to addr %8.8X\n", data, addr);
		return INTERP_RESULT_AC;
	}
	
	if(addr + 2 > memsz || addr < 0x1000)
	{
		TWARNING("Out-of-bounds halfword store to %8.8X\n", addr);
		return INTERP_RESULT_ABT;
	}
	
	switch(addr % 4)
	{
		case 0:
		case 1:
			mem[addr/4] &= 0xFFFF0000u;
			mem[addr/4] |= (uint32_t)data <<  0;		
		break;
		case 2:
		case 3:
			mem[addr/4] &= 0x0000FFFFu;
			mem[addr/4] |= (uint32_t)data << 16;		
		break;
	}
	
	return INTERP_RESULT_OK;
}

static interp_result_t interp_store_b(uint32_t *mem, uint32_t memsz, uint32_t addr, uint8_t data)
{
	if(addr + 1 > memsz || addr < 0x1000)
	{
		TWARNING("Out-of-bounds byte store to %8.8X\n", addr);
		return INTERP_RESULT_ABT;
	}
	
	switch(addr % 4)
	{
		case 0:
			mem[addr/4] &= 0xFFFFFF00u;
			mem[addr/4] |= (uint32_t)data <<  0;		
		break;
		case 1:
			mem[addr/4] &= 0xFFFF00FFu;
			mem[addr/4] |= (uint32_t)data <<  8;		
		break;
		case 2:
			mem[addr/4] &= 0xFF00FFFFu;
			mem[addr/4] |= (uint32_t)data << 16;		
		break;
		case 3:
			mem[addr/4] &= 0x00FFFFFFu;
			mem[addr/4] |= (uint32_t)data << 24;		
		break;
	}
	
	return INTERP_RESULT_OK;
}

static interp_result_t interp_load_d(const uint32_t *mem, uint32_t memsz, uint32_t addr, uint64_t *data)
{
	if(addr & 7)
	{
		TWARNING("Misaligned doubleword load from addr %8.8X\n", addr);
		return INTERP_RESULT_AC;
	}
	
	if(addr + 8 > memsz || addr < 0x1000)
	{
		TWARNING("Out-of-bounds doubleword load from %8.8X\n", addr);
		return INTERP_RESULT_ABT;
	}
	
	*data = mem[ (addr/4) + 0 ];
	*data |= ((uint64_t)mem[ (addr/4) + 1 ]) << 32;
	return INTERP_RESULT_OK;
}

static interp_result_t interp_load_w(const uint32_t *mem, uint32_t memsz, uint32_t addr, uint32_t *data)
{
	if(addr & 3)
	{
		TWARNING("Misaligned word load from addr %8.8X\n", addr);
		return INTERP_RESULT_AC;
	}

	if(addr + 4 > memsz || addr < 0x1000)
	{
		TWARNING("Out-of-bounds word load from %8.8X\n", addr);
		return INTERP_RESULT_ABT;
	}	
	
	*data = mem[addr/4];
	return INTERP_RESULT_OK;
}

static interp_result_t interp_load_h(const uint32_t *mem, uint32_t memsz, uint32_t addr, uint32_t *data)
{
	if(addr & 1)
	{
		TWARNING("Misaligned halfword load from addr %8.8X\n", addr);
		return INTERP_RESULT_AC;
	}	
	
	if(addr + 2 > memsz || addr < 0x1000)
	{
		TWARNING("Out-of-bounds halfword load from %8.8X\n", addr);
		return INTERP_RESULT_ABT;
	}	
	
	switch(addr % 4)
	{
		case 0:
		case 1:
			*data = (mem[addr/4] >> 0) & 0xFFFF; return INTERP_RESULT_OK;
		case 2:
		case 3:
			*data = (mem[addr/4] >> 16) & 0xFFFF; return INTERP_RESULT_OK;
		default: return INTERP_RESULT_FATAL; //Shouldn't happen
	}
}

static interp_result_t interp_load_b(const uint32_t *mem, uint32_t memsz, uint32_t addr, uint32_t *data)
{
	if(addr + 1 > memsz || addr < 0x1000)
	{
		TWARNING("Out-of-bounds byte load from %8.8X\n", addr);
		return INTERP_RESULT_ABT;
	}	
	
	switch(addr % 4)
	{
		case 0: *data = (mem[addr/4] >>  0) & 0xFF; return INTERP_RESULT_OK; 
		case 1: *data = (mem[addr/4] >>  8) & 0xFF; return INTERP_RESULT_OK;
		case 2: *data = (mem[addr/4] >> 16) & 0xFF; return INTERP_RESULT_OK;
		case 3: *data = (mem[addr/4] >> 24) & 0xFF; return INTERP_RESULT_OK;
		default: return INTERP_RESULT_FATAL; //Shouldn't happen
	}
}

static void interp_dataproc(int opcode, uint32_t *dest, uint32_t *cpsr, uint32_t reg_operand, uint32_t shifter_operand, bool writeflags, bool shifter_carry)
{
	TDEBUG("Data op %X: ", opcode);
	bool carryflag = (*cpsr) & FLAG_C;
	uint64_t result = 0;
	
	uint64_t reg64 = reg_operand;
	uint64_t shifter64 = shifter_operand;
	
	int64_t reg_s = (int32_t)reg_operand;
	int64_t shifter_s = (int32_t)shifter_operand;
	int64_t result_s = 0;
	
	switch(opcode)
	{
		case  0: 
			result = reg64 & shifter64; 
			TDEBUG("%8.8X & %8.8X = %8.8X\n", reg_operand, shifter_operand, (uint32_t)result);
		break;
		case  1: 
			result = reg64 ^ shifter64;
			TDEBUG("%8.8X ^ %8.8X = %8.8X\n", reg_operand, shifter_operand, (uint32_t)result);
		break;
		case  2:
			result = reg64 - shifter64;
			result_s = reg_s - shifter_s;
			TDEBUG("%8.8X - %8.8X = %8.8X\n", reg_operand, shifter_operand, (uint32_t)result);
		break;
		case  3:
			result = shifter64 - reg64;
			result_s = shifter_s - reg_s;
			TDEBUG("%8.8X - %8.8X = %8.8X\n", shifter_operand, reg_operand, (uint32_t)result);
		break;
		case  4:
			result = reg64 + shifter64;
			result_s = reg_s + shifter_s;
			TDEBUG("%8.8X + %8.8X = %8.8X\n", reg_operand, shifter_operand, (uint32_t)result);		
		break;
		case  5:
			result = reg64 + shifter64 + (carryflag?1:0);
			result_s = reg_s + shifter_s + (carryflag?1:0);
			TDEBUG("%8.8X + %8.8X + %d = %8.8X\n",
				reg_operand, shifter_operand, (carryflag?1:0), (uint32_t)result);
		break;
		case  6:
			result = reg64 - shifter64 - (carryflag?0:1);
			result_s = reg_s - shifter_s - (carryflag?0:1);
			TDEBUG("%8.8X - %8.8X - %d = %8.8X\n",
				reg_operand, shifter_operand, (carryflag?0:1), (uint32_t)result);
		break;
		case  7:
			result = shifter64 - reg64 - (carryflag?0:1);
			result_s = shifter_s - reg_s - (carryflag?0:1);
			TDEBUG("%8.8X - %8.8X - %d = %8.8X\n",
				shifter_operand, reg_operand, (carryflag?0:1), (uint32_t)result);
		break;
		case  8:
			result = reg64 & shifter64;
			TDEBUG("%8.8X & %8.8X = %8.8X (compare)\n",
				reg_operand, shifter_operand, (uint32_t)result);
		break;
		case  9:
			result = reg64 ^ shifter64;
			TDEBUG("%8.8X ^ %8.8X = %8.8X (compare)\n",
				reg_operand, shifter_operand, (uint32_t)result);
		break;
		case 10:
			result = reg64 - shifter64;
			result_s = reg_s - shifter_s;
			TDEBUG("%8.8X - %8.8X = %8.8X (compare)\n",
				reg_operand, shifter_operand, (uint32_t)result);
		break;
		case 11:
			result = reg64 + shifter64;
			result_s = reg_s + shifter_s;
			TDEBUG("%8.8X + %8.8X = %8.8X (compare)\n",
				reg_operand, shifter_operand, (uint32_t)result);
		break;
		case 12:
			result = reg64 | shifter64;
			TDEBUG("%8.8X | %8.8X = %8.8X\n", reg_operand, shifter_operand, (uint32_t)result);
		break;
		case 13:
			result = shifter64;
			TDEBUG("%8.8X = %8.8X\n", shifter_operand, (uint32_t)result);
		break;
		case 14:
			result = reg64 & ~shifter64;
			TDEBUG("%8.8X & ~%8.8X = %8.8X\n", reg_operand, shifter_operand, (uint32_t)result);
		break;
		case 15:
			result = ~shifter64;
			TDEBUG("~%8.8X = %8.8X\n", shifter_operand, (uint32_t)result);
		break;
		default:
			result = 0x2BADBEEF;
			TERROR("Bad opcode %d in call to data processing\n", opcode);
	}
	
	if(writeflags)
	{
		TDEBUG("%s", "Flags set");
		
		//Which opcodes count as addition/subtraction for flag purposes
		static const bool addsubs[16] = {0,0,1,1,1,1,1,1,0,0,1,1,0,0,0,0};
		static const bool subs[16]    = {0,0,1,1,0,0,1,1,0,0,1,0,0,0,0,0};
		static const bool cshift[16]  = {1,1,0,0,0,0,0,0,1,1,0,0,1,1,1,1}; //operations storing shifter-carry
		
		if(result & 0xFFFFFFFFu)
		{
			TDEBUG(" %s", "Z0");
			*cpsr &= ~FLAG_Z;
		}
		else
		{
			TDEBUG(" %s", "Z1");
			*cpsr |= FLAG_Z;
		}
		
		if(result & 0x80000000u)
		{
			TDEBUG(" %s", "N1");
			*cpsr |= FLAG_N;
		}
		else
		{
			TDEBUG(" %s", "N0");
			*cpsr &= ~FLAG_N;
		}
		
		if(subs[opcode])
		{
			//Subtraction ops use inverted carry-flag...? set when there's no borrow
			if(!(result & 0xFFFFFFFF00000000u))
			{
				TDEBUG(" %s", "C1");
				*cpsr |= FLAG_C;
			}
			else
			{
				TDEBUG(" %s", "C0");
				*cpsr &= ~FLAG_C;
			}
		}
		else if(addsubs[opcode])
		{
			if((result & 0xFFFFFFFF00000000u))
			{
				TDEBUG(" %s", "C1");
				*cpsr |= FLAG_C;
			}
			else
			{
				TDEBUG(" %s", "C0");
				*cpsr &= ~FLAG_C;
			}
		}
		else if(cshift[opcode])
		{
			//Some ops like MOV set carry as the carry-out of the shifter
			if(shifter_carry)
			{
				TDEBUG(" %s", "C1");
				*cpsr |= FLAG_C;
			}
			else
			{
				TDEBUG(" %s", "C0");
				*cpsr &= ~FLAG_C;
			}
		}
		
		if(addsubs[opcode])
		{
			if(result_s < -2147483648ll || result_s >= 2147483648ll)
			{
				TDEBUG(" %s", "V1");
				*cpsr |= FLAG_V;
			}
			else
			{
				TDEBUG(" %s", "V0");				
				*cpsr &= ~FLAG_V;
			}
		}
		
		TDEBUG("%s", " ");
	}
	
	//Comparison ops don't write their result, others do
	if( (opcode & 0xC) != 0x8 )
	{
		TDEBUG("%s", "Result saved");
		*dest = result & 0xFFFFFFFFu;
	}
	
	TDEBUG("%s", "\n");
}

static interp_result_t interp_step_inner(uint32_t *regs, uint32_t *cpsr, uint32_t *mem, size_t memsz, uint32_t force_ir)
{
	//Validate program counter
	regs[15] -= 4; //We'll add 8 later, so sub 4 and normally move 4 at a time
	
	if(!force_ir)
	{
		if(regs[15] % 4)
		{
			//Misaligned program counter
			//(Todo - thumb mode)
			TERROR("Misaligned program counter %8.8X\n", regs[15]);
			return INTERP_RESULT_FATAL;
		}
		if(regs[15] < 0x1000 || regs[15] + 4 > memsz)
		{
			//Out of bounds program counter, simulate as prefetch abort
			TWARNING("Out-of-bounds program counter %8.8X, memsz=%8.8X\n", regs[15], (uint32_t)memsz);
			return INTERP_RESULT_PF;
		}
	}
	
	//Fetch next instruction
	const uint32_t ir = (force_ir) ? (force_ir) : (mem[regs[15] / 4]);
	if(force_ir)
		TWARNING("%s", "(IR FORCED) ");
	
	TDEBUG("=== INTERP STEP === PC %8.8X : IR %8.8X : ", regs[15], ir);
	
	regs[15] += 8; //For the rest of the CPU, r15 refers to the current instruction plus 8 bytes.
	
	//Decode instruction
	const int rm     = (ir >>  0) & 0xF;
	const int shift  = (ir >>  5) & 0x3;
	const int shamt  = (ir >>  7) & 0x1F;
	const int rd     = (ir >> 12) & 0xF;
	const int rn     = (ir >> 16) & 0xF;
	const int sdata  = (ir >> 20) & 0x1;
	const int opcode = (ir >> 21) & 0xF;
	const int rs     = (ir >>  8) & 0xF;
	const int imm8   = (ir >>  0) & 0xFF;
	const int rotate = (ir >>  8) & 0xF;
	const int sbo    = (ir >> 12) & 0xF;
	const int imm12  = (ir >>  0) & 0xFFF;
	const int mask   = (ir >> 16) & 0xF;
	const int l      = (ir >> 20) & 0x1;
	const int w      = (ir >> 21) & 0x1;
	const int b      = (ir >> 22) & 0x1;
	const int slsm   = (ir >> 22) & 0x1;
	const int p      = (ir >> 24) & 0x1;
	const int n      = (ir >> 22) & 0x1;
	const int u      = (ir >> 23) & 0x1;
	const int group  = (ir >> 25) & 0x7;
	const int cond   = (ir >> 28) & 0xF;
	const int swino  = (ir >>  0) & 0xFFFFFF;
	const int bl     = (ir >> 24) & 0x1;
	
	(void)rm; (void)shift; (void)shamt; (void)rd; (void)rn; (void)sdata;
	(void)opcode; (void)rs; (void)imm8; (void)rotate; (void)sbo;
	(void)imm12; (void)mask; (void)l; (void)w; (void)b; (void)slsm;
	(void)p; (void)n; (void)u; (void)swino;

	TDEBUG("cond:%X group:%X opcode:%X\n", cond, group, opcode);

	if(ir == 0xE7F009F2)
	{
		//UDF 0x92 is our system-call instruction
		TDEBUG("Caught syscall %8.8X\n", regs[0]);
		return INTERP_RESULT_SYSCALL;
	}
	
	if(ir == 0xe7ffdefe)
	{
		//This is the GDB breakpoint instruction
		TDEBUG("%s", "Caught GDB breakpoint instruction\n");
		return INTERP_RESULT_BKPT;
	}
	
	if(ir == 0xEAFFFFFE)
	{
		//This is an unconditional jump back to the current instruction
		TERROR("Caught one-cycle infinite loop instruction %8.8X\n", ir);
		return INTERP_RESULT_FATAL;
	}

	//Check condition field and see if instruction is skipped
	if(cond == 0xF)
	{
		TERROR("Unconditional unhandled %8.8X\n", ir);
		return INTERP_RESULT_FATAL;
	}
	
	bool condsatisfied = true;
	bool fz = *cpsr & FLAG_Z;
	bool fc = *cpsr & FLAG_C;
	bool fn = *cpsr & FLAG_N;
	bool fv = *cpsr & FLAG_V;
	switch(cond)
	{
		case  0: condsatisfied = fz; break;
		case  1: condsatisfied = !fz; break;
		case  2: condsatisfied = fc; break;
		case  3: condsatisfied = !fc; break;
		case  4: condsatisfied = fn; break;
		case  5: condsatisfied = !fn; break;
		case  6: condsatisfied = fv; break;
		case  7: condsatisfied = !fv; break;
		case  8: condsatisfied = fc && (!fz); break;
		case  9: condsatisfied = (!fc) || fz; break;
		case 10: condsatisfied = (fn && fv) || ((!fn) && (!fv)); break;
		case 11: condsatisfied = (fn && (!fv)) || ((!fn) && fv); break;
		case 12: condsatisfied = (!fz) && ( (fn && fv) || ((!fn)&&(!fv)) ); break;
		case 13: condsatisfied = fz || (fn && (!fv)) || ((!fn) && fv); break;
		case 14: condsatisfied = true; break;
		case 15: condsatisfied = true; break;
		default:
			TERROR("Bad condition code %d\n", cond);
			return INTERP_RESULT_FATAL;
	}
	if(!condsatisfied)
	{
		TDEBUG("%s", "Skipping as condition not met.\n");
		return INTERP_RESULT_OK;
	}
	
	if(group == 0x7)
	{
		if(p == 0)
		{
			//Coprocessor instruction
			TERROR("Coproc unhandled %8.8X\n", ir);
			return INTERP_RESULT_FATAL;
		}
		else
		{
			//Software interrupt
			TERROR("Unhandled SWI instruction %8.8X\n", ir);
			return INTERP_RESULT_FATAL;
		}
	}
	
	if(group == 0x6)
	{
		//Coprocessor load/store and double register transfers
		TERROR("Coproc l/s unhandled %8.8X\n", ir);
		return INTERP_RESULT_FATAL;
	}
	
	if(group == 0x5)
	{
		//Branch and branch with link
		uint32_t offset = ir & 0xFFFFFFu;
		if(offset & 0x800000u)
			offset |= 0xFF000000u;
		
		offset *= 4;
		offset += 4;
		
		if(bl)
		{
			regs[14] = regs[15] - 4;
			regs[15] += offset;
			TDEBUG("Branch with link +%8.8X to %8.8X, saved %8.8X in LR\n", offset, regs[15]-4, regs[14]);
		}
		else
		{
			regs[15] += offset;
			TDEBUG("Branch +%8.8X to %8.8X\n", offset, regs[15]-4);
		}
			
		return INTERP_RESULT_OK;
	}
	
	if(group == 0x4)
	{
		//Load/store multiple
		TDEBUG("Load/store multiple %8.8X\n", ir);
		
		//Make sure the S bit isn't set (that's a privileged op)
		if(slsm)
		{
			TERROR("%s", "Load/store op has S bit set, cannot run in usermode\n");
			return INTERP_RESULT_FATAL;
		}
		
		//Do loads/stores
		uint32_t addr = regs[rn];
		interp_result_t memresult = INTERP_RESULT_OK;
		for(int rcnt = 0; rcnt < 16; rcnt++)
		{
			//Register numbers go backwards if we're storing downwards
			int reg = u ? rcnt : (15 - rcnt);
			
			if(!(ir & (1u << reg)))
				continue; //Register not in the set
			
			//Address moves up/down according to U bit
			uint32_t nextaddr = u ? (addr + 4) : (addr - 4);
			
			//Do load or store according to L bit
			//Access might use the current- or next-address, according to P-bit
			uint32_t access_addr = p?nextaddr:addr;
			
			if(l)
				memresult = interp_load_w(mem, memsz, access_addr, &(regs[reg]));
			else
				memresult = interp_store_w(mem, memsz, access_addr, regs[reg]);
			
			TDEBUG("\tr%d %c @%8.8X (#%8.8X)\n", reg, l?'<':'>', access_addr, regs[reg]);
			
			if(l && reg == 15)
				regs[15] += 4; //Correct PC as we keep it offset
			
			//Keep going unless the memory address faults
			addr = nextaddr;
			if(memresult != INTERP_RESULT_OK)
				break;
			
		}
		
		//Write-back the address to the register after incrementing/decrementing, if set
		if(w)
			regs[rn] = addr;
		
		return memresult;
	}
	
	if(group == 0x3)
	{
		if( (ir & 0x07F000F0) == 0x07F000F0)
		{
			TERROR("Undefined instruction %8.8X\n", ir);
			return INTERP_RESULT_FATAL;
		}
		else if(ir & 0x10)
		{
			TERROR("Media instruction %8.8X\n", ir);
			return INTERP_RESULT_FATAL;
		}
		else
		{
			TDEBUG("load/store register offset %c r%d from r%d (%8.8X)\n",
				u?'+':'-', rm, rn, regs[rn]);
			
			
			int effective_shift = shamt;
			TDEBUG("Scaled offset - Shifting r%d (%8.8X) by %d ", rm, regs[rm], effective_shift);
			uint32_t srcdata = regs[rm];
			uint32_t shifted = 0;
			switch(shift)
			{
				case 0:
					TDEBUG("%s", "(LSL)");
					shifted |= srcdata << effective_shift;
					break;
				case 1:
					TDEBUG("%s", "(LSR)");
					shifted |= srcdata >> effective_shift;
					break;
				case 2:
					TDEBUG("%s", "(ASR)");
					shifted |= srcdata >> effective_shift;
					if(srcdata & 0x80000000u)
						shifted |= 0xFFFFFFFF << (32 - effective_shift);
					break;
				case 3:
					TDEBUG("%s", "(ROR)");
					shifted |= srcdata >> effective_shift;
					shifted |= srcdata << (32 - effective_shift);
					break;
				default:
					TERROR("Bad shift operation %d\n", shift);
					return INTERP_RESULT_FATAL;
			}
			
			TDEBUG(" gives %8.8X\n", shifted);
		
			uint32_t baseonly = regs[rn];
			uint32_t withoffset = u ? (regs[rn] + shifted) : (regs[rn] - shifted);
			
			uint32_t effective = p ? withoffset : baseonly;
			if(!p)
			{
				TDEBUG("postindexed, addr=base=%8.8X, ", effective);
			}
			else
			{
				TDEBUG("preindex/offset, addr=base+off=%8.8X, ", effective);
			}
			
			interp_result_t memresult = INTERP_RESULT_FATAL;
			if(l)
			{
				TDEBUG("%s", "loading ");
				if(b)
				{
					memresult = interp_load_b(mem, memsz, effective, &(regs[rd]));
					TDEBUG("byte %2.2X", regs[rd]);
				}
				else
				{
					memresult = interp_load_w(mem, memsz, effective, &(regs[rd]));
					TDEBUG("word %8.8X", regs[rd]);
				}
				TDEBUG(" to r%d", rd);
			}
			else
			{
				TDEBUG("%s", "storing ");
				if(b)
				{
					memresult = interp_store_b(mem, memsz, effective, regs[rd]);
					TDEBUG("byte %2.2X", regs[rd]);
				}
				else
				{
					memresult = interp_store_w(mem, memsz, effective, regs[rd]);
					TDEBUG("word %8.8X", regs[rd]);
				}
				TDEBUG(" from r%d", rd);
			}
			
			if(p && w)
			{
				TDEBUG("%s", ", preindex writeback");
				regs[rn] = withoffset;
			}
			if((!p) && (!w))
			{
				TDEBUG("%s", ", postindex writeback");
				regs[rn] = withoffset;
			}
			if((!p) && w)
			{
				TERROR("%s", "\nLDRBT/LDRT/STRBT/STRT unsupported\n");
				return INTERP_RESULT_FATAL;
			}
			
			if(l && (rd == 15))
				regs[15] += 4; //Correct PC as we keep it offset
				
			
			TDEBUG("%s", "\n");
			return memresult;
			
		}
	}
	
	if(group == 0x2)
	{
		//Load/store immediate offset
		
		uint32_t immediate = ir & 0xFFFu;
		TDEBUG("load/store immediate offset %c %8.8X from r%d (%8.8X) ",
			u?'+':'-', immediate, rn, regs[rn]);
		
		uint32_t baseonly = regs[rn];
		uint32_t withoffset = u ? (regs[rn] + immediate) : (regs[rn] - immediate);
		
		uint32_t effective = p ? withoffset : baseonly;
		if(!p)
		{
			TDEBUG("postindexed, addr=base=%8.8X, ", effective);
		}
		else
		{
			TDEBUG("preindex/offset, addr=base+off=%8.8X, ", effective);
		}
		
		interp_result_t memresult = INTERP_RESULT_FATAL;
		if(l)
		{
			TDEBUG("%s", "loading ");
			if(b)
			{
				memresult = interp_load_b(mem, memsz, effective, &(regs[rd]));
				TDEBUG("byte %2.2X", regs[rd]);
			}
			else
			{
				memresult = interp_load_w(mem, memsz, effective, &(regs[rd]));
				TDEBUG("word %8.8X", regs[rd]);
			}
			TDEBUG(" to r%d", rd);
		}
		else
		{
			TDEBUG("%s", "storing ");
			if(b)
			{
				memresult = interp_store_b(mem, memsz, effective, regs[rd]);
				TDEBUG("byte %2.2X", regs[rd]);
			}
			else
			{
				memresult = interp_store_w(mem, memsz, effective, regs[rd]);
				TDEBUG("word %8.8X", regs[rd]);
			}
			TDEBUG(" from r%d", rd);
		}
		
		if(p && w)
		{
			TDEBUG("%s", ", preindex writeback");
			regs[rn] = withoffset;
		}
		if((!p) && (!w))
		{
			TDEBUG("%s", ", postindex writeback");
			regs[rn] = withoffset;
		}
		if((!p) && w)
		{
			TERROR("%s", "\nLDRBT/LDRT/STRBT/STRT unsupported\n");
			return INTERP_RESULT_FATAL;
		}
		
		if(l && (rd == 15))
			regs[15] += 4; //Correct PC as we keep it offset
			
		
		TDEBUG("%s", "\n");
		return memresult;
	}
	
	if(group == 0x1)
	{
		if(!sdata && (opcode == 8 || opcode == 10))
		{
			TERROR("Undefined instruction %8.8X\n", ir);
			return INTERP_RESULT_FATAL;
		}
		else if(!sdata && (opcode == 9 || opcode == 11))
		{
			TERROR("Move immediate to status register %8.8X\n", ir);
			return INTERP_RESULT_FATAL;
		}
		else
		{
			//Data processing immediate
			uint32_t imm8rotated = (uint32_t)imm8 >> (rotate*2);
			imm8rotated |= (uint32_t)imm8 << (32 - (rotate*2));
			
			//Seriously this is what it says on Page A5-6 of ARM DDI01001
			int shifter_carry = 0;
			if(rotate == 0)
				shifter_carry = fc ? 1 : 0;
			else
				shifter_carry = (imm8rotated & 0x80000000u) ? 1 : 0;
	
			TDEBUG("Data processing immediate, rotated immediate = %8.8X, shifter-carry %d\n", imm8rotated, shifter_carry ? 1 : 0);
			interp_dataproc(opcode, regs + rd, cpsr, regs[rn], imm8rotated, sdata, shifter_carry);
			return INTERP_RESULT_OK;
		}
	}
	
	if(group == 0x0)
	{
		if( (ir & 0x0FF000F0) == (0x01200010) )
		{
			//BX instruction (Added in ARMv5)
			if( (ir & 0x000FFF00) != 0x000FFF00 )
			{
				TERROR("BX instruction has bad should-be-one fields, %8.8X\n", ir);
				return INTERP_RESULT_FATAL;
			}
			
			TDEBUG("BX instruction to r%d = %8.8X\n", rm, regs[rm]);
			regs[15] = (regs[rm] & 0xFFFFFFFE) + 4;
			if(regs[15] & 3)
			{
				TERROR("%s", "Error - thumb mode unsupported!\n");
				return INTERP_RESULT_FATAL;
			}
			
			return INTERP_RESULT_OK;
		}
		else if( (ir & 0x0FE000F0) == 0x00A00090 )
		{
			//UMLAL - Unsigned Multiply Accumulate Long (Quake loves this one)
			int rdlo = rd;
			int rdhi = rn;
			TDEBUG("UMLAL {r%d,r%d} += r%d * r%d\n", rdhi, rdlo, rm, rs);
			
			uint64_t accum = 0;
			accum = (accum << 32) | regs[rdhi];
			accum = (accum << 32) | regs[rdlo];
			
			accum += (uint64_t)(regs[rm]) * (uint64_t)(regs[rs]);
			
			regs[rdhi] = (accum >> 32) & 0xFFFFFFFFu;
			regs[rdlo] = (accum >>  0) & 0xFFFFFFFFu;
			
			if(sdata)
			{
				TDEBUG("%s", "Writing flags for UMLAL");
				if(accum == 0)
				{
					TDEBUG(" %s", "Z1");
					*cpsr |= FLAG_Z;
				}
				else
				{
					TDEBUG(" %s", "Z0");				
					*cpsr &= ~FLAG_Z;
				}
				
				if(regs[rdhi] & 0x80000000u)
				{
					TDEBUG(" %s", "N1");
					*cpsr |= FLAG_N;
				}
				else
				{
					TDEBUG(" %s", "N0");				
					*cpsr &= ~FLAG_N;
				}
				TDEBUG("%s", "\n");
			}
			return INTERP_RESULT_OK;
			
		}
		else if( (ir & 0x0FE000F0) == 0x00E00090 )
		{
			//SMLAL - Signed Multiply Accumulate Long
			int rdlo = rd;
			int rdhi = rn;
			TDEBUG("SMLAL {r%d,r%d} += r%d * r%d\n", rdhi, rdlo, rm, rs);
			
			uint64_t accum = 0;
			accum = (accum << 32) | regs[rdhi];
			accum = (accum << 32) | regs[rdlo];
			
			accum += (int64_t)((int32_t)(regs[rm])) * (int64_t)((int32_t)(regs[rs]));
			
			regs[rdhi] = (accum >> 32) & 0xFFFFFFFFu;
			regs[rdlo] = (accum >>  0) & 0xFFFFFFFFu;
			
			if(sdata)
			{
				TDEBUG("%s", "Writing flags for SMLAL");
				if(accum == 0)
				{
					TDEBUG(" %s", "Z1");
					*cpsr |= FLAG_Z;
				}
				else
				{
					TDEBUG(" %s", "Z0");				
					*cpsr &= ~FLAG_Z;
				}
				
				if(regs[rdhi] & 0x80000000u)
				{
					TDEBUG(" %s", "N1");
					*cpsr |= FLAG_N;
				}
				else
				{
					TDEBUG(" %s", "N0");				
					*cpsr &= ~FLAG_N;
				}
				TDEBUG("%s", "\n");
			}
			return INTERP_RESULT_OK;
		}
		else if( (ir & 0x0FE000F0) == 0x00200090 )
		{
			//MLA - Multiply accumulate
			//Note rd and rn positions reversed
			TDEBUG("MLA multiply accumulate (r%d * r%d) + r%d -> r%d\n", rm, rs, rd, rn);
			regs[rn] = (regs[rm] * regs[rs]) + regs[rd];
			if(sdata)
			{
				//Update N and Z flags
				TDEBUG("%s", "Writing flags for MLA");
				if(regs[rn] == 0)
				{
					TDEBUG(" %s", "Z1");
					*cpsr |= FLAG_Z;
				}
				else
				{
					TDEBUG(" %s", "Z0");				
					*cpsr &= ~FLAG_Z;
				}
				
				if(regs[rn] & 0x80000000u)
				{
					TDEBUG(" %s", "N1");
					*cpsr |= FLAG_N;
				}
				else
				{
					TDEBUG(" %s", "N0");				
					*cpsr &= ~FLAG_N;
				}
				TDEBUG("%s", "\n");
			}
			return INTERP_RESULT_OK;
		}
		else if( (ir & 0x0FE0F0F0) == 0x00000090 )
		{
			//MUL - Multiply
			//Note "rn" position used as destination "rd"
			TDEBUG("MUL Multiply r%d * r%d -> r%d\n", rm, rs, rn);
			regs[rn] = regs[rm] * regs[rs];
			if(sdata)
			{
				//Update N and Z flags
				TDEBUG("%s", "Writing flags for MUL");
				if(regs[rn] == 0)
				{
					TDEBUG(" %s", "Z1");
					*cpsr |= FLAG_Z;
				}
				else
				{
					TDEBUG(" %s", "Z0");				
					*cpsr &= ~FLAG_Z;
				}
				
				if(regs[rn] & 0x80000000u)
				{
					TDEBUG(" %s", "N1");
					*cpsr |= FLAG_N;
				}
				else
				{
					TDEBUG(" %s", "N0");				
					*cpsr &= ~FLAG_N;
				}
				TDEBUG("%s", "\n");
			}
			return INTERP_RESULT_OK;
			
		}
		else if( (ir & 0x0FE000F0) == 0x00C00090 )
		{
			//SMULL - Signed multiply long
			int rdlo = rd;
			int rdhi = rn;
			TDEBUG("SMULL Signed Multiply Long r%d * r%d -> {r%d,r%d}\n", rm, rs, rdhi, rdlo);
			
			int64_t mul_a = (int32_t)regs[rm];
			int64_t mul_b = (int32_t)regs[rs];
			int64_t product = mul_a * mul_b;
			
			regs[rdlo] = (product >>  0) & 0xFFFFFFFFu;
			regs[rdhi] = (product >> 32) & 0xFFFFFFFFu;
		
			if(sdata)
			{
				//Update N and Z flags
				TDEBUG("%s", "Writing flags for SMULL");
				if(product == 0)
				{
					TDEBUG(" %s", "Z1");
					*cpsr |= FLAG_Z;
				}
				else
				{
					TDEBUG(" %s", "Z0");				
					*cpsr &= ~FLAG_Z;
				}
				
				if(product < 0)
				{
					TDEBUG(" %s", "N1");
					*cpsr |= FLAG_N;
				}
				else
				{
					TDEBUG(" %s", "N0");				
					*cpsr &= ~FLAG_N;
				}
				TDEBUG("%s", "\n");
			}
			return INTERP_RESULT_OK;
		}
		else if( (ir & 0x0FE000F0) == 0x00800090 )
		{
			//UMULL - Unsigned multiply long
			int rdlo = rd;
			int rdhi = rn;
			TDEBUG("UMULL Unsigned Multiply Long r%d * r%d -> {r%d,r%d}\n", rm, rs, rdhi, rdlo);
			
			uint64_t mul_a = (uint32_t)regs[rm];
			uint64_t mul_b = (uint32_t)regs[rs];
			uint64_t product = mul_a * mul_b;
			
			regs[rdlo] = (product >>  0) & 0xFFFFFFFFu;
			regs[rdhi] = (product >> 32) & 0xFFFFFFFFu;
		
			if(sdata)
			{
				//Update N and Z flags
				TDEBUG("%s", "Writing flags for UMULL");
				if(product == 0)
				{
					TDEBUG(" %s", "Z1");
					*cpsr |= FLAG_Z;
				}
				else
				{
					TDEBUG(" %s", "Z0");				
					*cpsr &= ~FLAG_Z;
				}
				
				if(product & 0x8000000000000000ull)
				{
					TDEBUG(" %s", "N1");
					*cpsr |= FLAG_N;
				}
				else
				{
					TDEBUG(" %s", "N0");				
					*cpsr &= ~FLAG_N;
				}
				TDEBUG("%s", "\n");
			}
			return INTERP_RESULT_OK;			
		}
		else if( (ir & 0x0E000090) == 0x00000090 )
		{
			//Load and store halfword or doubleword, and load signed byte (Added in ARMv5/v5TE)
			//Load/store, size, and signedness determined by L, S, H bits
			TDEBUG("ARMv5 sbyte/hword/dword load/store %8.8X\n", ir);
			int lsh = 0;
			lsh += (ir & (1u << 20)) ? 4 : 0; //L
			lsh += (ir & (1u <<  6)) ? 2 : 0; //S
			lsh += (ir & (1u <<  5)) ? 1 : 0; //H
			
			uint32_t offset = 0;
			if(ir & (1u << 22))
			{
				//Bit 22 set - immediate offset/index addressing
				uint32_t immedh = (ir >> 8) & 0xF;
				uint32_t immedl = (ir >> 0) & 0xF;
				offset = (immedh << 4) + immedl;
				TDEBUG("Immediate offset %8.8X\n", offset);
			}
			else
			{
				//Bit 22 clear - register offset/index addressing
				uint32_t sbz = (ir >> 8) & 0xF;
				if(sbz != 0)
				{
					TERROR("Should-be-zero bits in byte/halfword load/store %8.8X not zero!\n", ir);
					return INTERP_RESULT_FATAL;
				}
				offset = regs[rm];
				TDEBUG("Register offset r%d = %8.8X\n", rm, offset);
			}
			
			uint32_t effective = regs[rn];
			if(!p)
			{
				TDEBUG("Postindexed addressing, using base r%d = %8.8X as address\n", rn, regs[rn]);
			}
			else
			{
				if(u)
					effective += offset;
				else
					effective -= offset;
				
				TDEBUG("Offset/preindex addressing, using r%d (%8.8X) %c %8.8X = %8.8X as address\n",
					rn, regs[rn], u?'+':'-', offset, effective);
			}
				
			
			//Type of load/store determined by LSH bits...
			interp_result_t memresult = INTERP_RESULT_OK;
			switch(lsh)
			{
				case 1: //Store halfword
				{
					memresult = interp_store_h(mem, memsz, effective, regs[rd]);
					TDEBUG("Store halfword %4.4X from r%d to %8.8X\n",
						regs[rd] & 0xFFFF, rd, effective);
				}
				break;
				case 2: //Load doubleword
				{
					uint64_t dw = 0;
					memresult = interp_load_d(mem, memsz, effective, &dw);
					regs[rd ^ 0] = dw >>  0;
					regs[rd ^ 1] = dw >> 32;
					TDEBUG("Load doubleword %8.8X %8.8X from %8.8X to r%d r%d\n",
						regs[rd^0], regs[rd^1], effective, rd^0, rd^1);
				}
				break;
				case 3: //Store doubleword
				{
					uint64_t dw = 0;
					dw = regs[rd ^ 0];
					dw |= ((uint64_t)regs[rd ^ 1]) << 32;
					memresult = interp_store_d(mem, memsz, effective, dw);
					TDEBUG("Store doubleword %8.8X %8.8X from r%d r%d to %8.8X\n",
						regs[rd^0], regs[rd^1], rd^0, rd^1, effective);
				}
				break;
				case 5: //Load unsigned halfword
				{
					memresult = interp_load_h(mem, memsz, effective, &(regs[rd]));
					TDEBUG("Load halfword %8.8X from %8.8X to r%d\n",
						regs[rd], effective, rd);
				}
				break;
				case 6: //Load signed byte
				{
					memresult = interp_load_b(mem, memsz, effective, &(regs[rd]));
					if((memresult == INTERP_RESULT_OK) && (regs[rd] & 0x80))
						regs[rd] |= 0xFFFFFF00;
					
					TDEBUG("Load signed byte %8.8X from %8.8X to r%d\n",
						regs[rd], effective, rd);
				}
				break;
				case 7: //Load signed halfword
				{
					memresult = interp_load_h(mem, memsz, effective, &(regs[rd]));
					if((memresult == INTERP_RESULT_OK) && (regs[rd] & 0x8000))
						regs[rd] |= 0xFFFF0000u;
					
					TDEBUG("Load signed halfword %8.8X from %8.8X to r%d\n",
						regs[rd], effective, rd);
				}
				break;
				default:
					TERROR("Bad combination of LSH bits %X in ARMv5 load/store %8.8X\n", lsh, ir);
					return INTERP_RESULT_FATAL;
			}
			
			//Optionally write-back the now-offset address to the original register
			if(p && w)
			{
				//New memory address written back after offsetting
				regs[rn] = effective;
				TDEBUG("Writing back r%d = %8.8X after preindex operation\n", rn, regs[rn]);
			}
			else if(p)
			{
				//Base register unchanged (offset addressing)
			}
			else if(w)
			{
				//Invalid combination in ARMv5TE
				TERROR("Load/store with W && !P bits %8.8X, architecturally unpredictable\n", ir);
				return INTERP_RESULT_FATAL;
			}
			else
			{
				//Offset applied and written back but only after doing the access
				regs[rn] = u ? (effective+offset) : (effective-offset);
				TDEBUG("Writing back r%d = %8.8X after postindex operation\n", rn, regs[rn]);
			}
			
			return memresult;
		}
		else
		{
			if( (ir & (1u << 7)) && (ir & (1u << 4)) )
			{
				TERROR("Unhandled Multiply %8.8X\n", ir);
				return INTERP_RESULT_FATAL;
			}
			else if( ((opcode & 0xC) == 0x8) && !sdata )
			{
				if( (ir & 0x0FFF0FF0) == 0x016F0F10 )
				{
					TDEBUG("%s", "Count leading zeroes (CLZ)\n");
					uint32_t tocount = regs[rm]; //in case rm==rd
					regs[rd] = 0;
					for(uint32_t tt = 0x80000000u; tt != 0; tt >>= 1)
					{
						if(tocount & tt)
							break;
						regs[rd]++;
					}
					return INTERP_RESULT_OK;
				}
				
				if( (ir & 0x0FF00090) == 0x01000080 )
				{
					TDEBUG("%s", "Signed multiply accumulate bottom/top 16-bits\n");
					
					//Note - rd and rn flipped relative to normal decoding!
					
					int16_t mula = (ir & (1u << 5)) ? ((regs[rm] >> 16) & 0xFFFF) : (regs[rm] & 0xFFFF);
					int16_t mulb = (ir & (1u << 6)) ? ((regs[rs] >> 16) & 0xFFFF) : (regs[rs] & 0xFFFF);
					int64_t result = ((int64_t)mula * (int64_t)mulb) + (int64_t)regs[rd];
					if(result < (int64_t)(0xFFFFFFFF80000000) || result > (int64_t)0x7FFFFFFF)
						*cpsr |= FLAG_Q;
						
					regs[rn] = result; //Note nonstandard decoding of rd/rn
					
					return INTERP_RESULT_OK;
				}
				
				if( (ir & 0x0FFFFFF0) == (0x012FFF30) )
				{
					//Branch and link and exchange thumb state (blx)
					TDEBUG("Branch and link and exchange to register r%d = %8.8X ", rm, regs[rm]); 
					regs[14] = regs[15] - 4;
					regs[15] = regs[rm] + 4; //We store PC offset
					TDEBUG("saved %8.8X in LR\n", regs[14]);
		
					return INTERP_RESULT_OK;
				}
				
				//Compare instruction that doesn't write-back the flags...?
				TERROR("Unhandled misc instruction %8.8X\n", ir);
				return INTERP_RESULT_FATAL;
			}
			else
			{
				//Data processing immediate shift / data processing register shift
				//Instruction bit 4 determines whether immediate or register shift amount
				int shift_by_reg = (ir & (1u << 4));
				if(shift_by_reg)
					TDEBUG("Data processing register shift, rs=%d\n", rs);
				else
					TDEBUG("Data processing immediate shift, shamt=%d\n", shamt);
				
				int to_shift = shift_by_reg ? (regs[rs] & 0xFF) : shamt;
				
				//You gotta be shitting me, this is actually how ARM defines it
				uint32_t shifter_operand = 0;
				bool shifter_carry_out = false;
				if(shift == 0)
				{
					if(!shift_by_reg)
					{
						TDEBUG("%s", "LSL imm");
						if(to_shift == 0)
						{
							shifter_operand = regs[rm];
							shifter_carry_out = fc;
						}	
						else
						{
							shifter_operand = regs[rm] << to_shift;
							shifter_carry_out = regs[rm] & (1u << (32 - to_shift));
						}
					}
					else
					{
						TDEBUG("%s", "LSL reg");
						if(to_shift == 0)
						{
							shifter_operand = regs[rm];
							shifter_carry_out = fc;
						}
						else if(to_shift < 32)
						{
							shifter_operand = regs[rm] << to_shift;
							shifter_carry_out = regs[rm] & (1u << (32 - to_shift));
						}
						else if(to_shift == 32)
						{
							shifter_operand = 0;
							shifter_carry_out = regs[rm] & 1;
						}
						else
						{
							shifter_operand = 0;
							shifter_carry_out = 0;
						}
					}
				}
				else if(shift == 1)
				{
					if(!shift_by_reg)
					{
						TDEBUG("%s", "LSR imm");
						if(to_shift == 0)
						{
							shifter_operand = 0;
							shifter_carry_out = regs[rm] & (1u << 31);
						}
						else
						{
							shifter_operand = regs[rm] >> to_shift;
							shifter_carry_out = regs[rm] & (1u << (to_shift - 1));
						}
					}
					else
					{
						TDEBUG("%s", "LSR reg");
						if(to_shift == 0)
						{
							shifter_operand = regs[rm];
							shifter_carry_out = fc;
						}
						else if(to_shift < 32)
						{
							shifter_operand = regs[rm] >> to_shift;
							shifter_carry_out = regs[rm] & (1u << (to_shift - 1));
						}
						else if(to_shift == 32)
						{
							shifter_operand = 0;
							shifter_carry_out = regs[rm] & (1u << 31);
						}
						else
						{
							shifter_operand = 0;
							shifter_carry_out = 0;
						}
					}
				}
				else if(shift == 2)
				{
					if(!shift_by_reg)
					{
						TDEBUG("%s", "ASR imm");
						if(to_shift == 0)
						{
							if(!(regs[rm] & (1u << 31)))
							{
								shifter_operand = 0;
								shifter_carry_out = 0;
							}
							else
							{
								shifter_operand = 0xFFFFFFFFu;
								shifter_carry_out = 1;
							}
						}
						else
						{
							shifter_operand = regs[rm] >> to_shift;
							if(regs[rm] & (1u << 31))
								shifter_operand |= (0xFFFFFFFFu) << (32 - to_shift);
						}
					}
					else
					{
						TDEBUG("%s", "ASR reg");
						if(to_shift == 0)
						{
							shifter_operand = regs[rm];
							shifter_carry_out = fc;
						}
						else if(to_shift < 32)
						{
							shifter_operand = regs[rm] >> to_shift;
							if(regs[rm] & (1u << 31))
								shifter_operand |= (0xFFFFFFFFu) << (32 - to_shift);
							
							shifter_carry_out = regs[rm] & (1u << (to_shift - 1));
						}
						else if(to_shift >= 32)
						{
							if(!(regs[rm] & (1u << 31)))
							{
								shifter_operand = 0;
								shifter_carry_out = 0;
							}
							else
							{
								shifter_operand = 0xFFFFFFFFu;
								shifter_carry_out = 1;
							}
						}
					}
				}
				else if(shift == 3)
				{
					if(!shift_by_reg)
					{
						TDEBUG("%s", "ROR imm");
						if(to_shift == 0)
						{
							TDEBUG("%s", " with extend");
							shifter_operand = fc ? 0x80000000u : 0;
							shifter_operand |= regs[rm] >> 1;
							shifter_carry_out = regs[rm] & 1;
						}
						else
						{
							shifter_operand = regs[rm] >> to_shift;
							shifter_operand |= regs[rm] << (32 - to_shift);
							shifter_carry_out = regs[rm] & (1u << (to_shift - 1));
						}
					}
					else
					{
						TDEBUG("%s", "ROR reg");
						if(to_shift == 0)
						{
							shifter_operand = regs[rm];
							shifter_carry_out = fc;
						}
						else if( (to_shift&0x1F) == 0 )
						{
							shifter_operand = regs[rm];
							shifter_carry_out = regs[rm] & (1u << 31);
						}
						else
						{
							shifter_operand = regs[rm] >> (to_shift & 0x1F);
							shifter_operand |= regs[rm] << (32 - (to_shift & 0x1F));
							shifter_carry_out = regs[rm] & (1u << ((to_shift & 0x1F)-1));
						}
					}
				}
				else
				{
					TERROR("Bad shift operation %d\n", shift);
					return INTERP_RESULT_FATAL;
				}
				
				TDEBUG(" r%d (%8.8X) by %d gives %8.8X with shifter-carry %d", 
					rm, regs[rm], to_shift, shifter_operand, shifter_carry_out ? 1 : 0);
				
				TDEBUG("\nOp Destination=r%d\n", rd);
				interp_dataproc(opcode, regs + rd, cpsr, regs[rn], shifter_operand, sdata, shifter_carry_out);
				return INTERP_RESULT_OK;
			}
		}
	}
	
	TERROR("Bad decoding for instruction %8.8X\n", ir);
	return INTERP_RESULT_FATAL;
}

interp_result_t interp_step(uint32_t *regs, uint32_t *cpsr, uint32_t *mem, size_t memsz)
{
	//Dumb stuff about what "PC" actually reads as during an instruction
	regs[15] += 4;
	interp_result_t r = interp_step_inner(regs, cpsr, mem, memsz, 0);
	regs[15] -= 4;
		
	return r;
}

interp_result_t interp_step_force(uint32_t *regs, uint32_t *cpsr, uint32_t *mem, size_t memsz, uint32_t ir)
{
	//Dumb stuff about what "PC" actually reads as during an instruction
	regs[15] += 4;
	interp_result_t r = interp_step_inner(regs, cpsr, mem, memsz, ir);
	regs[15] -= 4;
		
	return r;	
}

