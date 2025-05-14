//sysc.cpp
//Emulated system-calls in Neki32 simulator
//Bryan E. Topp <betopp@betopp.com> 2024

//Nemul, the Neki32 Simulator, Copyright 2025 Nekisoft Pty Ltd, ACN 680 583 251
//This program is free software: you can redistribute it and/or modify
//it under the terms of the GNU General Public License as published by
//the Free Software Foundation, either version 3 of the License, or
//(at your option) any later version.

#include "sysc.h"
#include "trace.h"
#include "prefs.h"
#include <unistd.h>
#include <stdlib.h>
#include <string.h>


//Process making the system call
static process_t *sysc_pptr;

//Host file used to service disk reads/writes
int sysc_diskfd = -1;

//Framebuffer last made active
int sysc_fb_now_pid;
int sysc_fb_now_ptr;
int sysc_fb_now_mode;

//Framebuffer yet to be made active
int sysc_fb_enq_pid;
int sysc_fb_enq_ptr;
int sysc_fb_enq_mode;

//Inputs waiting to be delivered
#define SYSC_INPUTQ_MAX (PREFS_PAD_MAX*2)
uint32_t sysc_inputq_data[SYSC_INPUTQ_MAX];
int sysc_inputq_rptr;
int sysc_inputq_wptr;

//Memory size needed for each mode of framebuffer
const int sysc_fb_sizes[] = 
{
	0, //Text
	640*480*2, //VGA RGB565
	320*240*2, //240p RGB565
};

void sysc_setdiskfd(int fd)
{
	sysc_diskfd = fd;
}

void sysc_popfbptr(uint16_t **bufptr_out, int *mode_out)
{	
	//Default if we don't have anything to display
	*bufptr_out = NULL;
	*mode_out = 0;
	
	//Enqueued framebuffer is now considered active when the emulator uses it
	sysc_fb_now_pid = sysc_fb_enq_pid;
	sysc_fb_now_ptr = sysc_fb_enq_ptr;
	sysc_fb_now_mode = sysc_fb_enq_mode;
	
	//Return the actual location in host memory
	for(int pp = 0; pp < PROCESS_MAX; pp++)
	{
		if(process_table[pp].pid != sysc_fb_now_pid)
			continue; //Wrong process
		
		if(process_table[pp].mem == NULL)
			return; //Process died while display active
		
		*bufptr_out = (uint16_t*)(process_table[pp].mem + (sysc_fb_now_ptr / 4));
		*mode_out = sysc_fb_now_mode;
		process_table[pp].unpaused = true;
		return;
	}
	
	//Didn't find process displaying something
	
}

void sysc_pushpads(const uint16_t *pads)
{	
	//Just reset the "input queue" each time
	for(int pp = 0; pp < PREFS_PAD_MAX; pp++)
	{
		sysc_inputq_data[pp] = pads[pp];
		sysc_inputq_data[pp] <<= 16;
		sysc_inputq_data[pp] |= 'A' + pp;
	}
	
	sysc_inputq_rptr = 0;
	sysc_inputq_wptr = PREFS_PAD_MAX;
	
	//Unpause all processes (whatever)
	for(int pp = 0; pp < PROCESS_MAX; pp++)
	{
		if(process_table[pp].state == PROCESS_STATE_ALIVE)
			process_table[pp].unpaused = true;
	}
}

void pvmk_sc_none(void)
{
	TRACE("%s\n", "pvmk_sc_none");
}

void pvmk_sc_pause(void)
{
	TRACE("%s\n", "pvmk_sc_pause");
	sysc_pptr->paused = true;
	return;
}

int pvmk_sc_getticks(void)
{
	TRACE("%s\n", "pvmk_sc_getticks");
	
	extern uint32_t EmulTimerTicks;
	return EmulTimerTicks & 0x7FFFFFFFu;
}

