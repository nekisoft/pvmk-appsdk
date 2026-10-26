//prof.cpp
//Instruction-attribution profiler for the Neki32 simulator.
//
//Nemul, the Neki32 Simulator, Copyright 2025 Nekisoft Pty Ltd, ACN 680 583 251
//This program is free software: you can redistribute it and/or modify
//it under the terms of the GNU General Public License as published by
//the Free Software Foundation, either version 3 of the License, or
//(at your option) any later version.
//
//Why a counter per program counter rather than a sampling profiler: the emulator has no
//timing model at all, so the only honest measurement it can make is how many times each
//instruction ran. Sampling the interpreter's own clock measures the host, not the emulated
//machine, and a sampling profiler would need the emulated time to advance in a realistic
//way, which is exactly what is missing.
//
//A dense array per process, indexed by pc/4, is 25 MB for the machine's 24 MByte address
//space. That is affordable once, and it makes the recording side one increment with no
//hashing and no branch prediction surprises in the hottest loop the emulator has.
//
//The histogram is deliberately unweighted. Deciding what an instruction costs is a model,
//and a model belongs in the program that reads the histogram, where it can be changed and
//argued about without rebuilding the emulator. profan.py does that: it decodes each counted
//instruction and applies a cycle model to it.

#include "prof.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "process.h"

int prof_enabled = 0;

static char prof_prefix[1024];
static int prof_configured = 0;
static long prof_from = 0;
static long prof_to = 0x7FFFFFFF;
static long prof_now = 0;

static uint32_t *prof_counts[PROCESS_MAX];
static uint32_t prof_words[PROCESS_MAX];
static uint64_t prof_hits[PROCESS_MAX];
static uint64_t prof_total = 0;
static uint32_t prof_sysc[256];
static uint64_t prof_sysc_total = 0;

static void prof_configure(void)
{
	prof_configured = 1;

	const char *prefix = getenv("NEMUL_PROF");
	if(prefix == NULL || prefix[0] == '\0')
	{
		prof_prefix[0] = '\0';
		return;
	}
	snprintf(prof_prefix, sizeof(prof_prefix), "%s", prefix);

	const char *from = getenv("NEMUL_PROF_FROM");
	if(from != NULL)
		prof_from = atol(from);

	const char *to = getenv("NEMUL_PROF_TO");
	if(to != NULL)
		prof_to = atol(to);
}

void prof_tick(long ms, int quiet)
{
	if(!prof_configured)
		prof_configure();

	if(prof_prefix[0] == '\0')
	{
		prof_enabled = 0;
		return;
	}

	prof_now = ms;

	int was = prof_enabled;
	prof_enabled = (ms >= prof_from && ms < prof_to) ? 1 : 0;

	if(prof_enabled && !was && !quiet)
		fprintf(stderr, "prof: recording from %ld ms\n", ms);
	if(!prof_enabled && was && !quiet)
		fprintf(stderr, "prof: stopped recording at %ld ms, %llu instructions\n",
			ms, (unsigned long long)prof_total);
}

void prof_count(int idx, uint32_t pc)
{
	if(idx < 0 || idx >= PROCESS_MAX)
		return;

	if(prof_counts[idx] == NULL)
	{
		//Size comes from the process so that a small test program does not cost 25 MB
		uint32_t size = process_table[idx].size;
		if(size == 0)
			return;
		prof_words[idx] = size / 4;
		prof_counts[idx] = (uint32_t*)calloc(prof_words[idx], sizeof(uint32_t));
		if(prof_counts[idx] == NULL)
		{
			fprintf(stderr, "prof: cannot allocate %u counters for pid %d\n",
				prof_words[idx], process_table[idx].pid);
			prof_words[idx] = 0;
			return;
		}
	}

	uint32_t at = pc / 4;
	if(at >= prof_words[idx])
		return;

	prof_counts[idx][at]++;
	prof_hits[idx]++;
	prof_total++;
}

void prof_sysc_hit(uint32_t nr)
{
	//Windowed like the histogram, so the two line up: a flip count from the whole run
	//dividing a window's instructions would be a different number every time the window
	//moved, which is exactly the mistake this profiler exists to avoid.
	if(!prof_enabled)
		return;

	if(nr < 256)
		prof_sysc[nr]++;
	prof_sysc_total++;
}

//System-call numbers, so the dump is readable without the header
static const char *sysc_name(uint32_t nr)
{
	switch(nr)
	{
		case 0x00: return "none";
		case 0x01: return "pause";
		case 0x02: return "getticks";
		case 0x05: return "fork";
		case 0x06: return "wait";
		case 0x07: return "exit";
		case 0x08: return "env_save";
		case 0x09: return "env_load";
		case 0x20: return "sig_mask";
		case 0x22: return "sig_return";
		case 0x30: return "gfx_flip";
		case 0x40: return "mem_sbrk";
		case 0x50: return "input";
		case 0x60: return "snd_play";
		case 0x81: return "nvm_save";
		case 0x82: return "nvm_load";
		case 0x91: return "disk_read2k";
		case 0x92: return "disk_write2k";
		case 0x93: return "cardprobe";
		case 0xA1: return "mexec_append";
		case 0xA2: return "mexec_apply";
		case 0xB0: return "print";
		default:   return "?";
	}
}

void prof_dump(void)
{
	if(prof_prefix[0] == '\0')
		return;

	for(int idx = 0; idx < PROCESS_MAX; idx++)
	{
		if(prof_counts[idx] == NULL || prof_hits[idx] == 0)
			continue;

		char path[1200];
		snprintf(path, sizeof(path), "%s-%d.txt", prof_prefix, process_table[idx].pid);
		FILE *out = fopen(path, "w");
		if(out == NULL)
		{
			fprintf(stderr, "prof: cannot write %s\n", path);
			continue;
		}

		//Header lines start with '#', so the reader can skip them in one rule
		fprintf(out, "#nemul instruction histogram\n");
		fprintf(out, "#pid %d\n", process_table[idx].pid);
		fprintf(out, "#from %ld\n", prof_from);
		fprintf(out, "#to %ld\n", (prof_to == 0x7FFFFFFF) ? prof_now : prof_to);
		fprintf(out, "#hits %llu\n", (unsigned long long)prof_hits[idx]);
		fprintf(out, "#syscalls %llu\n", (unsigned long long)prof_sysc_total);
		fprintf(out, "#\n");

		for(uint32_t at = 0; at < prof_words[idx]; at++)
		{
			if(prof_counts[idx][at] != 0)
				fprintf(out, "%8.8X %u\n", at * 4, prof_counts[idx][at]);
		}

		fclose(out);
		fprintf(stderr, "prof: wrote %s, %llu instructions for pid %d\n",
			path, (unsigned long long)prof_hits[idx], process_table[idx].pid);
	}

	//System-call totals, once, next to the histograms
	if(prof_sysc_total != 0)
	{
		char path[1200];
		snprintf(path, sizeof(path), "%s-sysc.txt", prof_prefix);
		FILE *out = fopen(path, "w");
		if(out != NULL)
		{
			for(uint32_t nr = 0; nr < 256; nr++)
			{
				if(prof_sysc[nr] != 0)
					fprintf(out, "%2.2X %-14s %u\n", nr, sysc_name(nr), prof_sysc[nr]);
			}
			fprintf(out, "-- %-14s %llu\n", "total", (unsigned long long)prof_sysc_total);
			fclose(out);
			fprintf(stderr, "prof: wrote %s\n", path);
		}
	}
}
