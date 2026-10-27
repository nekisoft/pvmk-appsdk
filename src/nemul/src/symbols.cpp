//symbols.cpp
//Function names and source line positions read from an emulated program's ELF file
//Bryan E. Topp <betopp@betopp.com> 2025

//Nemul, the Neki32 Simulator, Copyright 2025 Nekisoft Pty Ltd, ACN 680 583 251
//This program is free software: you can redistribute it and/or modify
//it under the terms of the GNU General Public License as published by
//the Free Software Foundation, either version 3 of the License, or
//(at your option) any later version.

//Two pieces of the file matter here: .symtab, which is a plain list of addresses and names, and
//.debug_line, which is a small program that, run from the start, walks the addresses of the
//program in the order the compiler wrote them and says which file and line each belongs to.
//
//The line program comes in four versions with three different headers. The SDK's compiler emits
//whichever its GCC was built for - most units here are version 3, a few are version 5 - so both
//shapes are read.

#include "symbols.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//One row of the line table: addresses from addr up to end, from this file and line
typedef struct sym_row_s
{
	uint32_t addr;
	uint32_t end;
	int file;
	int line;
} sym_row_t;

//One function symbol
typedef struct sym_func_s
{
	uint32_t addr;
	uint32_t size;
	uint32_t name;
} sym_func_t;

struct symbols_s
{
	//The whole file, kept because the names in it point into it
	uint8_t *data;
	long size;
	long loaded;

	//Functions, sorted by address
	sym_func_t *funcs;
	int nfuncs;

	//Line table rows, sorted by address
	sym_row_t *rows;
	int nrows;
	int maxrows;

	//File names the line table refers to, as one big block of strings
	char **files;
	int nfiles;
	int maxfiles;
};

//Bounds-checked little-endian reads, so a truncated or wrong file cannot run off the end
static bool rd(uint8_t *data, long size, long at, void *out, long len)
{
	if(at < 0 || len < 0 || at + len > size)
		return false;
	memcpy(out, data + at, len);
	return true;
}

static bool rd8(uint8_t *data, long size, long at, uint8_t *out)
{
	return rd(data, size, at, out, 1);
}

static bool rd16(uint8_t *data, long size, long at, uint16_t *out)
{
	uint8_t raw[2];
	if(!rd(data, size, at, raw, 2)) return false;
	*out = (uint16_t)(raw[0] | (raw[1] << 8));
	return true;
}

static bool rd32(uint8_t *data, long size, long at, uint32_t *out)
{
	uint8_t raw[4];
	if(!rd(data, size, at, raw, 4)) return false;
	*out = (uint32_t)raw[0] | ((uint32_t)raw[1] << 8) | ((uint32_t)raw[2] << 16) |
		((uint32_t)raw[3] << 24);
	return true;
}

//Unsigned and signed variable-length numbers, which the line program uses for everything
static bool rd_uleb(uint8_t *data, long size, long *at, uint64_t *out)
{
	uint64_t value = 0;
	int shift = 0;
	for(;;)
	{
		uint8_t byte = 0;
		if(!rd8(data, size, (*at)++, &byte)) return false;
		value |= (uint64_t)(byte & 0x7F) << shift;
		shift += 7;
		if(!(byte & 0x80))
			break;
		if(shift > 63) return false;
	}
	*out = value;
	return true;
}

static bool rd_sleb(uint8_t *data, long size, long *at, int64_t *out)
{
	int64_t value = 0;
	int shift = 0;
	uint8_t byte = 0;
	do
	{
		if(!rd8(data, size, (*at)++, &byte)) return false;
		value |= (int64_t)(byte & 0x7F) << shift;
		shift += 7;
		if(shift > 63) return false;
	} while(byte & 0x80);

	if((byte & 0x40) && shift < 64)
		value |= -((int64_t)1 << shift);

	*out = value;
	return true;
}

static bool add_row(symbols_t *syms, uint32_t addr, int file, int line)
{
	if(syms->nrows >= syms->maxrows)
	{
		const int newmax = syms->maxrows ? (syms->maxrows * 2) : 1024;
		sym_row_t *bigger = (sym_row_t*)realloc(syms->rows, sizeof(sym_row_t) * newmax);
		if(bigger == NULL) return false;
		syms->rows = bigger;
		syms->maxrows = newmax;
	}

	sym_row_t *row = &(syms->rows[syms->nrows]);
	row->addr = addr;
	row->end = addr;   //filled in when the sequence ends
	row->file = file;
	row->line = line;
	syms->nrows++;
	return true;
}

