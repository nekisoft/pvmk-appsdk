//sysc.cpp
//Emulated system-calls in Neki32 simulator
//Bryan E. Topp <betopp@betopp.com> 2024

//Nemul, the Neki32 Simulator, Copyright 2025 Nekisoft Pty Ltd, ACN 680 583 251
//This program is free software: you can redistribute it and/or modify
//it under the terms of the GNU General Public License as published by
//the Free Software Foundation, either version 3 of the License, or
//(at your option) any later version.

#define FILE_TRACE_CAT TRACE_CAT_SYSC
#include "trace.h"


#include "sysc.h"
#include "prefs.h"
#include "prof.h"
#include "rsp.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>


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

//Card-path logging, for watching the boot path talk to the emulated card.
//The GUI's trace ring is for reading back in a window and is not much use from a script, so
//this prints straight to stderr when NEMUL_LOG_CARD is set, and stops after a few thousand
//lines so a long run does not bury everything else.
static int sysc_logcard_left = 3000;
static int sysc_logcard_on = -1;
static void sysc_logcard(const char *fmt, ...)
{
	if(sysc_logcard_on < 0)
		sysc_logcard_on = (getenv("NEMUL_LOG_CARD") != NULL) ? 1 : 0;
	if(!sysc_logcard_on || sysc_logcard_left <= 0)
		return;
	sysc_logcard_left--;

	va_list args;
	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	va_end(args);
}

//Virtual card for _sc_cardprobe.
//Nemul has no card, so it answers from the same record sector the inspector writes on real
//hardware: twenty-one 96-byte records, one per command, exactly as cardinspect_sim.py makes them
//from a capture. That means a card image built by mkanticopy for a given card can be booted
//here, and the answers the emulated boot path gets are byte for byte the ones that card gave.
//Flip one CID byte in the file and this is the same data in a different card's slot, which is
//the copy case.
static unsigned char sysc_cardrecs[2048];
static int sysc_cardrecs_valid = 0;

int sysc_setcardrecords(const char *path)
{
	sysc_cardrecs_valid = 0;

	FILE *recfile = fopen(path, "rb");
	if(recfile == NULL)
		return -1;

	//A whole card image is acceptable too, since the records live at LBA 18 on it
	int got = 0;
	if(fseek(recfile, 0, SEEK_END) == 0 && ftell(recfile) == 2048)
	{
		rewind(recfile);
		got = fread(sysc_cardrecs, 1, sizeof(sysc_cardrecs), recfile);
	}
	else
	{
		if(fseek(recfile, 18 * 2048, SEEK_SET) == 0)
			got = fread(sysc_cardrecs, 1, sizeof(sysc_cardrecs), recfile);
	}
	fclose(recfile);

	if(got != (int)sizeof(sysc_cardrecs))
		return -1;
	if(memcmp(sysc_cardrecs, "PVMKINSP", 8) != 0)
		return -1;
	if(sysc_cardrecs[8] != 1)
		return -1;

	int count = sysc_cardrecs[10] | (sysc_cardrecs[11] << 8);
	if(count < 1 || count > 21)
		return -1;

	sysc_cardrecs_valid = 1;
	return 0;
}

//Layout of _sc_cardprobe_t as the emulated ARM sees it: uint64_t then six 32-bit fields,
//32 bytes. The host compiler would lay it out differently, so the fields are read by offset.
#define CARDPROBE_REQ_SIZE  32
#define CARDPROBE_OFF_CMD    0
#define CARDPROBE_OFF_RDAT  16
#define CARDPROBE_OFF_RLEN  20
#define CARDPROBE_OFF_RESP  24
#define CARDPROBE_OFF_RESPLEN 28

//Record layout, as inspector.h has it
#define CARD_REC_LEN        96
#define CARD_REC_OFF_INDEX   0
#define CARD_REC_OFF_RESPLEN 2
#define CARD_REC_OFF_DATALEN 3
#define CARD_REC_OFF_RESULT  8
#define CARD_REC_OFF_RESP   10
#define CARD_REC_OFF_DATA   27
#define CARD_HDR_LEN        32

//Probe flags, mirroring _SC_CARDPROBE_* in the kernel
#define CARD_FLAG_WDATA     (1ull << 53)
#define CARD_FLAG_RDATA     (1ull << 54)

