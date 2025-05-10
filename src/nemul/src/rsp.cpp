//rsp.cpp
//GDB RSP server for Neki32 emulator
//Bryan E. Topp <betopp@betopp.com> 2025

#include "rsp.h"
#include "trace.h"
#include "process.h"

#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

//Compatibility shim for cross-platform TCP stuff
//Todo - mangle this for WinSock if we're on Windows
#if 1
	//Normal POSIX-y names
	#include <sys/types.h>
	#include <sys/socket.h>
	#include <netdb.h>
	#include <errno.h>
	#include <unistd.h>
	#include <fcntl.h>
	static int rsp_c_close(int sockfd)
		{ return close(sockfd); }
	static int rsp_c_socket(int domain, int type, int protocol)
		{ return socket(domain, type, protocol); }
	static int rsp_c_listen(int sockfd, int backlog)
		{ return listen(sockfd, backlog); }
	static int rsp_c_bind(int sockfd, struct sockaddr *my_addr, int addrlen)
		{ return bind(sockfd, my_addr, addrlen); }
	static int rsp_c_getaddrinfo(const char *node, const char *service, const struct addrinfo *hint, struct addrinfo **res)
		{ return getaddrinfo(node, service, hint, res); }
	static void rsp_c_freeaddrinfo(struct addrinfo *f)
		{ freeaddrinfo(f); }
	static int rsp_c_accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen)
		{ return accept(sockfd, addr, addrlen); }
	static int rsp_c_write(int sockfd, const char *data, int len)
		{ return write(sockfd, data, len); }
	static int rsp_c_read(int sockfd, char *data, int len)
		{ return read(sockfd, data, len); }
	static const char *rsp_c_errstr(void)
		{ return strerror(errno); }
	static int rsp_c_nonblock(int sockfd)
		{ return fcntl(sockfd, F_SETFL, O_NONBLOCK); }
#else
	//...put WinSock shit here
#endif

//Socket accepting new connections for RSP
static int rsp_listen_sock = -1;

//Socket communicating with a client for RSP
static int rsp_client_sock = -1;

//Port we use for RSP listener
static int rsp_port;
	
//Number of times RSP listener experienced a failure (to bind, open a socket, whatever)
static int rsp_giveup_count;
				
//Buffer for incoming RSP packet
static char rsp_incoming_buf[4096];
static int rsp_incoming_len;
		
//Buffer for outgoing RSP packet
static char rsp_outgoing_buf[4096];
static int rsp_outgoing_len;
static int rsp_outgoing_sent;
static unsigned char rsp_outgoing_checksum;

//Process we're operating on
//Neki32 is single-threaded so we say process N has a single thread N.
static int rsp_pid_g; //For general ops
static int rsp_pid_c; //For continue ops

//For qfThreadInfo/qsThreadInfo, which process-table entry is being reported
static int rsp_threadinfo_step;

//Whether GDB explicitly told us it supports error-text, making such safe to use in memory-read commands
static int rsp_errtext_supported;

//Reports an RSP error and disables RSP
static void rsp_fatal(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	
	if(rsp_listen_sock >= 0)
	{
		rsp_c_close(rsp_listen_sock);
		rsp_listen_sock = -1;
	}
	
	if(rsp_client_sock >= 0)
	{
		rsp_c_close(rsp_client_sock);
		rsp_client_sock = -1;
	}
	
	memset(rsp_incoming_buf, 0, sizeof(rsp_incoming_buf));
	rsp_incoming_len = 0;
	
	memset(rsp_outgoing_buf, 0, sizeof(rsp_outgoing_buf));
	rsp_outgoing_len = 0;
	
	rsp_giveup_count++;
	if(rsp_giveup_count > 5)
	{
		rsp_port = 0;
		TRACE("%s", "GDB-RSP listener disabled.\n");
	}		
}
		
//Enqueues a literal byte to be sent back to the RSP client (GDB)		
static void rsp_putbyte(char byte_out)
{
	//This is a programming failure if we enqueue too much data at once
	//(At the top level, we drain outgoing data before reading more commands.)
	if(rsp_outgoing_len >= (int)sizeof(rsp_outgoing_buf))
	{
		rsp_fatal("%s", "Enqueued too much outgoing data. Cannot continue.\n");
		return;
	}
	
	rsp_outgoing_buf[rsp_outgoing_len] = byte_out;
	rsp_outgoing_len++;
	
	rsp_outgoing_checksum += byte_out;
}

//Enqueues the beginning of a packet to be sent back to GDB
static void rsp_putpkt_start(void)
{
	rsp_putbyte('$');
	rsp_outgoing_checksum = 0;
}

//Enqueues the end of a packet to be sent back to GDB
static void rsp_putpkt_end(void)
{
	const char *hexmap = "0123456789ABCDEF";
	unsigned char to_send = rsp_outgoing_checksum; //Because it will change in rsp_putbyte
	rsp_putbyte('#');
	rsp_putbyte(hexmap[(to_send >> 4) & 0xF]);
	rsp_putbyte(hexmap[(to_send >> 0) & 0xF]);
}

//Enqueus a character to be sent back to GDB
static void rsp_putpkt_char(char bb)
{
	if(bb == '$' || bb == '#' || bb == '}' || bb == '*')
	{
		//Need to escape these characters
		rsp_putbyte(0x7D);
		rsp_putbyte(bb ^ 0x20);
	}
	else
	{
		rsp_putbyte(bb);
	}
}

//Enqueues a string to be sent back to GDB
static void rsp_putpkt_str(const char *str)
{
	for(const char *ss = str; *ss != '\0'; ss++)
	{
		rsp_putpkt_char(*ss);
	}
}

//Enqueues an 8-bit number to be sent back to GDB
static void rsp_putpkt_hex8(unsigned char number)
{
	const char *hexmap = "0123456789ABCDEF";
	rsp_putpkt_char(hexmap[ (number >> 4) & 0xF ]);
	rsp_putpkt_char(hexmap[ (number >> 0) & 0xF ]);
}