//Adds a row for an address, and closes out the rows at the address before it, which cover the
//program up to where this one starts
static int emit_row(symbols_t *syms, uint32_t addr, int file, int line,
	int *run_start, uint32_t *run_addr)
{
	if(addr != *run_addr)
	{
		for(int rr = *run_start; rr < syms->nrows; rr++)
			syms->rows[rr].end = addr;
		*run_start = syms->nrows;
		*run_addr = addr;
	}
	return add_row(syms, addr, file, line);
}

static int add_file(symbols_t *syms, const char *name)
{
	if(syms->nfiles >= syms->maxfiles)
	{
		const int newmax = syms->maxfiles ? (syms->maxfiles * 2) : 32;
		char **bigger = (char**)realloc(syms->files, sizeof(char*) * newmax);
		if(bigger == NULL) return -1;
		syms->files = bigger;
		syms->maxfiles = newmax;
	}

	char *copy = strdup(name);
	if(copy == NULL) return -1;
	syms->files[syms->nfiles] = copy;
	return syms->nfiles++;
}

//Reads one form from the line table header's directory and file tables
static bool rd_form(symbols_t *syms, uint16_t form, long *at, char *text, int textsz,
	uint64_t *num_out, long line_str_off, long line_str_size,
	long str_off, long str_size)
{
	uint8_t u8 = 0;
	uint16_t u16 = 0;
	uint32_t u32 = 0;
	uint64_t u64 = 0;

	switch(form)
	{
		case 0x08:   //DW_FORM_string
		{
			long start = *at;
			for(;;)
			{
				if(!rd8(syms->data, syms->size, *at, &u8)) return false;
				(*at)++;
				if(u8 == 0) break;
			}
			long len = (*at - 1) - start;
			if(len >= textsz) len = textsz - 1;
			memcpy(text, syms->data + start, (size_t)len);
			text[len] = 0;
			return true;
		}

		case 0x1f:   //DW_FORM_line_strp
		{
			if(!rd32(syms->data, syms->size, *at, &u32)) return false;
			*at += 4;
			if(line_str_off == 0 && line_str_size == 0) return false;
			if((long)u32 >= line_str_size) return false;
			long start = line_str_off + u32;
			long stop = start;
			while(stop < line_str_off + line_str_size && syms->data[stop] != 0)
				stop++;
			long len = stop - start;
			if(len >= textsz) len = textsz - 1;
			memcpy(text, syms->data + start, (size_t)len);
			text[len] = 0;
			return true;
		}

		case 0x0e:   //DW_FORM_strp
		{
			if(!rd32(syms->data, syms->size, *at, &u32)) return false;
			*at += 4;
			if((long)u32 >= str_size) return false;
			long start = str_off + u32;
			long stop = start;
			while(stop < str_off + str_size && syms->data[stop] != 0)
				stop++;
			long len = stop - start;
			if(len >= textsz) len = textsz - 1;
			memcpy(text, syms->data + start, (size_t)len);
			text[len] = 0;
			return true;
		}

		case 0x0b:   //DW_FORM_data1
			if(!rd8(syms->data, syms->size, (*at)++, &u8)) return false;
			*num_out = u8;
			return true;

		case 0x05:   //DW_FORM_data2
			if(!rd16(syms->data, syms->size, *at, &u16)) return false;
			*at += 2;
			*num_out = u16;
			return true;

		case 0x06:   //DW_FORM_data4
			if(!rd32(syms->data, syms->size, *at, &u32)) return false;
			*at += 4;
			*num_out = u32;
			return true;

		case 0x07:   //DW_FORM_data8
			if(!rd32(syms->data, syms->size, *at, &u32)) return false;
			u64 = u32;
			if(!rd32(syms->data, syms->size, *at + 4, &u32)) return false;
			*at += 8;
			*num_out = u64 | ((uint64_t)u32 << 32);
			return true;

		case 0x0f:   //DW_FORM_udata
			return rd_uleb(syms->data, syms->size, at, num_out);

		case 0x0d:   //DW_FORM_sdata
		{
			int64_t s = 0;
			if(!rd_sleb(syms->data, syms->size, at, &s)) return false;
			*num_out = (uint64_t)s;
			return true;
		}

		case 0x19:   //DW_FORM_flag_present
			*num_out = 1;
			return true;

		case 0x1e:   //DW_FORM_data16
			*at += 16;
			return true;

		default:
			//Anything else (strx and friends) needs tables this does not read
			return false;
	}
}