static uint32_t cardprobe_rd32(uint32_t byteaddr)
{
	return sysc_pptr->mem[byteaddr / 4];
}

static void cardprobe_wr8(uint32_t byteaddr, unsigned char val)
{
	uint32_t *word = &(sysc_pptr->mem[byteaddr / 4]);
	*word = (*word & ~(0xFFu << (8 * (byteaddr % 4)))) | ((uint32_t)val << (8 * (byteaddr % 4)));
}

int pvmk_sc_cardprobe(uint32_t args_ptr)
{
	TDEBUG("%s %8.8X\n", "pvmk_sc_cardprobe", args_ptr);
	sysc_logcard("cardprobe: called with args %8.8X, process size %u\n", args_ptr, sysc_pptr->size);

	if(args_ptr == 0 || (args_ptr % 4) != 0)
		return -PVMK_EINVAL;
	if(args_ptr + CARDPROBE_REQ_SIZE > sysc_pptr->size)
		return -PVMK_EFAULT;

	uint64_t cmd = (uint64_t)cardprobe_rd32(args_ptr + CARDPROBE_OFF_CMD)
		| ((uint64_t)cardprobe_rd32(args_ptr + CARDPROBE_OFF_CMD + 4) << 32);

	uint32_t index = (uint32_t)((cmd >> 40) & 0x3Fu);
	uint32_t resp_ptr = cardprobe_rd32(args_ptr + CARDPROBE_OFF_RESP);
	int32_t resplen = (int32_t)cardprobe_rd32(args_ptr + CARDPROBE_OFF_RESPLEN);
	uint32_t rdat_ptr = cardprobe_rd32(args_ptr + CARDPROBE_OFF_RDAT);
	int32_t rlen = (int32_t)cardprobe_rd32(args_ptr + CARDPROBE_OFF_RLEN);

	//Data-out is not simulated: nothing in the card scheme needs it, and half-doing it would
	//be worse than refusing it, the same way the kernel port does.
	if(cmd & CARD_FLAG_WDATA)
		return -PVMK_EINVAL;

	if(!sysc_cardrecs_valid)
		return -PVMK_ENXIO;

	if(resplen < 0 || (resp_ptr != 0 && resp_ptr + (uint32_t)resplen > sysc_pptr->size))
		return -PVMK_EFAULT;
	if(rlen < 0 || (rdat_ptr != 0 && rdat_ptr + (uint32_t)rlen > sysc_pptr->size))
		return -PVMK_EFAULT;

	//Find this command's record. A command with no record is one this card never answered,
	//which on real hardware reads as silence, so it returns zero bytes here too.
	int count = sysc_cardrecs[10] | (sysc_cardrecs[11] << 8);
	const unsigned char *rec = NULL;
	for(int rr = 0; rr < count; rr++)
	{
		const unsigned char *ent = sysc_cardrecs + CARD_HDR_LEN + (CARD_REC_LEN * rr);
		if(ent[CARD_REC_OFF_INDEX] == (unsigned char)index)
		{
			rec = ent;
			break;
		}
	}

	sysc_logcard("cardprobe: CMD%u index=%u resp_ptr=%8.8X resplen=%d%s\n",
		index, index, resp_ptr, (int)resplen, rec ? "" : " (no record - silent)");

	if(rec == NULL)
		return 0;

	//A command that failed when it was recorded fails the same way now
	int16_t recorded = (int16_t)(rec[CARD_REC_OFF_RESULT] | (rec[CARD_REC_OFF_RESULT + 1] << 8));
	if(recorded < 0)
		return 0;
	if(rec[CARD_REC_OFF_RESPLEN] < 5)
		return 0;

	int tocopy = rec[CARD_REC_OFF_RESPLEN];
	if(tocopy > resplen)
		tocopy = resplen;
	if(resp_ptr != 0)
	{
		for(int bb = 0; bb < tocopy; bb++)
			cardprobe_wr8(resp_ptr + (uint32_t)bb, rec[CARD_REC_OFF_RESP + bb]);
	}

	//A command with a data block gives the recorded one back
	if((cmd & CARD_FLAG_RDATA) && rdat_ptr != 0 && rlen > 0)
	{
		int datalen = rec[CARD_REC_OFF_DATALEN];
		if(datalen > rlen)
			datalen = rlen;
		for(int bb = 0; bb < datalen; bb++)
			cardprobe_wr8(rdat_ptr + (uint32_t)bb, rec[CARD_REC_OFF_DATA + bb]);
		if(datalen == 0)
			return 0; //Recorded without any data, so the block is empty
	}

	sysc_logcard("cardprobe: CMD%u -> %d bytes, first %2.2X %2.2X %2.2X %2.2X\n",
		index, tocopy, rec[CARD_REC_OFF_RESP + 1], rec[CARD_REC_OFF_RESP + 2],
		rec[CARD_REC_OFF_RESP + 3], rec[CARD_REC_OFF_RESP + 4]);

	return tocopy;
}

