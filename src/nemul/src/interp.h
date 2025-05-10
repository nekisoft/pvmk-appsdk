//interp.h
//ARM interpreter
//Bryan E. Topp <betopp@betopp.com> 2025
#ifndef INTERP_H
#define INTERP_H

#include <stdint.h>
#include <stddef.h>

//Possible results of running the interpreter
typedef enum interp_result_e
{
	INTERP_RESULT_OK = 0, //All good, instruction happened without a problem
	INTERP_RESULT_AC, //Alignment-check failure
	INTERP_RESULT_ABT, //Data abort (access to bad data memory)
	INTERP_RESULT_PF, //Prefetch abort (access to bad instruction memory)
	INTERP_RESULT_SYSCALL, //System call triggered
	INTERP_RESULT_BKPT, //GDB breakpoint hit
	INTERP_RESULT_FATAL, //Something so horrible happened that we can't keep emulating
	INTERP_RESULT_MAX //Number of valid interpreter results
} interp_result_t;

//Runs ARM interpreter on the given CPU and memory image
//Returns what happened
interp_result_t interp_step(uint32_t *regs, uint32_t *cpsr, uint32_t *mem, size_t memsz);

//Runs ARM interpreter but forces the instruction register to be the given instruction
interp_result_t interp_step_force(uint32_t *regs, uint32_t *cpsr, uint32_t *mem, size_t memsz, uint32_t ir);

#endif //INTERP_H