//Runs one line number program, adding a row for every address it emits
static bool read_line_program(symbols_t *syms, long unit_start, long unit_end,
	long line_str_off, long line_str_size, long str_off, long str_size)
{
	long at = unit_start;
	uint16_t version = 0;
	uint32_t header_length = 0;
	uint8_t address_size = 4;
	uint8_t segment_size = 0;

	if(!rd16(syms->data, syms->size, at + 4, &version))
		return false;
	at += 6;

	if(version >= 5)
	{
		if(!rd8(syms->data, syms->size, at, &address_size)) return false;
		if(!rd8(syms->data, syms->size, at + 1, &segment_size)) return false;
		if(!rd32(syms->data, syms->size, at + 2, &header_length)) return false;
		at += 6;
	}
	else
	{
		if(!rd32(syms->data, syms->size, at, &header_length)) return false;
		at += 4;
	}
	(void)address_size;
	(void)segment_size;

	uint8_t min_inst = 0, max_ops = 0, default_stmt = 0, line_range = 0, opcode_base = 0;
	int8_t line_base = 0;
	int file_base = 0;   //where this unit's file table starts in our list
	if(!rd8(syms->data, syms->size, at, &min_inst)) return false;
	at += 1;
	if(version >= 4)
		at += 1;   //maximum_operations_per_instruction
	if(!rd8(syms->data, syms->size, at, &default_stmt)) return false;
	if(!rd8(syms->data, syms->size, at + 1, (uint8_t*)&line_base)) return false;
	if(!rd8(syms->data, syms->size, at + 2, &line_range)) return false;
	if(!rd8(syms->data, syms->size, at + 3, &opcode_base)) return false;
	at += 4;
	(void)default_stmt;
	(void)max_ops;

	if(opcode_base == 0 || line_range == 0)
		return false;

	//How many operands each standard opcode takes, which has to be honoured even for the ones
	//that change nothing shown here: reading past an operand desynchronises the whole program
	uint8_t opcode_operands[256] = { 0 };
	for(int oo = 0; oo < (int)opcode_base - 1 && oo < 255; oo++)
	{
		if(!rd8(syms->data, syms->size, at, &(opcode_operands[oo]))) return false;
		at++;
	}

	//Version 5 puts the directories and files in tables of their own, each entry built from
	//pairs of a content type and a form
	if(version >= 5)
	{
		uint8_t dir_count_fmt = 0;
		if(!rd8(syms->data, syms->size, at, &dir_count_fmt)) return false;
		at += 1;

		uint16_t dir_types[8];
		uint16_t dir_forms[8];
		for(int ff = 0; ff < dir_count_fmt; ff++)
		{
			if(!rd16(syms->data, syms->size, at, &(dir_types[ff]))) return false;
			if(!rd16(syms->data, syms->size, at + 2, &(dir_forms[ff]))) return false;
			at += 4;
		}

		uint32_t dir_count = 0;
		if(!rd32(syms->data, syms->size, at, &dir_count)) return false;
		at += 4;

		//Directories are indexed from zero here but from one in the file entries, so keep a
		//stand-in at index zero
		int dir_base = add_file(syms, "");

		char text[512];
		for(uint32_t dd = 0; dd < dir_count; dd++)
		{
			text[0] = 0;
			for(int ff = 0; ff < dir_count_fmt; ff++)
			{
				uint64_t num = 0;
				char piece[512];
				piece[0] = 0;
				if(!rd_form(syms, dir_forms[ff], &at, piece, sizeof(piece), &num,
					line_str_off, line_str_size, str_off, str_size))
					return false;
				if(dir_types[ff] == 1)
					snprintf(text, sizeof(text), "%s", piece);
			}
			add_file(syms, text);
		}
		(void)dir_base;

		uint8_t file_count_fmt = 0;
		if(!rd8(syms->data, syms->size, at, &file_count_fmt)) return false;
		at += 1;

		uint16_t file_types[8];
		uint16_t file_forms[8];
		for(int ff = 0; ff < file_count_fmt; ff++)
		{
			if(!rd16(syms->data, syms->size, at, &(file_types[ff]))) return false;
			if(!rd16(syms->data, syms->size, at + 2, &(file_forms[ff]))) return false;
			at += 4;
		}

		uint32_t file_count = 0;
		if(!rd32(syms->data, syms->size, at, &file_count)) return false;
		at += 4;

		file_base = syms->nfiles;
		for(uint32_t ff = 0; ff < file_count; ff++)
		{
			text[0] = 0;
			uint64_t dir_index = 0;
			for(int cc = 0; cc < file_count_fmt; cc++)
			{
				uint64_t num = 0;
				char piece[512];
				piece[0] = 0;
				if(!rd_form(syms, file_forms[cc], &at, piece, sizeof(piece), &num,
					line_str_off, line_str_size, str_off, str_size))
					return false;
				if(file_types[cc] == 1)
					snprintf(text, sizeof(text), "%s", piece);
				if(file_types[cc] == 2)
					dir_index = num;
			}

			//Join the directory and the name the way the compiler meant them
			char joined[1024];
			if(dir_index > 0 && dir_index <= dir_count && syms->files[dir_index] != NULL &&
				syms->files[dir_index][0] != 0)
				snprintf(joined, sizeof(joined), "%s/%s", syms->files[dir_index], text);
			else
				snprintf(joined, sizeof(joined), "%s", text);
			add_file(syms, joined);
		}
		//File numbers in the program count from one
	}
	else
	{
		//Older versions put the include directory names in a list, then the files
		char text[512];
		int dirs[256];
		int ndirs = 0;
		for(;;)
		{
			long start = at;
			uint8_t u8 = 0;
			for(;;)
			{
				if(!rd8(syms->data, syms->size, at, &u8)) return false;
				at++;
				if(u8 == 0) break;
			}
			long len = (at - 1) - start;
			if(len == 0) break;
			if(len >= (long)sizeof(text)) len = sizeof(text) - 1;
			memcpy(text, syms->data + start, (size_t)len);
			text[len] = 0;
			if(ndirs < 256)
				dirs[ndirs++] = add_file(syms, text);
		}

		//File numbers in this unit count from one, starting at the first entry below
		file_base = syms->nfiles;
		for(;;)
		{
			long start = at;
			uint8_t u8 = 0;
			for(;;)
			{
				if(!rd8(syms->data, syms->size, at, &u8)) return false;
				at++;
				if(u8 == 0) break;
			}
			long len = (at - 1) - start;
			if(len == 0) break;
			if(len >= (long)sizeof(text)) len = sizeof(text) - 1;
			memcpy(text, syms->data + start, (size_t)len);
			text[len] = 0;

			uint64_t dir_index = 0, mtime = 0, length = 0;
			if(!rd_uleb(syms->data, syms->size, &at, &dir_index)) return false;
			if(!rd_uleb(syms->data, syms->size, &at, &mtime)) return false;
			if(!rd_uleb(syms->data, syms->size, &at, &length)) return false;
			(void)mtime; (void)length;

			char joined[1024];
			if(dir_index > 0 && dir_index <= (uint64_t)ndirs)
				snprintf(joined, sizeof(joined), "%s/%s", syms->files[dirs[dir_index - 1]], text);
			else
				snprintf(joined, sizeof(joined), "%s", text);
			add_file(syms, joined);
		}
	}
	(void)str_off; (void)str_size;

	//Run the program. File numbers count from one in every version, but each unit has its own
	//file table, so the rows carry the position of the file in the list built above.
	const long program = unit_start + 6 + (version >= 5 ? 6 : 4) + header_length;
	if(program < unit_start || program > unit_end)
		return false;
	const int file_zero = file_base - 1;
	at = program;

	uint32_t addr = 0;
	int file = 1;
	uint32_t line = 1;
	int run_start = syms->nrows;          //first row at run_addr, whose end is not known yet
	uint32_t run_addr = 0xFFFFFFFFu;

	while(at < unit_end)
	{
		uint8_t op = 0;
		if(!rd8(syms->data, syms->size, at++, &op)) return false;

		if(op >= opcode_base)
		{
			const int adjusted = op - opcode_base;
			addr += (uint32_t)min_inst * (uint32_t)(adjusted / line_range);
			line += (uint32_t)(line_base + (adjusted % line_range));
			emit_row(syms, addr, file_zero + file, (int)line, &run_start, &run_addr);
			continue;
		}

		switch(op)
		{
			case 1:   //copy
			{
				emit_row(syms, addr, file_zero + file, (int)line, &run_start, &run_addr);
				break;
			}

			case 2:   //advance_pc
			{
				uint64_t advance = 0;
				if(!rd_uleb(syms->data, syms->size, &at, &advance)) return false;
				addr += (uint32_t)advance * min_inst;
				break;
			}

			case 3:   //advance_line
			{
				int64_t advance = 0;
				if(!rd_sleb(syms->data, syms->size, &at, &advance)) return false;
				line = (uint32_t)((int64_t)line + advance);
				break;
			}

			case 4:   //set_file
			{
				uint64_t set = 0;
				if(!rd_uleb(syms->data, syms->size, &at, &set)) return false;
				file = (int)set;
				break;
			}

			case 8:   //const_add_pc
			{
				addr += (uint32_t)min_inst * (uint32_t)((255 - opcode_base) / line_range);
				break;
			}

			case 9:   //fixed_advance_pc
			{
				uint16_t advance = 0;
				if(!rd16(syms->data, syms->size, at, &advance)) return false;
				at += 2;
				addr += advance;
				break;
			}

			case 0:   //extended
			{
				uint64_t length = 0;
				if(!rd_uleb(syms->data, syms->size, &at, &length)) return false;
				const long next = at + (long)length;
				uint8_t sub = 0;
				if(!rd8(syms->data, syms->size, at, &sub)) return false;
				at++;

				if(sub == 1)
				{
					//End of sequence: the rows at the last address cover up to here
					for(int rr = run_start; rr < syms->nrows; rr++)
						syms->rows[rr].end = addr;
					run_start = syms->nrows;
					run_addr = 0xFFFFFFFFu;

					addr = 0;
					file = 1;
					line = 1;
				}
				else if(sub == 2)
				{
					//set_address
					uint32_t set = 0;
					if(!rd32(syms->data, syms->size, at, &set)) return false;
					addr = set;
				}
				else if(sub == 3 && version < 5)
				{
					//define_file: a name followed by three numbers
					uint8_t u8 = 0;
					long start = at;
					for(;;)
					{
						if(!rd8(syms->data, syms->size, at, &u8)) return false;
						at++;
						if(u8 == 0) break;
					}
					long len = (at - 1) - start;
					char text[512];
					if(len >= (long)sizeof(text)) len = sizeof(text) - 1;
					memcpy(text, syms->data + start, (size_t)len);
					text[len] = 0;
					uint64_t a = 0, b = 0, c = 0;
					if(!rd_uleb(syms->data, syms->size, &at, &a)) return false;
					if(!rd_uleb(syms->data, syms->size, &at, &b)) return false;
					if(!rd_uleb(syms->data, syms->size, &at, &c)) return false;
					add_file(syms, text);
				}

				at = next;
				break;
			}

			default:
			{
				//set_column and anything newer than this reader knows: take its operands so that
				//the rest of the program still reads correctly
				uint64_t ignored = 0;
				for(int oo = 0; oo < (int)opcode_operands[op - 1]; oo++)
				{
					if(!rd_uleb(syms->data, syms->size, &at, &ignored))
						return false;
				}
				break;
			}
		}
	}

	//Anything left over at the end of the unit covers up to where it stopped
	for(int rr = run_start; rr < syms->nrows; rr++)
		syms->rows[rr].end = addr;

	return true;
}