//Boots a given NNE image as PID1 instead of the embedded init. Used by the console front end
//so a single program can be run without the boot menu and the rest of init in the way.
int sysc_bootimage(const char *path)
{
	FILE *imgfile = fopen(path, "rb");
	if(imgfile == NULL)
		return -1;

	if(fseek(imgfile, 0, SEEK_END) != 0)
	{
		fclose(imgfile);
		return -1;
	}
	long imglen = ftell(imgfile);
	if(imglen <= 0 || imglen > 24 * 1024 * 1024)
	{
		fclose(imgfile);
		return -1;
	}
	rewind(imgfile);

	//The whole user address space, the way the machine layer maps it, not just the image.
	//crt0 puts the stack near the top of that space regardless of how big the program is, so a
	//process sized to its image alone takes a data abort on its first push.
	#define PROCESS_SPACE (24 * 1024 * 1024)
	uint32_t *imgmem = (uint32_t*)calloc(1, PROCESS_SPACE);
	if(imgmem == NULL)
	{
		fclose(imgfile);
		return -1;
	}

	if(fread(imgmem, 1, (size_t)imglen, imgfile) != (size_t)imglen)
	{
		free(imgmem);
		fclose(imgfile);
		return -1;
	}
	fclose(imgfile);

	//A process image that is not an NNE cannot run
	if(memcmp(imgmem, "NNEARM32", 8) != 0)
	{
		free(imgmem);
		return -1;
	}

	//Replace PID1
	process_t *pptr = &(process_table[1]);
	if(pptr->mem != NULL)
		free(pptr->mem);
	if(pptr->mexec_mem != NULL)
		free(pptr->mexec_mem);

	pptr->mem = imgmem;
	pptr->size = PROCESS_SPACE;
	pptr->mexec_mem = NULL;
	pptr->mexec_size = 0;
	pptr->state = PROCESS_STATE_ALIVE;
	pptr->pid = 1;
	pptr->ppid = 0;
	pptr->paused = false;
	pptr->unpaused = false;
	pptr->dbgstop = PROCESS_DBGSTOP_NONE;

	memset(pptr->regs, 0, sizeof(pptr->regs));
	pptr->regs[15] = 0x1000; //Where mexec puts the entry point
	pptr->cpsr = 0;          //User mode, as mexec_apply leaves it

	return 0;
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
	TDEBUG("%s\n", "pvmk_sc_none");
}

void pvmk_sc_pause(void)
{
	TDEBUG("%s\n", "pvmk_sc_pause");
	sysc_pptr->paused = true;
	return;
}

int pvmk_sc_getticks(void)
{
	TDEBUG("%s\n", "pvmk_sc_getticks");
	
	extern uint32_t EmulTimerTicks;
	return EmulTimerTicks & 0x7FFFFFFFu;
}

int pvmk_sc_fork(void)
{
	TDEBUG("%s\n", "pvmk_sc_fork");
	int child = process_fork(sysc_pptr->pid);
	sysc_logcard("fork: pid %d -> %d\n", sysc_pptr->pid, child);
	return child;
}

int pvmk_sc_wait(uint32_t idtype, uint32_t id, uint32_t options, uint32_t buf, uint32_t len)
{
	TDEBUG("%s %u %u %X %8.8X %u\n", "pvmk_sc_wait", idtype, id, options, buf, len);
	return 0;
}

void pvmk_sc_exit(uint32_t code, uint32_t sig)
{
	TDEBUG("%s %u %u\n", "pvmk_sc_exit", code, sig);
	sysc_logcard("exit: pid %d code %u sig %u\n", sysc_pptr->pid, code, sig);

	//The real kernel ends the process here; nothing in it runs again
	sysc_pptr->state = PROCESS_STATE_DEAD;
	rsp_notify_exit(sysc_pptr->pid);
	(void)sig;
}