int pvmk_sc_fork(void)
{
	TRACE("%s\n", "pvmk_sc_fork");
	return process_fork(sysc_pptr->pid);
}

int pvmk_sc_wait(uint32_t idtype, uint32_t id, uint32_t options, uint32_t buf, uint32_t len)
{
	TRACE("%s %u %u %X %8.8X %u\n", "pvmk_sc_wait", idtype, id, options, buf, len);
	return 0;
}

void pvmk_sc_exit(uint32_t code, uint32_t sig)
{
	TRACE("%s %u %u\n", "pvmk_sc_exit", code, sig);
}

int pvmk_sc_gfx_flip(uint32_t mode, uint32_t buffer)
{
	TRACE("%s %u %8.8X\n", "pvmk_sc_gfx_flip", mode, buffer);
	
	//Validate mode parameter
	if(mode >= sizeof(sysc_fb_sizes)/sizeof(sysc_fb_sizes[0]))
		return -PVMK_EINVAL;
	
	if(mode == 0 && buffer != 0)
		return -PVMK_EINVAL;
	
	if(mode != 0 && buffer == 0)
		return -PVMK_EINVAL;
	
	//Validate that the buffer (given how big it is, in the mode) fits in the caller's address space
	if(buffer < 4096)
		return -PVMK_EFAULT;
	
	if(buffer + sysc_fb_sizes[mode] > sysc_pptr->size)
		return -PVMK_EFAULT;
	
	//Set aside these parameters for next time we "enter vertical blanking" (update the emulator display)
	sysc_fb_enq_pid = sysc_pptr->pid;
	sysc_fb_enq_ptr = buffer;
	sysc_fb_enq_mode = mode;
	
	if(sysc_fb_now_pid == sysc_pptr->pid)
	{
		//The caller currently has an image onscreen - tell them which one
		return sysc_fb_now_ptr;
	}
	else
	{
		//Someone else has an image onscreen
		return 0;
	}
}

int pvmk_sc_mem_sbrk(uint32_t req)
{
	TRACE("%s %u\n", "pvmk_sc_mem_sbrk", req);
	
	//Compute new size, cap like real system
	uint32_t will_be = sysc_pptr->size + req;
	if(will_be < sysc_pptr->size)
	{
		TRACE("%s", "Overflow in process size; cannot make process smaller.\n");
		return -PVMK_EINVAL;
	}
	
	if(will_be > 24*1024*1024)
	{
		TRACE("%s", "Requested more memory than 24MBytes.\n");
		return -PVMK_ENOMEM;
	}
	
	//Try to reallocate the user memory
	void *result = realloc(sysc_pptr->mem, will_be);
	if(result == NULL)
	{
		//Host didn't have enough memory for it
		TRACE("%s", "Host is out of memory!\n");
		return -PVMK_ENOMEM;
	}
	
	//Clear the new memory
	memset((char*)result + sysc_pptr->size, 0x00, req);
	
	//Sized the process up successfully. Store new info, return old end-of-process
	uint32_t retval = sysc_pptr->size;
	sysc_pptr->mem = (uint32_t*)result;
	sysc_pptr->size = will_be;
	TRACE("Resized process %d from %8.8X to %8.8X\n", sysc_pptr->pid, retval, sysc_pptr->size);
	return retval;
}

int pvmk_sc_input(uint32_t buf, uint32_t each, uint32_t total)
{
	TRACE("%s %8.8X %u %u\n", "pvmk_sc_input", buf, each, total);
	
	if( (buf < 4096) || ((buf+total) > sysc_pptr->size) )
	{
		//Buffer out of bounds
		return -PVMK_EFAULT;
	}
	
	if( (buf % 4) || (each % 4) || (total % 4) )
	{
		//Misaligned
		return -PVMK_EINVAL;
	}
	
	if(each < 4)
	{
		//Not enough space for the smallest events
		return -PVMK_EINVAL;
	}
	
	int nread = 0;
	while( (sysc_inputq_wptr != sysc_inputq_rptr) && (total >= each) )
	{
		sysc_pptr->mem[buf/4] = sysc_inputq_data[sysc_inputq_rptr];
		
		sysc_inputq_rptr = (sysc_inputq_rptr + 1) % SYSC_INPUTQ_MAX;		
		buf += each;
		total -= each;
		nread++;
	}
	
	if(nread == 0)
		return -PVMK_EAGAIN;
	
	return nread;
}