static int cmp_row(const void *a, const void *b)
{
	const sym_row_t *ra = (const sym_row_t*)a;
	const sym_row_t *rb = (const sym_row_t*)b;
	if(ra->addr < rb->addr) return -1;
	if(ra->addr > rb->addr) return 1;
	return 0;
}

static int cmp_func(const void *a, const void *b)
{
	const sym_func_t *fa = (const sym_func_t*)a;
	const sym_func_t *fb = (const sym_func_t*)b;
	if(fa->addr < fb->addr) return -1;
	if(fa->addr > fb->addr) return 1;
	if(fa->size > fb->size) return -1;   //the biggest symbol at an address wins
	return 1;
}

symbols_t *symbols_load(const char *path)
{
	FILE *f = fopen(path, "rb");
	if(f == NULL)
		return NULL;

	symbols_t *syms = (symbols_t*)calloc(1, sizeof(symbols_t));
	if(syms == NULL)
	{
		fclose(f);
		return NULL;
	}

	fseek(f, 0, SEEK_END);
	syms->size = ftell(f);
	fseek(f, 0, SEEK_SET);
	if(syms->size <= 0)
	{
		fclose(f);
		free(syms);
		return NULL;
	}

	syms->data = (uint8_t*)malloc((size_t)syms->size);
	if(syms->data == NULL)
	{
		fclose(f);
		free(syms);
		return NULL;
	}
	syms->loaded = (long)fread(syms->data, 1, (size_t)syms->size, f);
	fclose(f);

	//Must be a 32-bit little-endian ARM executable
	if(syms->loaded < 52 || syms->data[0] != 0x7F || syms->data[1] != 'E' ||
		syms->data[2] != 'L' || syms->data[3] != 'F' ||
		syms->data[4] != 1 || syms->data[5] != 1)
	{
		symbols_free(syms);
		return NULL;
	}

	uint32_t shentsize = 0, shnum = 0, shstrndx = 0, shoff = 0;
	rd32(syms->data, syms->loaded, 32, &shoff);
	rd16(syms->data, syms->loaded, 46, (uint16_t*)&shentsize);
	rd16(syms->data, syms->loaded, 48, (uint16_t*)&shnum);
	rd16(syms->data, syms->loaded, 50, (uint16_t*)&shstrndx);
	if(shnum == 0 || shnum > 256 || shentsize < 40)
	{
		symbols_free(syms);
		return NULL;
	}

	uint32_t name_off = 0, sym_off = 0, sym_size = 0, sym_link = 0, sym_entsize = 0;
	long line_off = -1, line_size = 0;
	long line_str_off = -1, line_str_size = 0;
	long dbg_str_off = -1, dbg_str_size = 0;
	uint32_t shstr_off = 0, shstr_size = 0;

	//Section names first, so the sections can be found by name
	{
		uint32_t at = shoff + shstrndx * shentsize;
		rd32(syms->data, syms->loaded, at + 16, &shstr_off);
		rd32(syms->data, syms->loaded, at + 20, &shstr_size);
	}

	for(uint32_t ss = 0; ss < shnum; ss++)
	{
		const long at = shoff + ss * shentsize;
		uint32_t name = 0;
		if(!rd32(syms->data, syms->loaded, at, &name)) break;

		char sec[64] = "";
		if(name < shstr_size)
		{
			long start = shstr_off + name;
			long stop = start;
			while(stop < (long)(shstr_off + shstr_size) && syms->data[stop] != 0)
				stop++;
			long len = stop - start;
			if(len >= (long)sizeof(sec)) len = sizeof(sec) - 1;
			memcpy(sec, syms->data + start, (size_t)len);
			sec[len] = 0;
		}

		uint32_t off = 0, size = 0, link = 0, entsize = 0;
		rd32(syms->data, syms->loaded, at + 16, &off);
		rd32(syms->data, syms->loaded, at + 20, &size);
		rd32(syms->data, syms->loaded, at + 24, &link);
		rd32(syms->data, syms->loaded, at + 36, &entsize);

		if(strcmp(sec, ".symtab") == 0)
		{
			sym_off = off; sym_size = size; sym_link = link; sym_entsize = entsize;
		}
		else if(strcmp(sec, ".debug_line") == 0)
		{
			line_off = off; line_size = size;
		}
		else if(strcmp(sec, ".debug_line_str") == 0)
		{
			line_str_off = off; line_str_size = size;
		}
		else if(strcmp(sec, ".debug_str") == 0)
		{
			dbg_str_off = off; dbg_str_size = size;
		}
		(void)name_off;
	}

	//Function symbols
	if(sym_off != 0 && sym_entsize >= 16)
	{
		uint32_t str_off = 0, str_size = 0;
		{
			const long at = shoff + sym_link * shentsize;
			rd32(syms->data, syms->loaded, at + 16, &str_off);
			rd32(syms->data, syms->loaded, at + 20, &str_size);
		}

		const int count = (int)(sym_size / sym_entsize);
		syms->funcs = (sym_func_t*)calloc((size_t)(count > 0 ? count : 1), sizeof(sym_func_t));
		if(syms->funcs != NULL)
		{
			for(int ss = 0; ss < count; ss++)
			{
				const long at = sym_off + ss * (long)sym_entsize;
				uint32_t name = 0, value = 0, size = 0;
				uint8_t info = 0;
				if(!rd32(syms->data, syms->loaded, at, &name)) break;
				if(!rd32(syms->data, syms->loaded, at + 4, &value)) break;
				if(!rd32(syms->data, syms->loaded, at + 8, &size)) break;
				if(!rd8(syms->data, syms->loaded, at + 12, &info)) break;

				//STT_FUNC only, and only ones with a name and somewhere to be
				if((info & 0xF) != 2) continue;
				if(name == 0 || name >= str_size) continue;
				if(value == 0) continue;

				sym_func_t *fn = &(syms->funcs[syms->nfuncs++]);
				fn->addr = value;
				fn->size = size;
				fn->name = str_off + name;
			}

			qsort(syms->funcs, (size_t)syms->nfuncs, sizeof(sym_func_t), cmp_func);
		}
	}

	//Line table: a run of units, each with its own header
	if(line_off >= 0 && line_size > 0)
	{
		long at = line_off;
		const long end = line_off + line_size;
		while(at + 4 <= end)
		{
			uint32_t unit_length = 0;
			if(!rd32(syms->data, syms->loaded, at, &unit_length)) break;
			if(unit_length == 0 || unit_length == 0xFFFFFFFFu)
				break;   //nothing more, or a 64-bit unit this does not read
			const long unit_end = at + 4 + (long)unit_length;
			if(unit_end > end) break;

			//A unit that does not read cleanly leaves nothing behind: half a line table would
			//put names on the wrong addresses, which is worse than having none
			const int before = syms->nrows;
			if(!read_line_program(syms, at, unit_end,
				(line_str_off >= 0) ? line_str_off : 0, line_str_size,
				(dbg_str_off >= 0) ? dbg_str_off : 0, dbg_str_size))
				syms->nrows = before;

			at = unit_end;
		}

		if(syms->nrows > 0)
			qsort(syms->rows, (size_t)syms->nrows, sizeof(sym_row_t), cmp_row);
	}

	return syms;
}