int pvmk_sc_gfx_flip(uint32_t mode, uint32_t buffer)
{
	TDEBUG("%s %u %8.8X\n", "pvmk_sc_gfx_flip", mode, buffer);
	sysc_logcard("gfx_flip: pid %d mode %u buf %8.8X size %u\n",
		sysc_pptr->pid, mode, buffer, sysc_pptr->size);
	
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
	TDEBUG("%s %u\n", "pvmk_sc_mem_sbrk", req);
	
	//Compute new size, cap like real system
	uint32_t will_be = sysc_pptr->size + req;
	if(will_be < sysc_pptr->size)
	{
		TERROR("%s", "Overflow in process size; cannot make process smaller.\n");
		return -PVMK_EINVAL;
	}
	
	if(will_be > 24*1024*1024)
	{
		TWARNING("%s", "Requested more memory than 24MBytes.\n");
		return -PVMK_ENOMEM;
	}
	
	sysc_logcard("sbrk: pid %d size %u -> %u\n", sysc_pptr->pid, sysc_pptr->size, will_be);

	//Try to reallocate the user memory
	void *result = realloc(sysc_pptr->mem, will_be);
	if(result == NULL)
	{
		//Host didn't have enough memory for it
		TERROR("%s", "Host is out of memory!\n");
		return -PVMK_ENOMEM;
	}
	
	//Clear the new memory
	memset((char*)result + sysc_pptr->size, 0x00, req);
	
	//Sized the process up successfully. Store new info, return old end-of-process
	uint32_t retval = sysc_pptr->size;
	sysc_pptr->mem = (uint32_t*)result;
	sysc_pptr->size = will_be;
	TDEBUG("Resized process %d from %8.8X to %8.8X\n", sysc_pptr->pid, retval, sysc_pptr->size);
	return retval;
}

int pvmk_sc_input(uint32_t buf, uint32_t each, uint32_t total)
{
	TDEBUG("%s %8.8X %u %u\n", "pvmk_sc_input", buf, each, total);
	
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
	TDEBUG("%s %u %8.8X %u %u\n", "pvmk_sc_snd_play", mode, chunk, chunkbytes, maxbuf);
	
	return -PVMK_ENOSYS;
}

int pvmk_sc_nvm_save(uint32_t buf, uint32_t len)
{
	TDEBUG("%s %8.8X %u\n", "pvmk_sc_nvm_save", buf, len);
	
	return -PVMK_ENOSYS;
}

int pvmk_sc_nvm_load(uint32_t buf, uint32_t len)
{
	TDEBUG("%s %8.8X %u\n", "pvmk_sc_nvm_load", buf, len);
	
	return -PVMK_ENOSYS;
}

int pvmk_sc_env_save(uint32_t buf, uint32_t len)
{
	TDEBUG("%s %8.8X %u\n", "pvmk_sc_env_save", buf, len);
	
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
	TDEBUG("%s %8.8X %u\n", "pvmk_sc_env_load", buf, len);
	
	if(len > (unsigned)sysc_pptr->env_len)
		len = sysc_pptr->env_len;
	
	if(buf < 4096 || buf + len > sysc_pptr->size)
		return -PVMK_EFAULT;
	
	memcpy( ((char*)(sysc_pptr->mem)) + buf, sysc_pptr->env_buf, len);
	return len;
}

int pvmk_sc_sig_mask(uint32_t mask, uint32_t how)
{
	TDEBUG("%s %8.8X %u\n", "pvmk_sc_sig_mask", mask, how);
	
	return -PVMK_ENOSYS;
}

void pvmk_sc_sig_return(void)
{
	TDEBUG("%s\n", "pvmk_sc_sig_return");
}

int pvmk_sc_disk_read2k(uint32_t sector_num, uint32_t buf, uint32_t nsectors)
{
	TDEBUG("%s %u %8.8X %u\n", "pvmk_sc_disk_read2k", sector_num, buf, nsectors);
	sysc_logcard("disk: read sector %u (+%u)\n", sector_num, nsectors);
	
	if(sysc_diskfd < 0)
		return -PVMK_ENXIO;
	
	if(buf + (2048 * nsectors) > sysc_pptr->size)
		return -PVMK_EFAULT;
	if(buf < 4096)
		return -PVMK_EFAULT;
	if(buf % 4)
		return -PVMK_EFAULT;
	
	off_t seeked = lseek(sysc_diskfd, sector_num * 2048ull, SEEK_SET);
	if(seeked != sector_num * 2048ll)
		return -PVMK_ENOSPC;
	
	int nread = read(sysc_diskfd, &(sysc_pptr->mem[buf/4]), 2048 * nsectors);
	if(nread != nsectors * 2048ll)
		return -PVMK_ENOSPC;
	
	return 0;
}