//Enqueues a 32-bit number to be sent back to GDB (format for most numbers sent)
static void rsp_putpkt_hex32(unsigned int number)
{
	rsp_putpkt_hex8( (number >> 24) & 0xFF );
	rsp_putpkt_hex8( (number >> 16) & 0xFF );
	rsp_putpkt_hex8( (number >>  8) & 0xFF );
	rsp_putpkt_hex8( (number >>  0) & 0xFF );
}

//Enqueues a 32-bit number to be sent back to GDB, but sends the little byte first (format for register contents)
static void rsp_putpkt_hex32le(unsigned int number)
{
	rsp_putpkt_hex8( (number >>  0) & 0xFF );
	rsp_putpkt_hex8( (number >>  8) & 0xFF );
	rsp_putpkt_hex8( (number >> 16) & 0xFF );
	rsp_putpkt_hex8( (number >> 24) & 0xFF );
}

//Enqueues a hex-encoded ASCII string with formatted information to be sent back to GDB
static void rsp_putpkt_hexprintf(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	
	char tempbuf[1024] = {0};
	vsnprintf(tempbuf, sizeof(tempbuf)-1, fmt, ap);
	
	va_end(ap);

	for(int cc = 0; tempbuf[cc] != '\0'; cc++)
	{
		rsp_putpkt_hex8(tempbuf[cc]);
	}
}

//Enqueues a thread ID to be sent back to GDB
static void rsp_putpkt_tid(int pid, int tid)
{
	rsp_putpkt_char('p');
	rsp_putpkt_hex32(pid);
	rsp_putpkt_char('.');
	rsp_putpkt_hex32(tid);
}

//Enques an entire error packet
static void rsp_putpkt_errpkt(const char *fmt, ...)
{
	char errbuf[1024] = {0};
	va_list ap;
	va_start(ap, fmt);
	vsnprintf(errbuf, sizeof(errbuf)-1, fmt, ap);
	va_end(ap);
	
	rsp_putpkt_start();
	rsp_putpkt_str("E.");
	rsp_putpkt_str(errbuf);
	rsp_putpkt_end();
}

//Enqueues an entire error packet but only uses textual message if GDB explicitly said it's okay
//(Necessary for memory read command for example)
static void rsp_putpkt_errpkt_safe(const char *fmt, ...)
{
	if(rsp_errtext_supported)
	{
		char errbuf[1024] = {0};
		va_list ap;
		va_start(ap, fmt);
		vsnprintf(errbuf, sizeof(errbuf)-1, fmt, ap);
		va_end(ap);
		
		rsp_putpkt_start();
		rsp_putpkt_str("E.");
		rsp_putpkt_str(errbuf);
		rsp_putpkt_end();	
	}
	else
	{
		rsp_putpkt_start();
		rsp_putpkt_str("E01");
		rsp_putpkt_end();
	}
}

//Parses a byte, returns number of bytes consumed
int rsp_parse_hex8(const char *buf, int len, unsigned char *val_out)
{
	int consumed = 0;	
	unsigned char val = 0;
	while(len > 0 && consumed < 2)
	{
		if(buf[0] >= '0' && buf[0] <= '9')
			val = (val << 4) + (buf[0] +  0 - '0');
		else if(buf[0] >= 'A' && buf[0] <= 'F')
			val = (val << 4) + (buf[0] + 10 - 'A');
		else if(buf[0] >= 'a' && buf[0] <= 'f')
			val = (val << 4) + (buf[0] + 10 - 'a');
		else
			break;
		
		buf++;
		len--;
		consumed++;
	}
	
	if(val_out != NULL)
		*val_out = val;
	
	return consumed;
}

//Parses a number, returns number of bytes consumed
int rsp_parse_hex(const char *buf, int len, int *val_out)
{
	int consumed = 0;
	
	int neg = 0;
	if(len > 0 && buf[0] == '-')
	{
		neg = 1;
		len--;
		buf++;
		consumed++;
	}
	
	int val = 0;
	while(len > 0)
	{
		if(buf[0] >= '0' && buf[0] <= '9')
			val = (val << 4) + (buf[0] +  0 - '0');
		else if(buf[0] >= 'A' && buf[0] <= 'F')
			val = (val << 4) + (buf[0] + 10 - 'A');
		else if(buf[0] >= 'a' && buf[0] <= 'f')
			val = (val << 4) + (buf[0] + 10 - 'a');
		else
			break;
		
		buf++;
		len--;
		consumed++;
	}
	
	if(neg)
		val *= -1;
	
	if(val_out != NULL)
		*val_out = val;
	
	return consumed;
}

//Parses a thread-ID, returns number of bytes consumed
int rsp_parse_tid(const char *buf, int len, int *pid_out, int *tid_out)
{
	int pid = 0;
	int tid = 0;
	int consumed = 0;
	if(len > 0)
	{
		if(buf[0] == 'p')
		{
			//Multiprocess format, specifying a PID first
			buf++;
			len--;
			consumed++;
			
			//Read process ID first
			int pidlen = rsp_parse_hex(buf, len, &pid);
			buf += pidlen;
			len -= pidlen;
			consumed += pidlen;
			
			if(len > 0 && buf[0] == '.')
			{
				//Also specifying a thread
				buf++;
				len--;
				consumed++;
				
				int tidlen = rsp_parse_hex(buf, len, &tid);
				buf += tidlen;
				len -= tidlen;
				consumed += tidlen;
			}
			else
			{
				//All threads
				tid = -1;
			}
		}
		else
		{
			//Not multiprocess format
			int tidlen = rsp_parse_hex(buf, len, &tid);
			buf += tidlen;
			len -= tidlen;
			consumed += tidlen;
			
			if(tid == -1)
				pid = -1;
		}
	}
	
	if(pid_out != NULL)
		*pid_out = pid;
	if(tid_out != NULL)
		*tid_out = tid;
	
	return consumed;
}

//Command handler - query supported features
static void rsp_cmd_qsupported(const char *remain_buf, int remain_len)
{
	rsp_putpkt_start();
	rsp_putpkt_str("PacketSize=ff0;multiprocess+;error-message+;hwbreak+");
	rsp_putpkt_end();
	
	if(strnstr(remain_buf, "error-message+", remain_len))
		rsp_errtext_supported = 1;
	
	return;
}