int pvmk_sc_snd_play(uint32_t mode, uint32_t chunk, uint32_t chunkbytes, uint32_t maxbuf)
{
	TRACE("%s %u %8.8X %u %u\n", "pvmk_sc_snd_play", mode, chunk, chunkbytes, maxbuf);
	
	return -PVMK_ENOSYS;
}

int pvmk_sc_nvm_save(uint32_t buf, uint32_t len)
{
	TRACE("%s %8.8X %u\n", "pvmk_sc_nvm_save", buf, len);
	
	return -PVMK_ENOSYS;
}

int pvmk_sc_nvm_load(uint32_t buf, uint32_t len)
{
	TRACE("%s %8.8X %u\n", "pvmk_sc_nvm_load", buf, len);
	
	return -PVMK_ENOSYS;
}

int pvmk_sc_env_save(uint32_t buf, uint32_t len)
{
	TRACE("%s %8.8X %u\n", "pvmk_sc_env_save", buf, len);
	
	if(buf == 0 && len == 0)
	{
		sysc_pptr->env_len = 0;
		return 0;
	}

	if(len == 0)
		return 0;

	if(buf < 4096 || buf + len > sysc_pptr->size)
		return -PVMK_EFAULT;
	
	if(len > sizeof(sysc_pptr->env_buf))
		return -PVMK_EINVAL;
	
	if(sysc_pptr->env_len + len > sizeof(sysc_pptr->env_buf))
		len = sizeof(sysc_pptr->env_buf) - sysc_pptr->env_len;
	
	
	memcpy( sysc_pptr->env_buf + sysc_pptr->env_len, ((char*)(sysc_pptr->mem)) + buf, len);
	sysc_pptr->env_len += len;
	return len;
}

int pvmk_sc_env_load(uint32_t buf, uint32_t len)
{
	TRACE("%s %8.8X %u\n", "pvmk_sc_env_load", buf, len);
	
	if(len > (unsigned)sysc_pptr->env_len)
		len = sysc_pptr->env_len;
	
	if(buf < 4096 || buf + len > sysc_pptr->size)
		return -PVMK_EFAULT;
	
	memcpy( ((char*)(sysc_pptr->mem)) + buf, sysc_pptr->env_buf, len);
	return len;
}

int pvmk_sc_sig_mask(uint32_t mask, uint32_t how)
{
	TRACE("%s %8.8X %u\n", "pvmk_sc_sig_mask", mask, how);
	
	return -PVMK_ENOSYS;
}

void pvmk_sc_sig_return(void)
{
	TRACE("%s\n", "pvmk_sc_sig_return");
}

int pvmk_sc_disk_read2k(uint32_t sector_num, uint32_t buf, uint32_t nsectors)
{
	TRACE("%s %u %8.8X %u\n", "pvmk_sc_disk_read2k", sector_num, buf, nsectors);
	
	if(sysc_diskfd < 0)
		return -PVMK_ENXIO;
	
	if(buf + (2048 * nsectors) > sysc_pptr->size)
		return -PVMK_EFAULT;
	if(buf < 4096)
		return -PVMK_EFAULT;
	if(buf % 4)
		return -PVMK_EFAULT;
	
	off_t seeked = lseek(sysc_diskfd, sector_num * 2048ull, SEEK_SET);
	if(seeked != sector_num * 2048ull)
		return -PVMK_ENOSPC;
	
	int nread = read(sysc_diskfd, &(sysc_pptr->mem[buf/4]), 2048 * nsectors);
	if(nread != nsectors * 2048ull)
		return -PVMK_ENOSPC;
	
	return 0;
}

