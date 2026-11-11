//memmodel.h
//Timing model of the F1C200s memory system: ARM926EJ-S caches and the SiP DDR1,
//calibrated against measurements taken on the real board with
//kernel/ports/f1c/bench (see memmodel.cpp for every constant and its provenance).
//
//Compiled in only when NEMUL_TIMING is defined. The fast build gets inline
//no-ops, so nothing here costs it anything - not a branch, not a call.
//Bryan E. Topp <betopp@betopp.com> 2026

#ifndef MEMMODEL_H
#define MEMMODEL_H

#include <stdint.h>

#ifdef NEMUL_TIMING

//Start of a run: empty caches, closed rows, zero cycle count
void mem_model_reset(void);

//Select which address space the next accesses belong to. The caches are VIVT, so
//two processes at the same virtual address do not share lines; the kernel reaches
//the same bytes through the same FCSE-translated address, which this reproduces
//by keying on (pid << 25) | va for addresses below the 32MByte FCSE window.
void mem_model_process(uint32_t pid);

//One instruction fetch at this virtual address, and one data access. Both may
//add a cache miss plus DRAM time to the cycle count.
void mem_model_fetch(uint32_t va);
void mem_model_load(uint32_t va, uint32_t bytes);
void mem_model_store(uint32_t va, uint32_t bytes);

//Issue cost of one instruction: the PC it started at, the PC it ended at (which
//is how a taken branch is recognised without evaluating conditions) and the
//instruction word, which is decoded here for the classes that cost more than one
//cycle and for the load-use interlock.
void mem_model_issue(uint32_t pc, uint32_t next_pc, uint32_t ir);

//Cycles consumed so far, and a way to charge time the model cannot see
//(system calls, the display engine's scanout, idle).
uint64_t mem_model_cycles(void);
void mem_model_add_cycles(uint64_t c);

//Tell the model how many bytes the display engine reads out of DRAM per frame. Zero
//means the display is off. The engine competes for the same bus the CPU uses, so
//fills get slower while it is scanning.
void mem_model_set_scanout(uint32_t bytes_per_frame);

//What the model actually did: cache misses, DRAM fills, writebacks and row misses.
//Printed so a predicted time can be audited against what the program did, instead
//of being a number nobody can check.
struct mem_model_stats_t
{
	uint64_t icache_misses;
	uint64_t dcache_read_misses;
	uint64_t dcache_write_misses;
	uint64_t writebacks;
	uint64_t row_misses;
	uint64_t fills;
	uint64_t instructions;
	uint64_t issue_cycles;
	uint64_t fill_cycles;
	uint64_t stall_cycles;
	uint64_t branch_cycles;
	uint64_t interlock_cycles;
	uint64_t taken_branches;
};
void mem_model_stats(struct mem_model_stats_t *out);

//Convert a cycle count to microseconds at the modelled clock.
uint64_t mem_model_cycles_to_us(uint64_t cycles);

#else

#define mem_model_reset() ((void)0)
#define mem_model_process(pid) ((void)0)
#define mem_model_fetch(va) ((void)0)
#define mem_model_load(va, bytes) ((void)0)
#define mem_model_store(va, bytes) ((void)0)
#define mem_model_issue(pc, next_pc, ir) ((void)0)
#define mem_model_cycles() ((uint64_t)0)
#define mem_model_add_cycles(c) ((void)0)
#define mem_model_set_scanout(b) ((void)0)
#define mem_model_cycles_to_us(c) ((uint64_t)0)

#endif //NEMUL_TIMING

#endif //MEMMODEL_H