//Command handler - list of actions supported by vCont packet
static void rsp_cmd_vcontq(const char *remain_buf, int remain_len)
{
	(void)remain_buf;
	(void)remain_len;
	rsp_putpkt_start();
	rsp_putpkt_str("OK");
	rsp_putpkt_end();
	return;
}

//Command handler - exclamation point (!) enables extended mode
static void rsp_cmd_exclam(const char *remain_buf, int remain_len)
{
	(void)remain_buf;
	(void)remain_len;
	rsp_putpkt_start();
	rsp_putpkt_str("OK");
	rsp_putpkt_end();	
}

//Command handler - set thread ID for further operations
static void rsp_cmd_H(const char *remain_buf, int remain_len)
{
	//Check what operations we'll be setting this for - general or continue
	if(remain_len < 1)
	{
		//Packet too short
		rsp_putpkt_errpkt("No operation specified for setting thread-ID.");
		return;
	}
	char optype = remain_buf[0];
	remain_buf++;
	remain_len--;
	
	int *rsp_pid = NULL;
	if(optype == 'c')
	{
		//Continue ops
		rsp_pid = &rsp_pid_c;
	}
	else if(optype == 'g')
	{
		//General ops
		rsp_pid = &rsp_pid_g;
	}
	else
	{
		//Bad op type
		rsp_putpkt_errpkt("Bad operation type when setting thread-ID.");
		return;
	}
	
	//Parse the thread ID that follows
	int pid = 0;
	int tid = 0;
	int tidlen = rsp_parse_tid(remain_buf, remain_len, &pid, &tid);
	remain_buf += tidlen;
	remain_len -= tidlen;
	
	//Collapse PID and TID into a single PID for us
	if(pid > 0 && tid > 0 && pid != tid)
	{
		//Asked for process and thread ID but they don't match
		rsp_putpkt_errpkt("No such thread");
		return;
	}
	
	if(pid <= 0 && tid > 0)
	{
		//Asked for only a thread ID, use it as the process ID too
		pid = tid;
	}
	
	if(pid == -1)
	{
		//All processes
		*rsp_pid = -1;
	}
	else if(pid == 0)
	{
		//Arbitrary process. Pick the one with the highest PID.
		*rsp_pid = 1;
		for(int pp = 0; pp < PROCESS_MAX; pp++)
		{
			if(process_table[pp].state != PROCESS_STATE_ALIVE)
				continue;
			if(process_table[pp].pid < *rsp_pid)
				continue;
			
			*rsp_pid = process_table[pp].pid;
		}
	}
	else
	{
		//Asked for a specific process.
		int found = 0;
		for(int pp = 0; pp < PROCESS_MAX; pp++)
		{
			if(process_table[pp].state != PROCESS_STATE_ALIVE)
				continue;
			if(process_table[pp].pid != pid)
				continue;
			
			found = 1;
			break;
		}
		if(!found)
		{
			//Asked for a process that doesn't exist
			rsp_putpkt_errpkt("No such process");
			return;
		}
		
		*rsp_pid = pid;
	}
	
	//All good
	rsp_putpkt_start();
	rsp_putpkt_str("OK");
	rsp_putpkt_end();

	return;
}

//Returns subsequent thread information
static void rsp_cmd_qsthreadinfo(const char *remain_buf, int remain_len)
{
	(void)remain_buf;
	(void)remain_len;
	
	//Look for next process-table entry to report
	//Note - we're single-threaded per process, so each process becomes one thread at most
	while(1)
	{
		if(rsp_threadinfo_step >= PROCESS_MAX)
		{
			//No more threads to report
			rsp_putpkt_start();
			rsp_putpkt_str("l");
			rsp_putpkt_end();
			return;
		}
		
		if(process_table[rsp_threadinfo_step].state != PROCESS_STATE_ALIVE)
		{
			//This entry is not in use
			rsp_threadinfo_step++;
			continue;
		}
		
		//Report this process
		int thispid = process_table[rsp_threadinfo_step].pid;
		rsp_threadinfo_step++;
		
		rsp_putpkt_start();
		rsp_putpkt_str("m");
		rsp_putpkt_tid(thispid, thispid);
		rsp_putpkt_end();
		
		return;
	}
}

//Returns first thread information
static void rsp_cmd_qfthreadinfo(const char *remain_buf, int remain_len)
{
	rsp_threadinfo_step = 0;
	rsp_cmd_qsthreadinfo(remain_buf, remain_len);
}

//Returns whether the given process was started by the debugger or was existing and attached-to.
static void rsp_cmd_qattached(const char *remain_buf, int remain_len)
{
	int pid = 0;
	
	int pidlen = rsp_parse_hex(remain_buf, remain_len, &pid);
	remain_buf += pidlen;
	remain_len -= pidlen;
	
	for(int pp = 0; pp < PROCESS_MAX; pp++)
	{
		if(process_table[pp].state != PROCESS_STATE_ALIVE)
			continue;
		if(process_table[pp].pid != pid)
			continue;
		
		//Temp - just report that we always attached an existing process
		rsp_putpkt_start();
		rsp_putpkt_str("1");
		rsp_putpkt_end();
		return;
		
	}
	
	//Didn't find the process they asked about
	rsp_putpkt_errpkt("No such process");
	return;
}

//Command handler - returns current thread ID
static void rsp_cmd_qc(const char *remain_buf, int remain_len)
{
	//This is poorly documented... I guess it means the "general" thread-ID, not the "continue" thread ID?
	(void)remain_buf;
	(void)remain_len;
	rsp_putpkt_start();
	rsp_putpkt_str("QC");
	rsp_putpkt_tid(rsp_pid_g, rsp_pid_g);
	rsp_putpkt_end();
}

//Command handler - get reason for stop
static void rsp_cmd_quest(const char *remain_buf, int remain_len)
{
	(void)remain_buf;
	(void)remain_len;
	

	//Find the process being operated on
	process_t *pptr = process_find(rsp_pid_g);
	if(pptr == NULL)
	{
		//No valid process
		//For now just say it stopped because of a breakpoint when we connected
		rsp_putpkt_start();
		rsp_putpkt_str("T05");
		rsp_putpkt_end();
		return;
	}
	
	//Make sure it's stopped
	if(!pptr->dbgstop)
		pptr->dbgstop = PROCESS_DBGSTOP_CTRLC;
	
	
	switch(pptr->dbgstop)
	{
		//Todo - other stuff
		default:
		{
			rsp_putpkt_start();
			rsp_putpkt_str("T05");
			rsp_putpkt_end();
			return;
		}
	}
	
}