int pvmk_sc_disk_write2k(uint32_t sector_num, uint32_t buf, uint32_t nsectors)
{
	TRACE("%s %u %8.8X %u\n", "pvmk_sc_disk_write2k", sector_num, buf, nsectors);
	
	if(sysc_diskfd < 0)
		return -PVMK_ENXIO;
	
	if(buf + (2048 * nsectors) > sysc_pptr->size)
		return -PVMK_EFAULT;
	if(buf < 4096)
		return -PVMK_EFAULT;
	if(buf % 4)
		return -PVMK_EFAULT;
	
	off_t seeked = lseek(sysc_diskfd, sector_num * 2048ull, SEEK_SET);
	if(seeked != sector_num * 2048ull)
		return -PVMK_ENOSPC;
	
	int nwritten = write(sysc_diskfd, &(sysc_pptr->mem[buf/4]), 2048 * nsectors);
	if(nwritten != nsectors * 2048ull)
		return -PVMK_ENOSPC;
	
	return 0;	
}

int pvmk_sc_mexec_append(uint32_t buf, uint32_t len)
{
	TRACE("%s %8.8X %u\n", "pvmk_sc_mexec_append", buf, len);

	//Check for "reset" parameters
	if(buf == 0 && len == 0)
	{
		//Reset pending image
		sysc_pptr->mexec_size = 0;
		if(sysc_pptr->mexec_mem != NULL)
		{
			free(sysc_pptr->mexec_mem);
			sysc_pptr->mexec_mem = NULL;
		}
		return 0;
	}
	
	//Validate resulting image size
	if(sysc_pptr->mexec_size >= 24*1024*1024)
	{
		//Already too long
		return 0;
	}
	
	if(sysc_pptr->mexec_size + len > 24*1024*1024)
	{
		//Overlong after adding to it... truncate how much we add
		len = (24*1024*1024) - sysc_pptr->mexec_size;
	}
	
	//Validate incoming buffer (NULL pointer means "zero-fill the memory")
	if(buf != 0)
	{
		if(buf < 4096)
			return -PVMK_EFAULT;
		if(buf + len > sysc_pptr->size)
			return -PVMK_EFAULT;
	}
	
	const char *src = ((const char*)(sysc_pptr->mem)) + buf;
	
	//Resize pending data region on host
	uint32_t oldsize = sysc_pptr->mexec_size;
	sysc_pptr->mexec_size += len;
	
	if(sysc_pptr->mexec_mem == NULL)
		sysc_pptr->mexec_mem = (char*)calloc(sysc_pptr->mexec_size, 1);
	else
		sysc_pptr->mexec_mem = (char*)realloc(sysc_pptr->mexec_mem, sysc_pptr->mexec_size);
	
	if(sysc_pptr->mexec_mem == NULL)
	{
		//Failed to allocate enough memory on host
		return -PVMK_ENOMEM;
	}
	
	//Copy data in
	if(buf == 0)
	{
		//Zero-fill
		memset(sysc_pptr->mexec_mem + oldsize, 0, len);
	}
	else
	{
		//Copy in the data provided
		memcpy(sysc_pptr->mexec_mem + oldsize, src, len);
	}

	//Successfully appended
	return len;
}

void pvmk_sc_mexec_apply(void)
{
	TRACE("%s\n", "pvmk_sc_mexec_apply");
	
	//Mask all signals
	//sysc_pptr->
	
	//Swap existing process image for new one
	if(sysc_pptr->mem != NULL)
	{
		free(sysc_pptr->mem);
		sysc_pptr->mem = NULL;
	}
	
	sysc_pptr->mem = (uint32_t*)(sysc_pptr->mexec_mem);
	sysc_pptr->size = sysc_pptr->mexec_size;
	
	sysc_pptr->mexec_mem = NULL;
	sysc_pptr->mexec_size = 0;
	
	if(sysc_pptr->mem == NULL)
	{
		TRACE("Process %d killed itself by mexec'ing with no pending image\n", sysc_pptr->pid);
		sysc_pptr->state = PROCESS_STATE_DEAD;
	}
	
	//Reset CPU regs
	memset(sysc_pptr->regs, 0, sizeof(sysc_pptr->regs));
	sysc_pptr->regs[15] = 0x1000;
	sysc_pptr->cpsr = 0;
	
}