int pvmk_sc_disk_write2k(uint32_t sector_num, uint32_t buf, uint32_t nsectors)
{
	TDEBUG("%s %u %8.8X %u\n", "pvmk_sc_disk_write2k", sector_num, buf, nsectors);
	
	if(sysc_diskfd < 0)
		return -PVMK_ENXIO;
	
	if(buf + (2048 * nsectors) > sysc_pptr->size)
		return -PVMK_EFAULT;
	if(buf < 4096)
		return -PVMK_EFAULT;
	if(buf % 4)
		return -PVMK_EFAULT;
	
	off_t seeked = lseek(sysc_diskfd, sector_num * 2048ull, SEEK_SET);
	if(seeked != sector_num * 2048ll)
		return -PVMK_ENOSPC;
	
	int nwritten = write(sysc_diskfd, &(sysc_pptr->mem[buf/4]), 2048 * nsectors);
	if(nwritten != nsectors * 2048ll)
		return -PVMK_ENOSPC;
	
	return 0;	
}

int pvmk_sc_mexec_append(uint32_t buf, uint32_t len)
{
	TDEBUG("%s %8.8X %u\n", "pvmk_sc_mexec_append", buf, len);

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
	TDEBUG("%s\n", "pvmk_sc_mexec_apply");
	sysc_logcard("mexec: pid %d applying %u bytes\n", sysc_pptr->pid, sysc_pptr->mexec_size);
	
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
		TWARNING("Process %d killed itself by mexec'ing with no pending image\n", sysc_pptr->pid);
		sysc_pptr->state = PROCESS_STATE_DEAD;
		rsp_notify_exit(sysc_pptr->pid);
	}
	
	//Reset CPU regs
	memset(sysc_pptr->regs, 0, sizeof(sysc_pptr->regs));
	sysc_pptr->regs[15] = 0x1000;
	sysc_pptr->cpsr = 0;
	
}

int pvmk_sc_print(uint32_t buf_ptr)
{
	TDEBUG("%s %8.8X\n", "pvmk_sc_print", buf_ptr);
	
	//Print characters until we reach a NUL terminator or run afoul of memory bounds
	int printed = 0;
	while(1)
	{
		if(buf_ptr < 0x1000 || buf_ptr >= sysc_pptr->size)
		{
			TERROR("Bad address %8.8X in _sc_print\n", buf_ptr);
			return (printed > 0) ? printed : -PVMK_EFAULT;
		}
		
		unsigned char to_print = sysc_pptr->mem[buf_ptr/4] >> (8 * (buf_ptr%4));
		if(to_print == 0)
		{
			//End of string, correctly
			break;
		}
		
		//Todo - collect this for a text-mode buffer like the real machine does
		printf("%c", to_print);
		buf_ptr++;
	}		
	return printed;
}

void sysc(process_t *pptr)
{
	TDEBUG("Handling system-call %X from process %d\n", pptr->regs[0], pptr->pid);
	uint32_t *regs = pptr->regs;
	for(int pp = 1; pp < 6; pp++)
	{
		TDEBUG("\tParm %d: %8.8X\n", pp, regs[pp]);
	}
	
	sysc_pptr = pptr;
	int result = 0;
	prof_sysc_hit(regs[0]);
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
		case 0x93: result = pvmk_sc_cardprobe(regs[1]); break;
		case 0xA1: result = pvmk_sc_mexec_append(regs[1], regs[2]); break;
		case 0xA2: /* nr */ pvmk_sc_mexec_apply(); break;
		case 0xB0: result = pvmk_sc_print(regs[1]); break;
		default:   result = -PVMK_ENOSYS; TWARNING("Bad syscall 0x%X\n", regs[0]); break;
	}
	
	TDEBUG("Returning r0=%8.8X from syscall\n", result);
	regs[0] = result;
	return;
}