//Command handler - read general purpose registers
static void rsp_cmd_g(const char *remain_buf, int remain_len)
{
	(void)remain_buf;
	(void)remain_len;
	
	//Look up the current thread and get their registers
	uint32_t regs[16] = {0};
	for(int pp = 0; pp < PROCESS_MAX; pp++)
	{
		if(process_table[pp].state != PROCESS_STATE_ALIVE)
			continue;
		if(process_table[pp].pid != rsp_pid_g)
			continue;
		
		memcpy(regs, process_table[pp].regs, sizeof(process_table[pp].regs));
		break;
	}
	
	rsp_putpkt_start();
	for(unsigned int rr = 0; rr < sizeof(regs)/sizeof(regs[0]); rr++)
	{
		rsp_putpkt_hex32le(regs[rr]);
	}
	rsp_putpkt_end();
}

//Command handler - read memory
static void rsp_cmd_mread(const char *remain_buf, int remain_len)
{
	//Read address and length fields
	int address = 0;
	int address_consumed = rsp_parse_hex(remain_buf, remain_len, &address);
	remain_buf += address_consumed;
	remain_len -= address_consumed;
	
	int comma_consumed = 0;
	if(remain_len > 0 && remain_buf[0] == ',')
	{
		comma_consumed = 1;
		remain_buf++;
		remain_len--;
	}
	
	int length = 0;
	int length_consumed = rsp_parse_hex(remain_buf, remain_len, &length);
	remain_buf += length_consumed;
	remain_len -= length_consumed;
	
	if(!(address_consumed && comma_consumed && length_consumed))
	{
		rsp_putpkt_errpkt_safe("Bad format of memory-read command.");
		return;
	}
	
	//Validate that we can actually find the process being operated on
	process_t *pptr = process_find(rsp_pid_g);
	if(pptr == NULL)
	{
		rsp_putpkt_errpkt_safe("No such process");
		return;
	}
	if(pptr->mem == NULL)
	{
		rsp_putpkt_errpkt_safe("Process had no memory space, possibly dead already");
		return;
	}
	
	//Validate that the address range starts in valid memory
	if(address < 4096 || address >= (int)pptr->size)
	{
		rsp_putpkt_errpkt_safe("Memory read starts at invalid address %X", address);
		return;
	}
	
	//Read as many bytes as continue to be valid
	rsp_putpkt_start();
	const char *membytes = (const char*)(pptr->mem);
	for(int aa = address; aa < address + length; aa++)
	{
		if(aa >= (int)pptr->size)
			break;
		
		rsp_putpkt_hex8(membytes[aa]);
	}
	rsp_putpkt_end();
	return;
}

//Command handler - read individual register
static void rsp_cmd_pread(const char *remain_buf, int remain_len)
{
	//Read register index
	int idx = 0;
	int idx_consumed = rsp_parse_hex(remain_buf, remain_len, &idx);
	remain_buf += idx_consumed;
	remain_len -= idx_consumed;
	
	//Validate that we can actually find the process being operated on
	process_t *pptr = process_find(rsp_pid_g);
	if(pptr == NULL)
	{
		rsp_putpkt_errpkt("No such process");
		return;
	}
	
	if(idx >= 0 && idx < 15)
	{
		//GPRs
		rsp_putpkt_start();
		rsp_putpkt_hex32le(pptr->regs[idx]);
		rsp_putpkt_end();
		return;
	}
	if(idx == 25)
	{
		//CPSR
		rsp_putpkt_start();
		rsp_putpkt_hex32le(pptr->cpsr);
		rsp_putpkt_end();
		return;
	}
	
	rsp_putpkt_errpkt("No such register 0x%X", idx);
	return;
}

//Command handler - detach
static void rsp_cmd_detach(const char *remain_buf, int remain_len)
{
	int pid = -1;
	if(remain_len > 0 && remain_buf[0] == ';')
	{
		//PID should follow
		remain_buf++;
		remain_len--;
		
		int pid_consumed = rsp_parse_hex(remain_buf, remain_len, &pid);
		remain_buf += pid_consumed;
		remain_len -= pid_consumed;
	}
	
	if(pid == -1)
	{
		//Detach all
		for(int pp = 0; pp < PROCESS_MAX; pp++)
		{
			process_table[pp].dbgstop = PROCESS_DBGSTOP_NONE;
		}
	}
	else
	{
		//Detach a particular process
		process_t *pptr = process_find(pid);
		if(pptr == NULL)
		{
			rsp_putpkt_errpkt("No such process 0x%X", pid);
			return;
		}
		pptr->dbgstop = PROCESS_DBGSTOP_NONE;
	}
	
	rsp_putpkt_start();
	rsp_putpkt_str("OK");
	rsp_putpkt_end();
	return;
}

//Command handler - per thread information
static void rsp_cmd_qthreadextrainfo(const char *remain_buf, int remain_len)
{
	int pid = 0;
	int tid = 0;
	int tid_consumed = rsp_parse_tid(remain_buf, remain_len, &pid, &tid);
	remain_buf += tid_consumed;
	remain_len -= tid_consumed;
	
	process_t *pptr = process_find(pid);
	if(pptr == NULL)
	{
		rsp_putpkt_errpkt("No such process");
		return;
		
		rsp_putpkt_start();
		rsp_putpkt_hexprintf("No such process");
		rsp_putpkt_end();
		return;
	}
	
	rsp_putpkt_start();
	rsp_putpkt_hexprintf("nemul: PID=%d Size=0x%X", pptr->pid, pptr->size);
	if(pptr->paused && !pptr->unpaused)
		rsp_putpkt_hexprintf(" Paused=1");
	else
		rsp_putpkt_hexprintf(" Paused=0");
	
	rsp_putpkt_hexprintf(" DbgStop=%d", pptr->dbgstop);
	
	rsp_putpkt_end();
	return;
}