void symbols_free(symbols_t *syms)
{
	if(syms == NULL) return;
	for(int ff = 0; ff < syms->nfiles; ff++)
		free(syms->files[ff]);
	free(syms->files);
	free(syms->rows);
	free(syms->funcs);
	free(syms->data);
	free(syms);
}

const char *symbols_func(symbols_t *syms, uint32_t addr, uint32_t *start_out, uint32_t *size_out)
{
	if(syms == NULL || syms->nfuncs == 0)
		return NULL;

	//The last function starting at or below the address, allowing for one that is still in
	//front of the address but big enough to contain it
	int lo = 0, hi = syms->nfuncs - 1, best = -1;
	while(lo <= hi)
	{
		const int mid = (lo + hi) / 2;
		if(syms->funcs[mid].addr <= addr)
		{
			best = mid;
			lo = mid + 1;
		}
		else
		{
			hi = mid - 1;
		}
	}
	if(best < 0) return NULL;

	const sym_func_t *fn = &(syms->funcs[best]);
	if(fn->size != 0 && addr >= fn->addr + fn->size)
		return NULL;

	if(start_out) *start_out = fn->addr;
	if(size_out) *size_out = fn->size;
	return (const char*)(syms->data + fn->name);
}

bool symbols_line(symbols_t *syms, uint32_t addr, const char **file_out, int *line_out)
{
	if(syms == NULL || syms->nrows == 0)
		return false;

	int lo = 0, hi = syms->nrows - 1, best = -1;
	while(lo <= hi)
	{
		const int mid = (lo + hi) / 2;
		if(syms->rows[mid].addr <= addr)
		{
			best = mid;
			lo = mid + 1;
		}
		else
		{
			hi = mid - 1;
		}
	}
	if(best < 0) return false;

	//Rows often repeat an address, once per line that the same instruction came from. A listing
	//shows the first of them.
	while(best > 0 && syms->rows[best - 1].addr == syms->rows[best].addr)
		best--;

	const sym_row_t *row = &(syms->rows[best]);
	if(addr >= row->end)
		return false;

	if(file_out)
	{
		const char *name = "";
		if(row->file > 0 && row->file < syms->nfiles)
			name = syms->files[row->file];
		*file_out = name;
	}
	if(line_out) *line_out = row->line;
	return true;
}

void symbols_stats(symbols_t *syms, int *functions, int *lines)
{
	if(functions) *functions = syms ? syms->nfuncs : 0;
	if(lines) *lines = syms ? syms->nrows : 0;
}