int pvmk_sc_print(uint32_t buf_ptr)
{
	TRACE("%s %8.8X\n", "pvmk_sc_print", buf_ptr);
	
	//Print characters until we reach a NUL terminator or run afoul of memory bounds
	int printed = 0;
	while(1)
	{
		if(buf_ptr < 0x1000 || buf_ptr >= sysc_pptr->size)
		{
			TRACE("Bad address %8.8X in _sc_print\n", buf_ptr);
			return (printed > 0) ? printed : -PVMK_EFAULT;
		}
		
		unsigned char to_print = sysc_pptr->mem[buf_ptr/4] >> (8 * (buf_ptr%4));
		if(to_print == 0)
		{
			//End of string, correctly
			break;
		}
		
		TRACE("%c", to_print);
		buf_ptr++;
	}		
	return printed;
}

void sysc(process_t *pptr)
{
	TRACE("Handling system-call %X from process %d\n", pptr->regs[0], pptr->pid);
	uint32_t *regs = pptr->regs;
	for(int pp = 1; pp < 6; pp++)
	{
		TRACE("\tParm %d: %8.8X\n", pp, regs[pp]);
	}
	
	sysc_pptr = pptr;
	int result = 0;
	switch(regs[0])
	{
		case 0x00: /*void*/ pvmk_sc_none(); break;
		case 0x01: /*void*/ pvmk_sc_pause(); break;
		case 0x02: result = pvmk_sc_getticks(); break;
		case 0x05: result = pvmk_sc_fork(); break;
		case 0x06: result = pvmk_sc_wait(regs[1], regs[2], regs[3], regs[4], regs[5]); break;
		case 0x07: /* nr */ pvmk_sc_exit(regs[1], regs[2]); break;
		case 0x08: result = pvmk_sc_env_save(regs[1], regs[2]); break;
		case 0x09: result = pvmk_sc_env_load(regs[1], regs[2]); break;
		case 0x20: result = pvmk_sc_sig_mask(regs[1], regs[2]); break;
		case 0x22: /* nr */ pvmk_sc_sig_return(); break;
		case 0x30: result = pvmk_sc_gfx_flip(regs[1], regs[2]); break;
		case 0x40: result = pvmk_sc_mem_sbrk(regs[1]); break;
		case 0x50: result = pvmk_sc_input(regs[1], regs[2], regs[3]); break;
		case 0x60: result = pvmk_sc_snd_play(regs[1], regs[2], regs[3], regs[4]); break;
		case 0x81: result = pvmk_sc_nvm_save(regs[1], regs[2]); break;
		case 0x82: result = pvmk_sc_nvm_load(regs[1], regs[2]); break;
		case 0x91: result = pvmk_sc_disk_read2k(regs[1], regs[2], regs[3]); break;
		case 0x92: result = pvmk_sc_disk_write2k(regs[1], regs[2], regs[3]); break;
		case 0xA1: result = pvmk_sc_mexec_append(regs[1], regs[2]); break;
		case 0xA2: /* nr */ pvmk_sc_mexec_apply(); break;
		case 0xB0: result = pvmk_sc_print(regs[1]); break;
		default:   result = -PVMK_ENOSYS; TRACE("Bad syscall 0x%X\n", regs[0]); break;
	}
	
	TRACE("Returning r0=%8.8X from syscall\n", result);
	regs[0] = result;
	return;
}