//Command handler - write memory
static void rsp_cmd_mwrite(const char *remain_buf, int remain_len)
{
	//Read address
	int address = 0;
	int address_consumed = rsp_parse_hex(remain_buf, remain_len, &address);
	remain_buf += address_consumed;
	remain_len -= address_consumed;
	
	//Read comma
	if(remain_len > 0 && remain_buf[0] == ',')
	{
		remain_buf++;
		remain_len--;
	}
	else
	{
		rsp_putpkt_errpkt("Invalid format for memory-write command.");
		return;
	}
	
	//Read length of data to write
	int length = 0;
	int length_consumed = rsp_parse_hex(remain_buf, remain_len, &length);
	remain_buf += length_consumed;
	remain_len -= length_consumed;
	
	//Read colon
	if(remain_len > 0 && remain_buf[0] == ':')
	{
		remain_buf++;
		remain_len--;
	}
	else
	{
		rsp_putpkt_errpkt("Invalid format for memory-write command.");
		return;
	}
	
	//Make sure we can find the process to operate on
	process_t *pptr = process_find(rsp_pid_g);
	if(pptr == NULL)
	{
		rsp_putpkt_errpkt("No such process 0x%X", rsp_pid_g);
		return;
	}
	if(pptr->mem == NULL)
	{
		rsp_putpkt_errpkt("Process 0x%X has no memory - already dead, perhaps.", rsp_pid_g);
		return;
	}
	
	if(address < 0 || address >= (int)pptr->size)
	{
		rsp_putpkt_errpkt("Memory write is outside size of process memory.");
		return;
	}
	
	//Read bytes and write them into memory
	while(length > 0)
	{
		if(address >= (int)pptr->size)
			break;
		
		unsigned char data = 0;
		int data_consumed = rsp_parse_hex8(remain_buf, remain_len, &data);
		remain_buf += data_consumed;
		remain_len -= data_consumed;
		
		((unsigned char*)(pptr->mem))[address] = data;
		address++;
		length--;
	}
	
	rsp_putpkt_start();
	rsp_putpkt_str("OK");
	rsp_putpkt_end();
	return;
}

//Command handler - continue (verbose)
void rsp_cmd_vcont(const char *remain_buf, int remain_len)
{
	//Read list of actions and thread IDs
	while(remain_len > 0)
	{
		//Should start with a semicolon
		if(remain_len > 0 && remain_buf[0] == ';')
		{
			remain_buf++;
			remain_len--;
		}
		else
		{
			rsp_putpkt_errpkt("Bad format in vCont packet, no semicolon");
			return;
		}
		
		//Then should give an action
		char action = 0;
		//unsigned char sig = 0;
		if(remain_len >= 1 && remain_buf[0] == 'c')
		{
			//Continue
			action = 'c';
			remain_buf++;
			remain_len--;
		}
		//else if(remain_len >= 3 && remain_buf[0] == 'C')
		//{
		//	//Continue with signal
		//	action = 'C';
		//	remain_buf++;
		//	remain_len--;
		//	
		//	int sig_consumed = rsp_parse_hex8(remain_buf, remain_len, &sig);
		//	remain_buf += sig_consumed;
		//	remain_len -= sig_consumed;
		//}
		else
		{
			rsp_putpkt_errpkt("Unknown action %c in vCont packet", action);
			return;
		}
		
		//Then a colon
		if(remain_len > 0 && remain_buf[0] == ':')
		{
			remain_buf++;
			remain_len--;
		}
		else
		{
			rsp_putpkt_errpkt("Bad format in vCont packet, no colon");
			return;
		}
		
		//Then the thread ID
		int pid = 0;
		int tid = 0;
		int tid_consumed = rsp_parse_tid(remain_buf, remain_len, &pid, &tid);
		remain_buf += tid_consumed;
		remain_len -= tid_consumed;
		
		//So do that
		int matched = 0;
		for(int pp = 0; pp < PROCESS_MAX; pp++)
		{
			if(process_table[pp].pid == pid || pid == -1)
			{
				matched++;
				if(action == 'c')
				{
					process_table[pp].dbgstop = PROCESS_DBGSTOP_NONE;
				}
			}
		}
		
		if(matched == 0)
		{
			//No such process...? Say that it exited
			rsp_putpkt_start();
			//rsp_putpkt_str("X05;process:");
			//rsp_putpkt_hex32(pid);
			rsp_putpkt_str("W00");
			rsp_putpkt_end();
			return;
		}
	}
}

//Command handler - write register
static void rsp_cmd_pwrite(const char *remain_buf, int remain_len)
{
	//Parse register number
	int regnum = 0;
	int regnum_consumed = rsp_parse_hex(remain_buf, remain_len, &regnum);
	remain_buf += regnum_consumed;
	remain_len -= regnum_consumed;
	
	//Should be followed by equals sign
	if(remain_len > 0 && remain_buf[0] == '=')
	{
		remain_buf++;
		remain_len--;
	}
	else
	{
		rsp_putpkt_errpkt("Bad format in set-register command, no equals sign");
		return;
	}
	
	//Then get the register value
	unsigned int regval = 0;
	int bytesdone = 0;
	while(remain_len > 0)
	{
		unsigned char nextbyte = 0;
		int nextbyte_consumed = rsp_parse_hex8(remain_buf, remain_len, &nextbyte);
		remain_buf += nextbyte_consumed;
		remain_len -= nextbyte_consumed;
		
		if(nextbyte_consumed <= 0)
			break;
		
		regval |= ((unsigned int)nextbyte) << (8 * bytesdone);
		bytesdone++;
	}
	
	//Make sure we're operating on a valid process
	process_t *pptr = process_find(rsp_pid_g);
	if(pptr == NULL)
	{
		rsp_putpkt_errpkt("No such process 0x%X", rsp_pid_g);
		return;
	}
	
	//Set the register value
	if(regnum >= 0 && regnum < 16)
	{
		pptr->regs[regnum] = regval;
	}
	else if(regnum == 0x19)
	{
		pptr->cpsr = regval;
	}
	else
	{
		rsp_putpkt_errpkt("Unknown register number 0x%X", regnum);
		return;
	}
	
	rsp_putpkt_start();
	rsp_putpkt_str("OK");
	rsp_putpkt_end();
	return;
}

//Command handler - check if thread is alive
static void rsp_cmd_talive(const char *remain_buf, int remain_len)
{
	int pid = 0;
	int tid = 0;
	int parsed = rsp_parse_tid(remain_buf, remain_len, &pid, &tid);
	remain_buf += parsed;
	remain_len -= parsed;
	
	if(pid <= 0)
	{
		//Resolve to "any pid"
		for(int pp = 0; pp < PROCESS_MAX; pp++)
		{
			if(process_table[pp].state == PROCESS_STATE_ALIVE)
			{
				pid = process_table[pp].pid;
				break;
			}
		}
	}
	
	if(tid > 0 && tid != pid)
	{
		//No such thread - all processes are singlethreaded and the tid == pid
		rsp_putpkt_start();
		rsp_putpkt_end();
		return;
	}
	
	process_t *pptr = process_find(pid);
	if(pptr != NULL && pptr->state == PROCESS_STATE_ALIVE)
	{
		//Process is alive
		rsp_putpkt_start();
		rsp_putpkt_str("OK");
		rsp_putpkt_end();
		return;
	}
	else
	{
		//Process is dead and/or gone
		rsp_putpkt_start();
		rsp_putpkt_end();
		return;
	}
}

//Remote monitor command handler - reset process table as if booting a new game
static void rsp_rcmd_prep(void)
{
	rsp_putpkt_start();
	rsp_putpkt_hexprintf("Resetting process table...\n");
	rsp_putpkt_end();
	
	//Reset as if just booting
	process_reset();
	
	//Resize the initial process to be 24MBytes
	process_table[1].mem = (uint32_t*)realloc(process_table[1].mem, 24*1024*1024);
	memset(((char*)(process_table[1].mem)) + process_table[1].size, 0, 24*1024*1024 - process_table[1].size);
	process_table[1].size = 24*1024*1024;
	
	//Stop everybody immediately
	for(int pp = 0; pp < PROCESS_MAX; pp++)
	{
		if(process_table[pp].state == PROCESS_STATE_ALIVE)
			process_table[pp].dbgstop = PROCESS_DBGSTOP_CTRLC;
	}
	
	//Reset active PIDs
	rsp_pid_c = 1;
	rsp_pid_g = 1;
	
}

//Remote monitor command ("monitor ...") decoding table
typedef struct rsp_rcmd_s
{
	const char *cmd;
	const char *help;
	void (*func)(void);
} rsp_rcmd_t;
static const rsp_rcmd_t rsp_rcmd_table[] = 
{
	{ .cmd = "prep", .help = "Resets process-table as if booting a game", .func = rsp_rcmd_prep },
	{}
};

//Command handler - "monitor" remote commands
static void rsp_cmd_rcmd(const char *remain_buf, int remain_len)
{
	//Parse out the command given - for now just single tokens
	char decoded_cmd[16] = {0};
	int decoded_len = 0;
	while(remain_len > 0 && decoded_len < (int)sizeof(decoded_cmd))
	{
		unsigned char byte_val = 0;
		int byte_consumed = rsp_parse_hex8(remain_buf, remain_len, &byte_val);
		remain_buf += byte_consumed;
		remain_len -= byte_consumed;
		
		decoded_cmd[decoded_len] = (char)byte_val;
		decoded_len++;
	}
	
	//Check if they want a listing of the available commands
	if(!strcmp(decoded_cmd, "help") || decoded_cmd[0] == '\0')
	{
		//Show help
		rsp_putpkt_start();
		rsp_putpkt_hexprintf("nemul: The Neki32 user-level process simulator\n");
		rsp_putpkt_hexprintf("Version " BUILDVERSION " at " BUILDDATE " by " BUILDUSER "\n");
		rsp_putpkt_hexprintf("Monitor commands available:\n");
		for(const rsp_rcmd_t *cc = rsp_rcmd_table; cc->cmd != NULL; cc++)
		{
			rsp_putpkt_hexprintf("%8s - %s\n", cc->cmd, cc->help);
		}
		rsp_putpkt_end();
		return;
	}
	
	//Otherwise, check what command they're trying to run
	for(const rsp_rcmd_t *cc = rsp_rcmd_table; cc->cmd != NULL; cc++)
	{
		if(!strcmp(decoded_cmd, cc->cmd))
		{
			//Found it
			(cc->func)();
			return;
		}
	}
	
	//If none matched, tell them so
	rsp_putpkt_start();
	rsp_putpkt_hexprintf("Unknown monitor command (try \"monitor help\")\n");
	rsp_putpkt_end();
	return;
	
}

//Command decoding table
typedef struct rsp_cmd_s
{
	const char *prefix;
	void (*handler)(const char *remain_buf, int remain_len);
} rsp_cmd_t;
static const rsp_cmd_t rsp_cmd_table[] = 
{
	{ .prefix = "qThreadExtraInfo,",    .handler = rsp_cmd_qthreadextrainfo },
	{ .prefix = "qfThreadInfo",         .handler = rsp_cmd_qfthreadinfo },
	{ .prefix = "qsThreadInfo",         .handler = rsp_cmd_qsthreadinfo },
	{ .prefix = "qSupported",           .handler = rsp_cmd_qsupported },
	{ .prefix = "qAttached:",           .handler = rsp_cmd_qattached },
	{ .prefix = "qRcmd,",               .handler = rsp_cmd_rcmd },
	{ .prefix = "vCont?",               .handler = rsp_cmd_vcontq },
	{ .prefix = "vCont",                .handler = rsp_cmd_vcont },
	{ .prefix = "qC",                   .handler = rsp_cmd_qc },
	{ .prefix = "!",                    .handler = rsp_cmd_exclam },
	{ .prefix = "H",                    .handler = rsp_cmd_H },
	{ .prefix = "g",                    .handler = rsp_cmd_g },
	{ .prefix = "?",                    .handler = rsp_cmd_quest },
	{ .prefix = "m",                    .handler = rsp_cmd_mread },
	{ .prefix = "p",                    .handler = rsp_cmd_pread },
	{ .prefix = "D",                    .handler = rsp_cmd_detach },
	{ .prefix = "M",                    .handler = rsp_cmd_mwrite },
	{ .prefix = "P",                    .handler = rsp_cmd_pwrite },
	{ .prefix = "T",                    .handler = rsp_cmd_talive },
	{  }, //Sentinel
};

//Command decoding - top level
static void rsp_cmd(const char *cmd_buf, int cmd_len)
{
	TRACE("RSP command received: %.*s\n", cmd_len, cmd_buf);
	for(int tt = 0; rsp_cmd_table[tt].prefix != NULL; tt++)
	{
		const char *prefix = rsp_cmd_table[tt].prefix;
		int prefix_len = strlen(prefix);
		if(cmd_len >= prefix_len && !memcmp(cmd_buf, prefix, prefix_len))
		{
			//Matched
			(*rsp_cmd_table[tt].handler)(cmd_buf + prefix_len, cmd_len - prefix_len);
			return;
		}
	}
	
	//Other, empty, or unknown command - respond with empty response
	rsp_putpkt_start();
	rsp_putpkt_end();
	return;
}

		
//Handles incoming bytes from RSP client (i.e. GDB)
static void rsp_gotbyte(char byte_in)
{
	if(byte_in == '$')
	{
		//Dollarsign always starts a new packet
		memset(rsp_incoming_buf, 0, sizeof(rsp_incoming_buf));
		rsp_incoming_len = 0;
		
		rsp_incoming_buf[0] = '$';
		rsp_incoming_len = 1;
		return;
	}
	
	if(rsp_incoming_len == 0)
	{
		//If GDB sent us a Ctrl-C, react with a stop reply
		if(byte_in == 0x03)
		{
			//Stop everybody and respond as such
			for(int pp = 0; pp < PROCESS_MAX; pp++)
			{
				if(process_table[pp].state != PROCESS_STATE_ALIVE)
					continue;
				if(process_table[pp].dbgstop)
					continue;
				
				process_table[pp].dbgstop = PROCESS_DBGSTOP_CTRLC;
			}
			
			rsp_putpkt_start();
			rsp_putpkt_str("T05");
			rsp_putpkt_end();
			return;
		}
		
		//Process nothing else until we see a dollarsign to start the packet
		return;
	}
	
	if(rsp_incoming_len >= (int)sizeof(rsp_incoming_buf))
	{
		//Overlong packet, discard
		memset(rsp_incoming_buf, 0, sizeof(rsp_incoming_buf));
		rsp_incoming_len = 0;
		rsp_putbyte('-');
		return;
	}
	
	//Append the new character to the received packet
	rsp_incoming_buf[rsp_incoming_len] = byte_in;
	rsp_incoming_len++;
	
	if(rsp_incoming_len >= 4 && rsp_incoming_buf[rsp_incoming_len-3] == '#')
	{
		//Got the terminating '#' and checksum for a packet.
		//See if the packet is valid.
		unsigned char computed_checksum = 0;
		for(int bb = 1; bb < rsp_incoming_len - 3; bb++) //Skip leading '$' and trailing '#NN'
		{
			computed_checksum += (unsigned char)rsp_incoming_buf[bb];
		}
		
		unsigned char stated_checksum = 0;
		for(int cc = rsp_incoming_len - 2; cc < rsp_incoming_len; cc++) //Just the last two chars
		{
			char hexchar = rsp_incoming_buf[cc];
			stated_checksum <<= 4;
			if(hexchar >= '0' && hexchar <= '9')
				stated_checksum +=  0 + hexchar - '0';
			else if(hexchar >= 'A' && hexchar <= 'F')
				stated_checksum += 10 + hexchar - 'A';
			else if(hexchar >= 'a' && hexchar <= 'f')
				stated_checksum += 10 + hexchar - 'a';
		}
		
		if(computed_checksum == stated_checksum)
		{
			//Packet seems valid, acknowledge and pass to decoding
			rsp_putbyte('+');
			rsp_cmd(rsp_incoming_buf + 1, rsp_incoming_len - 4);
		}
		else
		{
			//Packet seems invalid, nack it
			rsp_putbyte('-');
		}
		
		//Done with the packet
		memset(rsp_incoming_buf, 0, sizeof(rsp_incoming_buf));
		rsp_incoming_len = 0;
		return;
	}	
}
	
void rsp_init(const prefs_t *prefs)
{
	//Close existing sockets if any
	if(rsp_listen_sock >= 0)
	{
		rsp_c_close(rsp_listen_sock);
		rsp_listen_sock = -1;
	}
	
	if(rsp_client_sock >= 0)
	{
		rsp_c_close(rsp_client_sock);
		rsp_client_sock = -1;
	}
	
	//Check if we're actually supposed to be running
	if(!prefs->rsp_enabled)
	{
		TRACE("%s", "GDB-RSP listener disabled in preferences. Not starting.\n");
		return;
	}
	
	rsp_port = prefs->rsp_port;
	TRACE("Setting up GDB-RSP listener on port %d\n", rsp_port);
}

void rsp_poll(void)
{
	if(rsp_port <= 0)
	{
		//RSP not configured
		return;
	}
	
	//Handle connection setup/acceptance
	if(rsp_client_sock < 0 && rsp_listen_sock < 0)
	{
		//Neither socket valid. Need to make the listener socket to get connections.
		
		//Get address to bind to, with appropriate local port
		struct addrinfo hints;
		memset(&hints, 0, sizeof(hints));
		hints.ai_family = AF_INET; //IPv4
		hints.ai_socktype = SOCK_STREAM; //TCP
		hints.ai_flags = AI_PASSIVE; //Fill in local IP automatically
		
		char portstr[8] = {0};
		snprintf(portstr, sizeof(portstr)-1, "%d", rsp_port);
		
		struct addrinfo *result_addr = NULL;
		int gai_result = rsp_c_getaddrinfo(NULL, portstr, &hints, &result_addr);
		if(gai_result < 0 || result_addr == NULL)
		{
			rsp_fatal("Failed to get address information for local port: %s\n", rsp_c_errstr());
			return;
		}
		
		//Make a socket for that type of address
		rsp_listen_sock = rsp_c_socket(result_addr->ai_family, result_addr->ai_socktype, result_addr->ai_protocol);
		if(rsp_listen_sock < 0)
		{
			rsp_fatal("Failed to make listener socket: %s\n", rsp_c_errstr());
			rsp_c_freeaddrinfo(result_addr);
			result_addr = NULL;
			return;
		}
		
		int listen_nonblocking = rsp_c_nonblock(rsp_listen_sock);
		if(listen_nonblocking < 0)
		{
			rsp_fatal("Failed to set listener socket to nonblocking: %s\n", rsp_c_errstr());
			rsp_c_freeaddrinfo(result_addr);
			result_addr = NULL;
			rsp_c_close(rsp_listen_sock);
			rsp_listen_sock = -1;
			return;
		}

		//Bind to the local port specified
		int bound = rsp_c_bind(rsp_listen_sock, result_addr->ai_addr, result_addr->ai_addrlen);
		if(bound < 0)
		{
			rsp_fatal("Failed to bind to local port: %s\n", rsp_c_errstr());
			rsp_c_freeaddrinfo(result_addr);
			result_addr = NULL;
			rsp_c_close(rsp_listen_sock);
			rsp_listen_sock = -1;
			return;
		}
		
		//Done with address info now
		rsp_c_freeaddrinfo(result_addr);
		result_addr = NULL;
		
		//Start listening for connections on that socket
		int listening = rsp_c_listen(rsp_listen_sock, 1);
		if(listening < 0)
		{
			rsp_fatal("Failed to listen on socket: %s\n", rsp_c_errstr());
			rsp_c_close(rsp_listen_sock);
			rsp_listen_sock = -1;
			return;
		}
		
		//Made the listener socket OK
		return;
	}
	else if(rsp_client_sock < 0 && rsp_listen_sock >= 0)
	{
		//No client, but we've made the listener. See if we can accept a connection.
		struct sockaddr_storage their_addr;
		memset(&their_addr, 0, sizeof(their_addr));
		
		socklen_t their_addrlen = sizeof(their_addr);
		
		rsp_client_sock = rsp_c_accept(rsp_listen_sock, (struct sockaddr*)(&their_addr), &their_addrlen);
		if(rsp_client_sock < 0)
		{
			//Nobody connected to us yet
			return;
		}
		
		int client_nonblocking = rsp_c_nonblock(rsp_client_sock);
		if(client_nonblocking < 0)
		{
			rsp_fatal("Failed to make client socket nonblocking: %s\n", rsp_c_errstr());
			rsp_c_close(rsp_client_sock);
			rsp_client_sock = -1;
			return;
		}
		
		//Great, we got a connection
		return;
	}
	else if(rsp_client_sock > 0 && rsp_listen_sock > 0)
	{
		//Have both client and listener sockets. Don't listen for any more clients.
		rsp_c_close(rsp_listen_sock);
		rsp_listen_sock = -1;
		return;
	}
	
	//Okay, we have a client connected and further listening closed.
	rsp_giveup_count = 0;
	
	//Drain outgoing data to the client before reading more commands
	while(1)
	{
		if(rsp_outgoing_len == 0)
		{
			//Nothing to send.
			break;
		}
		
		if(rsp_outgoing_sent >= rsp_outgoing_len)
		{
			//Drained the entire outgoing buffer.
			rsp_outgoing_sent = 0;
			rsp_outgoing_len = 0;
			memset(rsp_outgoing_buf, 0, sizeof(rsp_outgoing_buf));
			break;
		}
		
		int nsent = rsp_c_write(rsp_client_sock, rsp_outgoing_buf + rsp_outgoing_sent, rsp_outgoing_len - rsp_outgoing_sent);
		if(nsent < 0 && errno == EAGAIN)
		{
			//Pipe is stuffed up, need to wait
			return;
		}
		
		if(nsent < 0)
		{
			//Other error while sending
			rsp_fatal("Failed to write to RSP socket: %s\n", rsp_c_errstr());
			return;
		}
		
		if(nsent == 0)
		{
			//Client disconnected from us
			rsp_fatal("%s", "Client disconnected.\n");
			return;
		}
		
		//Sent some bytes successfully
		rsp_outgoing_sent += nsent;
	}
	
	//Read incoming data and process commands
	while(1)
	{
		//Get next set of incoming bytes
		char readbuf[256] = {0};
		int nread = rsp_c_read(rsp_client_sock, readbuf, sizeof(readbuf));
		if(nread < 0 && errno == EAGAIN)
		{
			//No more data at this time
			return;		
		}
		
		if(nread < 0)
		{
			//Other error
			rsp_fatal("Failed to read RSP socket: %s\n", rsp_c_errstr());
			return;
		}
		
		if(nread == 0)
		{
			//Client disconnected from us
			rsp_fatal("%s", "Client disconnected.\n");
			return;
		}
		
		//Run through bytes and feed into RSP protocol handler
		for(int bb = 0; bb < nread; bb++)
		{
			rsp_gotbyte(readbuf[bb]);
		}
	}
}

void rsp_dbgstop(int pid, process_dbgstop_t reason)
{
	process_t *pptr = process_find(pid);
	if(pptr == NULL)
	{
		TRACE("%s", "Tried to set debug-stop for an invalid process");
		return;
	}
	
	pptr->dbgstop = reason;
		
	static const int signal_mapping[PROCESS_DBGSTOP_MAX] = 
	{
		[PROCESS_DBGSTOP_AC] = 10, //GDB SIGBUS
		[PROCESS_DBGSTOP_BKPT] = 5, //GDB SIGTRAP
		[PROCESS_DBGSTOP_ABT] = 11, //GDB SIGSEGV
		[PROCESS_DBGSTOP_PF] = 11, //GDB SIGSEGV
	};
	
	int report_signal = 5;
	if(signal_mapping[reason])
		report_signal = signal_mapping[reason];
	
	rsp_putpkt_start();
	rsp_putpkt_str("T");
	rsp_putpkt_hex8(report_signal);
	rsp_putpkt_str("thread:p");
	rsp_putpkt_hex32(pptr->pid);
	rsp_putpkt_str(".");
	rsp_putpkt_hex32(pptr->pid);
	rsp_putpkt_str(";");
	rsp_putpkt_end();
}
