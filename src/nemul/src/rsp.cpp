//rsp.cpp
//GDB RSP server for Neki32 emulator
//Bryan E. Topp <betopp@betopp.com> 2025

//Nemul, the Neki32 Simulator, Copyright 2025 Nekisoft Pty Ltd, ACN 680 583 251
//This program is free software: you can redistribute it and/or modify
//it under the terms of the GNU General Public License as published by
//the Free Software Foundation, either version 3 of the License, or
//(at your option) any later version.

//The Neki32 runs processes, not threads, so each process is one thread as far as GDB is
//concerned, and the stub speaks the multiprocess protocol: a thread ID looks like "p<pid>.1",
//and thread one is the only thread a process has. Everything a client can do to a thread -
//select it, ask whether it is alive, continue or step it, break in it, hear that it stopped or
//exited - is answered per process, and a stop in any process stops the lot, which is what GDB's
//all-stop mode expects of a remote target.

#define FILE_TRACE_CAT TRACE_CAT_RSP
#include "trace.h"

#include "rsp.h"
#include "process.h"

#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

//Compatibility shim for cross-platform TCP stuff
#if !defined(__MINGW32__)
	//Constant that Winsock would use
	#define INVALID_SOCKET (-1)
	typedef int SOCKET;

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
		{ return (int)write(sockfd, data, (size_t)len); }
	static int rsp_c_read(int sockfd, char *data, int len)
		{ return (int)read(sockfd, data, (size_t)len); }
	static const char *rsp_c_errstr(void)
		{ return strerror(errno); }
	static int rsp_c_nonblock(int sockfd)
		{ return fcntl(sockfd, F_SETFL, O_NONBLOCK); }
	static int rsp_c_wouldblock(int err)
		{ return err == EAGAIN || err == EWOULDBLOCK; }
#else
	//Winsock mangled names
	#include <Winsock2.h>
	#include <ws2tcpip.h>

	static int rsp_c_close(int sockfd)
		{ return closesocket(sockfd); }
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
		{ return send(sockfd, data, len, 0); }
	static int rsp_c_read(int sockfd, char *data, int len)
		{ return recv(sockfd, data, len, 0); }
	static const char *rsp_c_errstr(void)
		{ return strerror(WSAGetLastError()); }
	static int rsp_c_nonblock(int sockfd)
		{ unsigned long mode = 1; return ioctlsocket(sockfd, FIONBIO, &mode); }
	static int rsp_c_wouldblock(int err)
		{ return err == WSAEWOULDBLOCK; }
#endif

//Socket accepting new connections for RSP
static SOCKET rsp_listen_sock = INVALID_SOCKET;

//Socket communicating with a client for RSP
static SOCKET rsp_client_sock = INVALID_SOCKET;

//Port we use for RSP listener
static int rsp_port;

//Number of times RSP listener experienced a failure (to bind, open a socket, whatever)
static int rsp_giveup_count;

//Buffer for incoming RSP packet, still escaped
static char rsp_incoming_buf[16384];
static int rsp_incoming_len;

//Packet after unescaping, which is what the command handlers see
static char rsp_packet_buf[16384];

//Buffer for outgoing RSP packet
static char rsp_outgoing_buf[16384];
static int rsp_outgoing_len;
static int rsp_outgoing_sent;
static unsigned char rsp_outgoing_checksum;

//Which process each of the two thread selectors is set to. -1 means "all of them", 0 means
//"whichever one is current", and anything else names a process.
static int rsp_pid_g = -1; //For general ops
static int rsp_pid_c = -1; //For continue ops

//Diagnostics for the protocol itself, which no client ever shows you: NEMUL_LOG_RSP=1 puts every
//decision that matters on stderr, so a session that stops making sense can be read rather than
//guessed at.
static int rsp_logging(void)
{
	static int logging = -1;
	if(logging < 0)
		logging = (getenv("NEMUL_LOG_RSP") != NULL) ? 1 : 0;
	return logging;
}
#define RSPLOG(...) do { if(rsp_logging()) { fprintf(stderr, "rsp: "); fprintf(stderr, __VA_ARGS__); } } while(0)

//Hook the front end gives us, so that a client can ask for a program to be run and can be told
//which one is loaded
static int (*rsp_hook_load)(const char *path, char *err, size_t errsz) = NULL;
static const char *(*rsp_hook_image)(void) = NULL;

//The breakpoints the client has placed, kept here as well as in the machine so that restarting
//the machine does not quietly lose them: a client that asks for a program to be run again
//expects its breakpoints to still be there, and it has no way of knowing that the process table
//behind them was thrown away.
#define RSP_BP_MAX 64
static uint32_t rsp_bp_addr[RSP_BP_MAX];
static int rsp_bp_count;

//For qfThreadInfo/qsThreadInfo, which process-table entry is being reported
static int rsp_threadinfo_step;

//Whether GDB explicitly told us it supports error-text, making such safe to use in memory-read commands
static int rsp_errtext_supported;

//The last stop we reported, so that "?" can report it again
static int rsp_stop_pid;
static int rsp_stop_signal;

//One instruction of one process to run before the machine goes on again, for single-stepping
static int rsp_step_pid;
static int rsp_step_count;

//What the front end wants done when a client sets the machine going
static void (*rsp_continue_hook)(void);

//------------------------------------------------------------------------------------------------
//Outgoing packets

//Reports an RSP error and disables RSP
static void rsp_fatal(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);

	if(rsp_listen_sock != INVALID_SOCKET)
	{
		rsp_c_close(rsp_listen_sock);
		rsp_listen_sock = INVALID_SOCKET;
	}

	if(rsp_client_sock != INVALID_SOCKET)
	{
		rsp_c_close(rsp_client_sock);
		rsp_client_sock = INVALID_SOCKET;
	}

	rsp_incoming_len = 0;
	rsp_outgoing_len = 0;
	rsp_outgoing_sent = 0;
	rsp_step_pid = 0;
	rsp_step_count = 0;

	//The client that was holding processes still has gone, so let them run again
	for(int pp = 0; pp < PROCESS_MAX; pp++)
		process_table[pp].dbgstop = PROCESS_DBGSTOP_NONE;

	rsp_giveup_count++;
	if(rsp_giveup_count > 5)
	{
		rsp_port = 0;
		TWARNING("%s", "GDB-RSP listener disabled.\n");
	}
}

//A client has gone away. This is an ordinary thing - a debugger is detached or closed all the
//time - so it must not count against the listener, which has to stay able to take the next one.
static void rsp_client_lost(void)
{
	if(rsp_client_sock != INVALID_SOCKET)
	{
		rsp_c_close(rsp_client_sock);
		rsp_client_sock = INVALID_SOCKET;
	}

	rsp_incoming_len = 0;
	rsp_outgoing_len = 0;
	rsp_outgoing_sent = 0;
	rsp_step_pid = 0;
	rsp_step_count = 0;
	rsp_pid_g = -1;
	rsp_pid_c = -1;
	rsp_stop_pid = 0;

	//Whatever was being held still for the client runs again
	for(int pp = 0; pp < PROCESS_MAX; pp++)
		process_table[pp].dbgstop = PROCESS_DBGSTOP_NONE;
}

//Enqueues a literal byte to be sent back to the RSP client (GDB)
static void rsp_putbyte(char byte_out)
{
	if(rsp_outgoing_len >= (int)sizeof(rsp_outgoing_buf))
	{
		rsp_fatal("%s", "Enqueued too much outgoing data. Cannot continue.\n");
		return;
	}

	rsp_outgoing_buf[rsp_outgoing_len] = byte_out;
	rsp_outgoing_len++;

	rsp_outgoing_checksum = (unsigned char)(rsp_outgoing_checksum + (unsigned char)byte_out);
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
		rsp_putbyte((char)(bb ^ 0x20));
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
		rsp_putpkt_char(*ss);
}

//Enqueues an 8-bit number to be sent back to GDB
static void rsp_putpkt_hex8(unsigned char number)
{
	const char *hexmap = "0123456789ABCDEF";
	rsp_putpkt_char(hexmap[ (number >> 4) & 0xF ]);
	rsp_putpkt_char(hexmap[ (number >> 0) & 0xF ]);
}

//Enqueues a number without the leading zeros that a thread ID does not need
static void rsp_putpkt_num(unsigned int number)
{
	const char *hexmap = "0123456789abcdef";
	char digits[16] = {0};
	int ndigits = 0;
	do
	{
		digits[ndigits] = hexmap[number & 0xF];
		ndigits++;
		number >>= 4;
	} while(number != 0);

	while(ndigits > 0)
	{
		ndigits--;
		rsp_putpkt_char(digits[ndigits]);
	}
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
		rsp_putpkt_hex8((unsigned char)tempbuf[cc]);
}

//Enqueues a thread ID to be sent back to GDB. A Neki32 process has exactly one thread, so the
//thread of process N is thread N: one number, in one inferior. Multiprocess mode would make
//each process an inferior of its own, and an inferior whose program is restarted is a different
//inferior than the one the breakpoints were put in - which is no way to debug a machine that
//starts its programs by rebooting itself.
static void rsp_putpkt_tid(int pid)
{
	if(pid <= 0)
	{
		rsp_putpkt_str("-1");
		return;
	}

	rsp_putpkt_num((unsigned)pid);
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
	if(!rsp_errtext_supported)
	{
		rsp_putpkt_start();
		rsp_putpkt_str("E01");
		rsp_putpkt_end();
		return;
	}

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

//Sends a plain acknowledgement for commands that just work
static void rsp_reply_ok(void)
{
	rsp_putpkt_start();
	rsp_putpkt_str("OK");
	rsp_putpkt_end();
}

//Sends the empty reply, which means "not supported" to a client
static void rsp_reply_empty(void)
{
	rsp_putpkt_start();
	rsp_putpkt_end();
}

//------------------------------------------------------------------------------------------------
//Parsing

//Parses a byte, returns number of bytes consumed
int rsp_parse_hex8(const char *buf, int len, unsigned char *val_out)
{
	int consumed = 0;
	unsigned char val = 0;
	while(len > 0 && consumed < 2)
	{
		if(buf[0] >= '0' && buf[0] <= '9')
			val = (unsigned char)((val << 4) + (buf[0] - '0'));
		else if(buf[0] >= 'A' && buf[0] <= 'F')
			val = (unsigned char)((val << 4) + 10 + (buf[0] - 'A'));
		else if(buf[0] >= 'a' && buf[0] <= 'f')
			val = (unsigned char)((val << 4) + 10 + (buf[0] - 'a'));
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

	unsigned int val = 0;
	while(len > 0)
	{
		if(buf[0] >= '0' && buf[0] <= '9')
			val = (val << 4) + (unsigned)(buf[0] - '0');
		else if(buf[0] >= 'A' && buf[0] <= 'F')
			val = (val << 4) + 10u + (unsigned)(buf[0] - 'A');
		else if(buf[0] >= 'a' && buf[0] <= 'f')
			val = (val << 4) + 10u + (unsigned)(buf[0] - 'a');
		else
			break;

		buf++;
		len--;
		consumed++;
	}

	if(val_out != NULL)
		*val_out = neg ? -(int)val : (int)val;

	return consumed;
}

//Parses a thread ID, which a client may write as "p<pid>.<tid>", "p<pid>", "<tid>", "-1" for all
//threads or "0" for any thread. Returns the number of bytes consumed and, through pid_out, the
//process the thread belongs to: -1 for all of them and 0 for "whichever one is current".
static int rsp_parse_thread(const char *buf, int len, int *pid_out)
{
	int pid = -1;
	int tid = 0;
	int consumed = 0;

	if(len > 0 && buf[0] == 'p')
	{
		//Multiprocess form
		buf++;
		len--;
		consumed++;

		const int pid_consumed = rsp_parse_hex(buf, len, &pid);
		buf += pid_consumed;
		len -= pid_consumed;
		consumed += pid_consumed;

		if(len > 0 && buf[0] == '.')
		{
			buf++;
			len--;
			consumed++;

			const int tid_consumed = rsp_parse_hex(buf, len, &tid);
			len -= tid_consumed;
			consumed += tid_consumed;
		}
	}
	else if(len > 0)
	{
		//Plain thread number, which a client uses when it is not told about processes
		const int tid_consumed = rsp_parse_hex(buf, len, &tid);
		len -= tid_consumed;
		consumed += tid_consumed;

		if(tid < 0)
			pid = -1;
		else if(tid == 0)
			pid = 0;
		else
			pid = tid;
	}

	//A process has exactly one thread; a request for any other is not one this machine has
	if(tid > 0 && tid != 1)
		pid = 0;

	if(pid_out != NULL)
		*pid_out = pid;

	return consumed;
}

//------------------------------------------------------------------------------------------------
//Threads and processes

//The process a request is about: the one named, or else whichever the client last selected, or
//else the first one that is alive
static process_t *rsp_pick(int pid)
{
	if(pid > 0)
		return process_find(pid);

	//Whichever the client selected, if it is still there
	process_t *selected = (rsp_pid_g > 0) ? process_find(rsp_pid_g) : NULL;
	if(selected != NULL && selected->state == PROCESS_STATE_ALIVE)
		return selected;

	//Then whoever caused the stop being answered: a register read belongs to the thread that
	//stopped, not to whichever process happens to sit earliest in the table
	process_t *stopped = (rsp_stop_pid > 0) ? process_find(rsp_stop_pid) : NULL;
	if(stopped != NULL && stopped->state == PROCESS_STATE_ALIVE)
		return stopped;

	//Then the machine's own program, which is the lowest pid there is, so that a client that has
	//not picked a thread reads the registers of something that has run rather than a process
	//that has only just been made and has nothing in its registers yet
	process_t *lowest = NULL;
	for(int pp = 0; pp < PROCESS_MAX; pp++)
	{
		if(process_table[pp].state != PROCESS_STATE_ALIVE)
			continue;

		if(lowest == NULL || process_table[pp].pid < lowest->pid)
			lowest = &(process_table[pp]);
	}

	return lowest;
}

//Whether the given process exists as far as the client is concerned
static int rsp_thread_alive(int pid)
{
	process_t *pptr = rsp_pick(pid);
	if(pptr == NULL)
		return 0;
	if(pptr->state != PROCESS_STATE_ALIVE)
		return 0;
	return 1;
}

//Makes sure the machine is standing still, as a client in all-stop mode expects the whole target
//to be stopped when any part of it stops
static void rsp_stop_everyone(process_dbgstop_t reason, int except_pid)
{
	for(int pp = 0; pp < PROCESS_MAX; pp++)
	{
		if(process_table[pp].state != PROCESS_STATE_ALIVE)
			continue;
		if(process_table[pp].pid == except_pid)
			continue;
		if(process_table[pp].dbgstop != PROCESS_DBGSTOP_NONE)
			continue;

		process_table[pp].dbgstop = reason;
	}
}

//Reports that a thread has stopped, with the signal that stopped it
static void rsp_report_stop(int pid, int signal)
{
	RSPLOG("reporting stop of pid %d, signal %d\n", pid, signal);
	rsp_stop_pid = pid;
	rsp_stop_signal = signal;

	rsp_putpkt_start();
	rsp_putpkt_str("T");
	rsp_putpkt_hex8((unsigned char)signal);
	rsp_putpkt_str("thread:");
	rsp_putpkt_tid(pid);
	rsp_putpkt_str(";");
	rsp_putpkt_end();
}

//------------------------------------------------------------------------------------------------
//Registers
//
//GDB's idea of an ARM register file is sixteen general registers, eight obsolete floating-point
//accumulators of twelve bytes each, the floating-point status word and the program status word.
//The machine has no floating point unit, so those read as zero and writes to them are forgotten.
#define RSP_FPA_COUNT 8
#define RSP_FPA_BYTES 12
#define RSP_REG_FPS 24
#define RSP_REG_CPSR 25

//Number of bytes any register takes up in a register packet
static int rsp_reg_size(int regnum)
{
	if(regnum >= 0 && regnum < 16)
		return 4;
	if(regnum >= 16 && regnum < 16 + RSP_FPA_COUNT)
		return RSP_FPA_BYTES;
	if(regnum == RSP_REG_FPS)
		return 4;
	if(regnum == RSP_REG_CPSR)
		return 4;
	return 0;
}

//Reads the value of a register, as bytes in the order they go down the wire
static int rsp_reg_get(process_t *pptr, int regnum, unsigned char *out)
{
	const int size = rsp_reg_size(regnum);
	if(size == 0)
		return 0;

	memset(out, 0, (size_t)size);

	if(regnum >= 16 && regnum != RSP_REG_CPSR)
		return size;

	uint32_t val = 0;
	if(regnum < 16)
		val = pptr->regs[regnum];
	else
		val = pptr->cpsr;

	for(int bb = 0; bb < 4; bb++)
		out[bb] = (unsigned char)((val >> (8 * bb)) & 0xFF);

	return size;
}

//Writes the value of a register from bytes in wire order
static int rsp_reg_set(process_t *pptr, int regnum, const unsigned char *in)
{
	const int size = rsp_reg_size(regnum);
	if(size == 0)
		return 0;

	//The floating point registers are accepted and forgotten: there is no FPU to put them in
	if(regnum >= 16 && regnum != RSP_REG_CPSR)
		return size;

	uint32_t val = 0;
	for(int bb = 0; bb < 4 && bb < size; bb++)
		val |= ((uint32_t)in[bb]) << (8 * bb);

	if(regnum < 16)
		pptr->regs[regnum] = val;
	else
		pptr->cpsr = val;

	return size;
}

//------------------------------------------------------------------------------------------------
//Command handlers

//Command handler - query supported features
static void rsp_cmd_qsupported(const char *remain_buf, int remain_len)
{
	(void)remain_len;

	rsp_putpkt_start();
	rsp_putpkt_str("PacketSize=ff0");
	rsp_putpkt_str(";vContSupported+");     //Continue and step may be given per thread in one packet
	rsp_putpkt_str(";swbreak+;hwbreak+");    //Breakpoints are ours to place and to report
	rsp_putpkt_str(";error-message+");      //Failures can say why, in words
	rsp_putpkt_str(";qXfer:threads:read+"); //The thread list can be fetched as XML
	rsp_putpkt_str(";qXfer:exec-file:read+"); //and the file the program was loaded from
	//No feature is announced for vRun: a client offers the packets it knows and asks; naming a
	//packet here is a warning from gdb, which reads a feature as a name and a plus or minus
	rsp_putpkt_end();

	if(strstr(remain_buf, "error-message+") != NULL)
		rsp_errtext_supported = 1;
}

//Command handler - list of actions supported by vCont packet
static void rsp_cmd_vcontq(const char *remain_buf, int remain_len)
{
	(void)remain_buf;
	(void)remain_len;

	rsp_putpkt_start();
	rsp_putpkt_str("vCont;c;C;s;S;t");
	rsp_putpkt_end();
}

//Command handler - exclamation point (!) enables extended mode
static void rsp_cmd_exclam(const char *remain_buf, int remain_len)
{
	(void)remain_buf;
	(void)remain_len;

	rsp_reply_ok();
}

//Command handler - set thread ID for further operations
static void rsp_cmd_H(const char *remain_buf, int remain_len)
{
	if(remain_len < 1)
	{
		rsp_putpkt_errpkt("No operation specified for setting thread-ID.");
		return;
	}

	const char optype = remain_buf[0];
	remain_buf++;
	remain_len--;

	int *which = NULL;
	if(optype == 'c')
		which = &rsp_pid_c;
	else if(optype == 'g')
		which = &rsp_pid_g;
	else
	{
		rsp_putpkt_errpkt("Bad operation type when setting thread-ID.");
		return;
	}

	int pid = -1;
	rsp_parse_thread(remain_buf, remain_len, &pid);

	//Anything that names a process must name one that is there
	if(pid > 0 && !rsp_thread_alive(pid))
	{
		rsp_putpkt_errpkt("No such thread");
		return;
	}

	*which = pid;
	rsp_reply_ok();
}

//Returns subsequent thread information
static void rsp_cmd_qsthreadinfo(const char *remain_buf, int remain_len)
{
	(void)remain_buf;
	(void)remain_len;

	while(rsp_threadinfo_step < PROCESS_MAX)
	{
		const process_t *pptr = &(process_table[rsp_threadinfo_step]);
		rsp_threadinfo_step++;

		if(pptr->state != PROCESS_STATE_ALIVE)
			continue;

		rsp_putpkt_start();
		rsp_putpkt_str("m");
		rsp_putpkt_tid(pptr->pid);
		rsp_putpkt_end();
		return;
	}

	//That was the last of them
	rsp_putpkt_start();
	rsp_putpkt_str("l");
	rsp_putpkt_end();
}

//Returns first thread information
static void rsp_cmd_qfthreadinfo(const char *remain_buf, int remain_len)
{
	rsp_threadinfo_step = 0;
	rsp_cmd_qsthreadinfo(remain_buf, remain_len);
}

//Command handler - per thread information, which a client shows beside the thread
static void rsp_cmd_qthreadextrainfo(const char *remain_buf, int remain_len)
{
	int pid = 0;
	rsp_parse_thread(remain_buf, remain_len, &pid);

	process_t *pptr = rsp_pick(pid);
	if(pptr == NULL)
	{
		rsp_putpkt_errpkt("No such thread");
		return;
	}

	const char *state = "alive";
	if(pptr->dbgstop != PROCESS_DBGSTOP_NONE)
		state = "stopped";
	else if(pptr->paused && !pptr->unpaused)
		state = "blocked";

	rsp_putpkt_start();
	rsp_putpkt_hexprintf("PID %d, %s, PC 0x%8.8X", pptr->pid, state, (unsigned)(pptr->regs[15]));
	rsp_putpkt_end();
}

//Command handler - the thread list as XML, which is how a client learns thread names
static void rsp_cmd_qxfer_threads(int offset, int length)
{
	char xml[1024] = {0};
	int len = 0;
	len += snprintf(xml + len, sizeof(xml) - (size_t)len, "<?xml version=\"1.0\"?>\n<threads>\n");
	for(int pp = 0; pp < PROCESS_MAX; pp++)
	{
		const process_t *pptr = &(process_table[pp]);
		if(pptr->state != PROCESS_STATE_ALIVE)
			continue;

		len += snprintf(xml + len, sizeof(xml) - (size_t)len,
			"<thread id=\"%d\" name=\"pid %d\" core=\"0\"/>\n",
			pptr->pid, pptr->pid);
	}
	len += snprintf(xml + len, sizeof(xml) - (size_t)len, "</threads>\n");

	if(offset < 0 || offset > len || length < 0)
	{
		rsp_reply_empty();
		return;
	}

	//A client asks for a slice of it and is told whether more follows
	int count = len - offset;
	if(count > length)
		count = length;
	const int more = (offset + count < len);

	rsp_putpkt_start();
	rsp_putpkt_char(more ? 'm' : 'l');
	for(int cc = 0; cc < count; cc++)
		rsp_putpkt_char(xml[offset + cc]);
	rsp_putpkt_end();
}

//Command handler - whether the client started this program or attached to a running one
static void rsp_cmd_qattached(const char *remain_buf, int remain_len)
{
	(void)remain_buf;
	(void)remain_len;

	//The machine is always already running when a client connects: nothing here starts programs
	rsp_putpkt_start();
	rsp_putpkt_str("1");
	rsp_putpkt_end();
}

//Command handler - what the client is told about one process, when it asks about the machine's
//processes rather than about the threads in them. A process here has no parent, no working
//directory and no command line: it was loaded from an image and started.
static void rsp_cmd_qprocessinfo(const char *remain_buf, int remain_len)
{
	int pid = 0;
	rsp_parse_thread(remain_buf, remain_len, &pid);

	process_t *pptr = rsp_pick(pid);
	if(pptr == NULL)
	{
		rsp_putpkt_errpkt("No such process");
		return;
	}

	rsp_putpkt_start();
	rsp_putpkt_str("pid:");
	rsp_putpkt_num((unsigned)(pptr->pid));
	rsp_putpkt_str(";parent-pid:");
	rsp_putpkt_num(0);
	rsp_putpkt_end();
}

//Command handler - returns current thread ID
static void rsp_cmd_qc(const char *remain_buf, int remain_len)
{
	(void)remain_buf;
	(void)remain_len;

	rsp_putpkt_start();
	rsp_putpkt_str("QC");
	rsp_putpkt_tid(rsp_pid_g);
	rsp_putpkt_end();
}

//Command handler - offsets between the executable file and memory, of which there are none
static void rsp_cmd_qoffsets(const char *remain_buf, int remain_len)
{
	(void)remain_buf;
	(void)remain_len;

	rsp_putpkt_start();
	rsp_putpkt_str("Text=0;Data=0;Bss=0");
	rsp_putpkt_end();
}

//Command handler - symbol lookup, which the client does from its own files
static void rsp_cmd_qsymbol(const char *remain_buf, int remain_len)
{
	(void)remain_buf;
	(void)remain_len;

	rsp_reply_ok();
}

//Command handler - get reason for stop
static void rsp_cmd_quest(const char *remain_buf, int remain_len)
{
	(void)remain_buf;
	(void)remain_len;

	//Report the stop the machine is actually in, or else that it was interrupted
	int pid = rsp_stop_pid;
	if(pid <= 0 || !rsp_thread_alive(pid))
	{
		process_t *pptr = rsp_pick(rsp_pid_g);
		pid = pptr ? pptr->pid : 0;
	}

	rsp_report_stop(pid, rsp_stop_signal ? rsp_stop_signal : 5);
}

//Command handler - read all registers
static void rsp_cmd_g(const char *remain_buf, int remain_len)
{
	(void)remain_buf;
	(void)remain_len;

	process_t *pptr = rsp_pick(rsp_pid_g);
	if(pptr == NULL)
	{
		rsp_putpkt_errpkt("No such thread");
		return;
	}

	rsp_putpkt_start();
	for(int rr = 0; rr <= RSP_REG_CPSR; rr++)
	{
		unsigned char value[16] = {0};
		const int size = rsp_reg_get(pptr, rr, value);
		for(int bb = 0; bb < size; bb++)
			rsp_putpkt_hex8(value[bb]);
	}
	rsp_putpkt_end();
}

//Command handler - write all registers
static void rsp_cmd_G(const char *remain_buf, int remain_len)
{
	process_t *pptr = rsp_pick(rsp_pid_g);
	if(pptr == NULL)
	{
		rsp_putpkt_errpkt("No such thread");
		return;
	}

	int at = 0;
	for(int rr = 0; rr <= RSP_REG_CPSR; rr++)
	{
		const int size = rsp_reg_size(rr);
		unsigned char value[16] = {0};
		for(int bb = 0; bb < size; bb++)
		{
			unsigned char byte = 0;
			const int consumed = rsp_parse_hex8(remain_buf + at, remain_len - at, &byte);
			if(consumed <= 0)
			{
				rsp_putpkt_errpkt("Register packet is too short.");
				return;
			}
			value[bb] = byte;
			at += consumed;
		}
		rsp_reg_set(pptr, rr, value);
	}

	rsp_reply_ok();
}

//Command handler - read one register
static void rsp_cmd_pread(const char *remain_buf, int remain_len)
{
	int regnum = 0;
	rsp_parse_hex(remain_buf, remain_len, &regnum);

	process_t *pptr = rsp_pick(rsp_pid_g);
	if(pptr == NULL)
	{
		rsp_putpkt_errpkt("No such thread");
		return;
	}

	if(rsp_reg_size(regnum) == 0)
	{
		rsp_putpkt_errpkt("No such register 0x%X", regnum);
		return;
	}

	unsigned char value[16] = {0};
	const int size = rsp_reg_get(pptr, regnum, value);

	rsp_putpkt_start();
	for(int bb = 0; bb < size; bb++)
		rsp_putpkt_hex8(value[bb]);
	rsp_putpkt_end();
}

//Command handler - write one register
static void rsp_cmd_pwrite(const char *remain_buf, int remain_len)
{
	int regnum = 0;
	int consumed = rsp_parse_hex(remain_buf, remain_len, &regnum);
	remain_buf += consumed;
	remain_len -= consumed;

	if(remain_len < 1 || remain_buf[0] != '=')
	{
		rsp_putpkt_errpkt("Bad format in set-register command, no equals sign");
		return;
	}
	remain_buf++;
	remain_len--;

	process_t *pptr = rsp_pick(rsp_pid_g);
	if(pptr == NULL)
	{
		rsp_putpkt_errpkt("No such thread");
		return;
	}

	const int size = rsp_reg_size(regnum);
	if(size == 0)
	{
		rsp_putpkt_errpkt("No such register 0x%X", regnum);
		return;
	}

	unsigned char value[16] = {0};
	for(int bb = 0; bb < size; bb++)
	{
		unsigned char byte = 0;
		consumed = rsp_parse_hex8(remain_buf, remain_len, &byte);
		if(consumed <= 0)
			break;
		value[bb] = byte;
		remain_buf += consumed;
		remain_len -= consumed;
	}

	rsp_reg_set(pptr, regnum, value);
	rsp_reply_ok();
}

//Command handler - read memory
static void rsp_cmd_mread(const char *remain_buf, int remain_len)
{
	int address = 0;
	int consumed = rsp_parse_hex(remain_buf, remain_len, &address);
	remain_buf += consumed;
	remain_len -= consumed;

	int comma = 0;
	if(remain_len > 0 && remain_buf[0] == ',')
	{
		comma = 1;
		remain_buf++;
		remain_len--;
	}

	int length = 0;
	const int length_consumed = rsp_parse_hex(remain_buf, remain_len, &length);

	if(!(consumed && comma && length_consumed))
	{
		rsp_putpkt_errpkt_safe("Bad format of memory-read command.");
		return;
	}

	process_t *pptr = rsp_pick(rsp_pid_g);
	if(pptr == NULL)
	{
		rsp_putpkt_errpkt_safe("No such thread");
		return;
	}
	if(pptr->mem == NULL)
	{
		rsp_putpkt_errpkt_safe("Process had no memory space, possibly dead already");
		return;
	}

	if(address < 0 || (uint32_t)address >= pptr->size || length < 0)
	{
		rsp_putpkt_errpkt_safe("Memory read starts at invalid address %X", address);
		return;
	}

	//Read as many bytes as continue to be valid, which is how a client reads up to the end of a
	//region whose size it does not know
	rsp_putpkt_start();
	const unsigned char *membytes = (const unsigned char *)(pptr->mem);
	for(int aa = address; aa < address + length && (uint32_t)aa < pptr->size; aa++)
	{
		rsp_putpkt_hex8(membytes[aa]);
	}
	rsp_putpkt_end();
}

//Command handler - write memory, in hex ("M") or in binary ("X")
static void rsp_cmd_mwrite(const char *remain_buf, int remain_len, int binary)
{
	int address = 0;
	int consumed = rsp_parse_hex(remain_buf, remain_len, &address);
	remain_buf += consumed;
	remain_len -= consumed;

	if(remain_len < 1 || remain_buf[0] != ',')
	{
		rsp_putpkt_errpkt("Invalid format for memory-write command.");
		return;
	}
	remain_buf++;
	remain_len--;

	int length = 0;
	consumed = rsp_parse_hex(remain_buf, remain_len, &length);
	remain_buf += consumed;
	remain_len -= consumed;

	if(remain_len < 1 || remain_buf[0] != ':')
	{
		rsp_putpkt_errpkt("Invalid format for memory-write command.");
		return;
	}
	remain_buf++;
	remain_len--;

	process_t *pptr = rsp_pick(rsp_pid_g);
	if(pptr == NULL)
	{
		rsp_putpkt_errpkt("No such thread");
		return;
	}
	if(pptr->mem == NULL)
	{
		rsp_putpkt_errpkt("Process has no memory - already dead, perhaps.");
		return;
	}
	if(address < 0 || (uint32_t)address >= pptr->size || length < 0)
	{
		rsp_putpkt_errpkt("Memory write is outside size of process memory.");
		return;
	}

	unsigned char *membytes = (unsigned char *)(pptr->mem);
	while(length > 0 && (uint32_t)address < pptr->size)
	{
		unsigned char data = 0;
		if(binary)
		{
			if(remain_len < 1)
				break;
			data = (unsigned char)remain_buf[0];
			remain_buf++;
			remain_len--;
		}
		else
		{
			consumed = rsp_parse_hex8(remain_buf, remain_len, &data);
			if(consumed <= 0)
				break;
			remain_buf += consumed;
			remain_len -= consumed;
		}

		membytes[address] = data;
		address++;
		length--;
	}

	rsp_reply_ok();
}

//Command handler - set or clear a breakpoint
static void rsp_cmd_breakpoint(const char *remain_buf, int remain_len, int set)
{
	int type = 0;
	int consumed = rsp_parse_hex(remain_buf, remain_len, &type);
	remain_buf += consumed;
	remain_len -= consumed;

	if(remain_len < 1 || remain_buf[0] != ',')
	{
		rsp_putpkt_errpkt("Bad format in breakpoint command.");
		return;
	}
	remain_buf++;
	remain_len--;

	int address = 0;
	int consumed_address = rsp_parse_hex(remain_buf, remain_len, &address);
	remain_buf += consumed_address;
	remain_len -= consumed_address;

	//Only execution breakpoints make sense here: there is no MMU to watch an address with, and
	//the emulator already stops on the instructions a program is about to run
	if(type != 0 && type != 1)
	{
		rsp_reply_empty();
		return;
	}

	//A breakpoint is for whichever process reaches it, unless the client names one: a client in
	//all-stop mode asks for the address and nothing else, and means the program it is about to
	//run, which is not always the process that happens to be selected at the time.
	int pid = 0;
	if(remain_len >= 2 && remain_buf[0] == ',' && remain_buf[1] == ';')
	{
		int named = 0;
		rsp_parse_thread(remain_buf + 2, remain_len - 2, &named);
		if(named > 0)
			pid = named;
	}

	RSPLOG("%c%d,%08X owner %d\n", set ? 'Z' : 'z', type, (unsigned)address, pid);

	if(set)
	{
		if(process_bp_add_owner((uint32_t)address, pid) < 0 && process_bp_count >= PROCESS_BP_MAX)
		{
			rsp_putpkt_errpkt("No room for another breakpoint.");
			return;
		}

		//Remember it too, so that it survives the machine being restarted under the client
		int known = 0;
		for(int bb = 0; bb < rsp_bp_count; bb++)
		{
			if(rsp_bp_addr[bb] == (uint32_t)address)
				known = 1;
		}
		if(!known && rsp_bp_count < RSP_BP_MAX)
			rsp_bp_addr[rsp_bp_count++] = (uint32_t)address;
	}
	else
	{
		process_bp_del_owner((uint32_t)address, pid);

		for(int bb = 0; bb < rsp_bp_count; bb++)
		{
			if(rsp_bp_addr[bb] == (uint32_t)address)
			{
				rsp_bp_addr[bb] = rsp_bp_addr[rsp_bp_count - 1];
				rsp_bp_count--;
				break;
			}
		}
	}

	rsp_reply_ok();
}

//Starts the processes a continue or step named, and remembers a step to carry out
static void rsp_do_continue(int pid, int step)
{
	//A request that names no process means every process
	if(pid == 0)
		pid = rsp_pid_c;

	if(step)
	{
		//Stand the thread still where it is first, so that the step is one instruction from
		//here rather than however far it gets before the next poll
		if(pid > 0)
		{
			process_t *pptr = process_find(pid);
			if(pptr != NULL)
				pptr->dbgstop = PROCESS_DBGSTOP_CTRLC;
		}
		else
		{
			rsp_stop_everyone(PROCESS_DBGSTOP_CTRLC, 0);
		}

		rsp_step_pid = (pid > 0) ? pid : 0;
		rsp_step_count = 1;
		return;
	}

	//A client resuming a thread expects the machine to run, even if the window paused it
	RSPLOG("continue pid %d (selected %d)\n", pid, rsp_pid_c);

	if(rsp_continue_hook != NULL)
		(*rsp_continue_hook)();

	if(pid > 0)
	{
		process_t *pptr = process_find(pid);
		if(pptr != NULL)
		{
			pptr->dbgstop = PROCESS_DBGSTOP_NONE;
			return;
		}

		//A client can name a process the machine no longer has - it started the machine again
		//on its own account, or the process finished while the client was looking elsewhere.
		//Resuming nothing would leave the machine standing still with the client waiting for a
		//stop that can never come, so the machine runs and the client is told what it finds.
		RSPLOG("continue named pid %d, which is not running; continuing the machine\n", pid);
	}

	for(int pp = 0; pp < PROCESS_MAX; pp++)
	{
		if(process_table[pp].state == PROCESS_STATE_ALIVE)
			process_table[pp].dbgstop = PROCESS_DBGSTOP_NONE;
	}
}

//Command handler - continue or step, from the plain packets that carry an optional signal
static void rsp_cmd_continue(const char *remain_buf, int remain_len, int step)
{
	//A signal number may follow, which is ignored: the emulator has nothing to deliver
	int signal = 0;
	rsp_parse_hex(remain_buf, remain_len, &signal);

	rsp_do_continue(0, step);
}

//Stops a thread where it is
static void rsp_do_stop(int pid)
{
	if(pid <= 0)
	{
		rsp_stop_everyone(PROCESS_DBGSTOP_CTRLC, 0);
		return;
	}

	process_t *pptr = process_find(pid);
	if(pptr != NULL && pptr->dbgstop == PROCESS_DBGSTOP_NONE)
		pptr->dbgstop = PROCESS_DBGSTOP_CTRLC;
}

//Command handler - continue and step, with a list of what each thread should do
static void rsp_cmd_vcont(const char *remain_buf, int remain_len)
{
	int stopped = 0;

	while(remain_len > 0)
	{
		if(remain_buf[0] != ';')
		{
			rsp_putpkt_errpkt("Bad format in vCont packet, no semicolon");
			return;
		}
		remain_buf++;
		remain_len--;

		if(remain_len < 1)
		{
			rsp_putpkt_errpkt("Bad format in vCont packet, no action");
			return;
		}

		const char action = remain_buf[0];
		remain_buf++;
		remain_len--;

		if(action == 'C' || action == 'S')
		{
			//Signals are accepted and ignored
			int signal = 0;
			const int consumed = rsp_parse_hex(remain_buf, remain_len, &signal);
			remain_buf += consumed;
			remain_len -= consumed;
		}
		else if(action != 'c' && action != 's' && action != 't')
		{
			rsp_putpkt_errpkt("Unknown action in vCont packet");
			return;
		}

		int pid = -1;
		if(remain_len > 0 && remain_buf[0] == ':')
		{
			remain_buf++;
			remain_len--;

			const int consumed = rsp_parse_thread(remain_buf, remain_len, &pid);
			remain_buf += consumed;
			remain_len -= consumed;
		}

		if(action == 'c' || action == 'C')
			rsp_do_continue(pid, 0);
		else if(action == 's' || action == 'S')
			rsp_do_continue(pid, 1);
		else
		{
			rsp_do_stop(pid);
			stopped = 1;
		}
	}

	//A stop request is answered at once; a continue is answered when something stops
	if(stopped)
	{
		process_t *pptr = rsp_pick(rsp_pid_g);
		rsp_report_stop(pptr ? pptr->pid : 0, 2);
	}
}

//Command handler - detach
static void rsp_cmd_detach(const char *remain_buf, int remain_len)
{
	(void)remain_buf;
	(void)remain_len;

	//Whatever the client did to the machine's processes is undone, so that they run again
	for(int pp = 0; pp < PROCESS_MAX; pp++)
		process_table[pp].dbgstop = PROCESS_DBGSTOP_NONE;

	rsp_step_pid = 0;
	rsp_step_count = 0;

	rsp_reply_ok();
}

//Command handler - kill, meaning the client is done with the machine
static void rsp_cmd_kill(const char *remain_buf, int remain_len)
{
	rsp_cmd_detach(remain_buf, remain_len);
}

//Command handler - end one process
static void rsp_cmd_vkill(const char *remain_buf, int remain_len)
{
	int pid = 0;
	rsp_parse_thread(remain_buf, remain_len, &pid);

	process_t *pptr = (pid > 0) ? process_find(pid) : rsp_pick(0);
	if(pptr == NULL)
	{
		rsp_putpkt_errpkt("No such thread");
		return;
	}

	pptr->dbgstop = PROCESS_DBGSTOP_NONE;
	pptr->state = PROCESS_STATE_DEAD;

	rsp_reply_ok();
}

//Command handler - restart the machine
//Puts the client's breakpoints back into the machine, for whichever process reaches them. A
//breakpoint survives the restart of the machine that threw away the process table: it is an
//address in the program the client is about to run again, and the client has no way of knowing
//that anything happened to it.
void rsp_after_reset(void)
{
	RSPLOG("machine restarted; putting back %d breakpoint(s)\n", rsp_bp_count);

	for(int bb = 0; bb < rsp_bp_count; bb++)
		process_bp_add_owner(rsp_bp_addr[bb], 0);

	//The processes the client had selected and stopped belong to the machine that has just gone.
	//Keeping them would leave the client's idea of the current thread pointing at a pid that is
	//either gone or belongs to some other program entirely.
	rsp_pid_g = -1;
	rsp_pid_c = -1;
	rsp_stop_pid = 0;
	rsp_stop_signal = 0;
	rsp_step_pid = 0;
	rsp_step_count = 0;

	//A machine that has just been started again stands still for a client that is debugging it,
	//so that the breakpoints can go in before the new program runs past them. Without this the
	//machine is away the moment it is restarted and a client that asks for a build to be run
	//never sees it stop where it asked.
	if(rsp_present())
		rsp_stop_everyone(PROCESS_DBGSTOP_CTRLC, 0);
}

//Stops the machine with a program loaded and reports the first process, which is what a client
//that has just asked for a program to be run expects to hear back
static void rsp_report_new_program(void)
{
	rsp_step_pid = 0;
	rsp_step_count = 0;

	rsp_stop_everyone(PROCESS_DBGSTOP_CTRLC, 0);

	int pid = 0;
	process_t *pptr = rsp_pick(0);
	if(pptr != NULL)
		pid = pptr->pid;

	rsp_report_stop(pid, 5);
}

//Command handler - run a program, which is how a client starts a new build without the emulator
//being restarted underneath it
static void rsp_cmd_vrun(const char *remain_buf, int remain_len)
{
	//vRun;<file>[;argument;...] - the file comes hex-encoded, as everything in this packet does
	char path[1024] = {0};
	if(remain_len > 0 && remain_buf[0] == ';')
	{
		remain_buf++;
		remain_len--;

		int name_len = 0;
		while(name_len < remain_len && remain_buf[name_len] != ';')
			name_len++;

		int out_len = 0;
		for(int cc = 0; cc + 1 < name_len && out_len < (int)sizeof(path) - 1; cc += 2)
		{
			unsigned char byte_val = 0;
			if(rsp_parse_hex8(remain_buf + cc, 2, &byte_val) <= 0)
				break;
			path[out_len++] = (char)byte_val;
		}
	}

	RSPLOG("vRun \"%s\"\n", path);

	if(rsp_hook_load == NULL)
	{
		rsp_putpkt_errpkt("This emulator cannot be given a program to run");
		return;
	}

	char err[512] = {0};
	if(rsp_hook_load(path, err, sizeof(err)) != 0)
	{
		rsp_putpkt_errpkt("%s", err[0] ? err : "Cannot run that program");
		return;
	}

	RSPLOG("client asked for program \"%s\"\n", path);
	rsp_report_new_program();
}

//Command handler - the file the client thinks the program came from, so that it can show which
//build is loaded and default "run" to it
static void rsp_cmd_qxfer_execfile(const char *remain_buf, int remain_len)
{
	//qXfer:exec-file:read:<annex>:<offset>,<length>
	const char *args = remain_buf;
	int args_len = remain_len;

	if(args_len > 0 && args[0] == ':')
	{
		args++;
		args_len--;
	}

	//The annex names the process; any process will do, since they all came from the same image
	while(args_len > 0 && args[0] != ':')
	{
		args++;
		args_len--;
	}

	int offset = 0;
	int length = 0;
	if(args_len > 0 && args[0] == ':')
	{
		args++;
		args_len--;

		int consumed = rsp_parse_hex(args, args_len, &offset);
		args += consumed;
		args_len -= consumed;

		if(args_len > 0 && args[0] == ',')
		{
			args++;
			args_len--;
			rsp_parse_hex(args, args_len, &length);
		}
	}

	const char *image = (rsp_hook_image != NULL) ? rsp_hook_image() : NULL;
	if(image == NULL || image[0] == '\0')
	{
		rsp_putpkt_errpkt("No image is loaded");
		return;
	}

	const int image_len = (int)strlen(image);
	if(offset < 0 || offset >= image_len)
	{
		//Past the end: the client has the whole of it
		rsp_putpkt_start();
		rsp_putpkt_char('l');
		rsp_putpkt_end();
		return;
	}

	if(length <= 0 || length > image_len - offset)
		length = image_len - offset;

	rsp_putpkt_start();
	rsp_putpkt_char((offset + length < image_len) ? 'm' : 'l');
	for(int cc = 0; cc < length; cc++)
		rsp_putpkt_char(image[offset + cc]);
	rsp_putpkt_end();
}

//Command handler - restart the machine, which is what a client did before "run" existed
static void rsp_cmd_restart(const char *remain_buf, int remain_len)
{
	(void)remain_buf;
	(void)remain_len;

	char err[512] = {0};
	if(rsp_hook_load != NULL && rsp_hook_load("", err, sizeof(err)) != 0)
	{
		rsp_putpkt_errpkt("%s", err[0] ? err : "Cannot restart the program");
		return;
	}

	if(rsp_hook_load == NULL)
	{
		process_reset();
		rsp_after_reset();
	}

	rsp_report_new_program();
}


//Command handler - check if thread is alive
static void rsp_cmd_talive(const char *remain_buf, int remain_len)
{
	int pid = 0;
	rsp_parse_thread(remain_buf, remain_len, &pid);

	if(rsp_thread_alive(pid))
	{
		rsp_reply_ok();
		return;
	}

	//An error is how a client is told that a thread has gone
	rsp_putpkt_errpkt("No such thread");
}

//------------------------------------------------------------------------------------------------
//Remote monitor commands

//Resets the process table as if booting a game
static void rsp_rcmd_prep(const char *args)
{
	(void)args;

	rsp_putpkt_start();
	rsp_putpkt_hexprintf("Resetting process table...\n");
	rsp_putpkt_end();

	//Clear memory for game process
	if(process_table[0].mem != NULL)
	{
		free(process_table[0].mem);
		process_table[0].mem = NULL;
	}
	if(process_table[0].mexec_mem != NULL)
	{
		free(process_table[0].mexec_mem);
		process_table[0].mexec_mem = NULL;
	}

	memset(&(process_table[0]), 0, sizeof(process_table[0]));

	process_table[0].mem = (uint32_t*)calloc(1, 24*1024*1024);
	process_table[0].size = 24*1024*1024;

	//Copy environment from init process
	memcpy(process_table[0].env_buf, process_table[1].env_buf, sizeof(process_table[0].env_buf));
	process_table[0].env_len = process_table[1].env_len;

	//Reset identity
	process_table[0].pid = PROCESS_MAX;
	process_table[0].ppid = 1;

	//Stop process at entry point
	process_table[0].regs[15] = 0x1000;
	process_table[0].dbgstop = PROCESS_DBGSTOP_CTRLC;
	process_table[0].state = PROCESS_STATE_ALIVE;

	//Nuke all other processes
	for(int pp = 2; pp < PROCESS_MAX; pp++)
	{
		if(process_table[pp].mem != NULL)
		{
			free(process_table[pp].mem);
			process_table[pp].mem = NULL;
		}
		if(process_table[pp].mexec_mem != NULL)
		{
			free(process_table[pp].mexec_mem);
			process_table[pp].mexec_mem = NULL;
		}

		memset(&(process_table[pp]), 0, sizeof(process_table[pp]));
	}

	//Reset active PIDs
	rsp_pid_c = -1;
	rsp_pid_g = -1;
}

//Lists the emulated processes and how each is getting on
static void rsp_rcmd_threads(const char *args)
{
	(void)args;

	static const char *const states[] = { "none", "alive", "dead" };

	rsp_putpkt_start();
	for(int pp = 0; pp < PROCESS_MAX; pp++)
	{
		const process_t *pptr = &(process_table[pp]);
		if(pptr->state == PROCESS_STATE_NONE)
			continue;

		const char *state = "?";
		if((int)(pptr->state) >= 0 && (int)(pptr->state) < (int)(sizeof(states)/sizeof(states[0])))
			state = states[pptr->state];

		rsp_putpkt_hexprintf("pid %d, ppid %d, %s, pc 0x%8.8X, dbgstop %d\n",
			pptr->pid, pptr->ppid, state, (unsigned)(pptr->regs[15]), (int)(pptr->dbgstop));
	}
	rsp_putpkt_end();
}

//Starts a program the client names, or the one already loaded when it names none
static void rsp_rcmd_run(const char *args)
{
	char path[1024] = {0};
	snprintf(path, sizeof(path), "%s", args != NULL ? args : "");

	//Ours is not to say whether the file is there: the loader is the one that opens it
	if(rsp_hook_load == NULL)
	{
		rsp_putpkt_start();
		rsp_putpkt_hexprintf("This emulator cannot be given a program to run\n");
		rsp_putpkt_end();
		return;
	}

	char err[512] = {0};
	if(rsp_hook_load(path, err, sizeof(err)) != 0)
	{
		rsp_putpkt_start();
		rsp_putpkt_hexprintf("%s\n", err[0] ? err : "Cannot run that program");
		rsp_putpkt_end();
		return;
	}

	const char *image = (rsp_hook_image != NULL) ? rsp_hook_image() : NULL;
	rsp_putpkt_start();
	rsp_putpkt_hexprintf("Running %s\n", (image != NULL && image[0]) ? image : "(nothing)");
	rsp_putpkt_end();
}

//Starts the image that is loaded again, from the file as it is now: the loop a client is in when
//it is rebuilding between runs
static void rsp_rcmd_reload(const char *args)
{
	(void)args;
	rsp_rcmd_run("");
}

//Says which image the machine is running
static void rsp_rcmd_image(const char *args)
{
	(void)args;

	const char *image = (rsp_hook_image != NULL) ? rsp_hook_image() : NULL;
	rsp_putpkt_start();
	rsp_putpkt_hexprintf("%s\n", (image != NULL && image[0]) ? image : "(no image loaded)");
	rsp_putpkt_end();
}

//Remote monitor command ("monitor ...") decoding table
typedef struct rsp_rcmd_s
{
	const char *cmd;
	const char *help;
	void (*func)(const char *args);
} rsp_rcmd_t;
static const rsp_rcmd_t rsp_rcmd_table[] =
{
	{ .cmd = "prep", .help = "Resets process-table as if booting a game", .func = rsp_rcmd_prep },
	{ .cmd = "threads", .help = "Lists the emulated processes", .func = rsp_rcmd_threads },
	{ .cmd = "run", .help = "run <file> - boots an image, or runs an .nne on its own", .func = rsp_rcmd_run },
	{ .cmd = "reload", .help = "Starts the loaded image again, as the file is now", .func = rsp_rcmd_reload },
	{ .cmd = "image", .help = "Says which image is loaded", .func = rsp_rcmd_image },
	{}
};

//Command handler - "monitor" remote commands
static void rsp_cmd_rcmd(const char *remain_buf, int remain_len)
{
	//The whole command is hex-encoded text; the verb is the first word, and whatever follows it
	//is its argument, such as the file another program should be loaded from
	char decoded_cmd[1024] = {0};
	int decoded_len = 0;
	while(remain_len > 0 && decoded_len < (int)sizeof(decoded_cmd) - 1)
	{
		unsigned char byte_val = 0;
		int byte_consumed = rsp_parse_hex8(remain_buf, remain_len, &byte_val);
		if(byte_consumed <= 0)
			break;
		remain_buf += byte_consumed;
		remain_len -= byte_consumed;

		decoded_cmd[decoded_len] = (char)byte_val;
		decoded_len++;
	}

	char *args = NULL;
	for(int cc = 0; cc < decoded_len; cc++)
	{
		if(decoded_cmd[cc] == ' ' || decoded_cmd[cc] == '	')
		{
			decoded_cmd[cc] = '\0';
			args = decoded_cmd + cc + 1;
			break;
		}
	}

	//Trailing whitespace is the client's newline, not part of the name of anything
	if(args != NULL)
	{
		const int args_len = (int)strlen(args);
		for(int cc = args_len - 1; cc >= 0; cc--)
		{
			if(args[cc] == ' ' || args[cc] == '	' || args[cc] == '\r' || args[cc] == '\n')
				args[cc] = '\0';
			else
				break;
		}
	}

	if(!strcmp(decoded_cmd, "help") || decoded_cmd[0] == '\0')
	{
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

	for(const rsp_rcmd_t *cc = rsp_rcmd_table; cc->cmd != NULL; cc++)
	{
		if(!strcmp(decoded_cmd, cc->cmd))
		{
			(cc->func)(args);
			return;
		}
	}

	rsp_putpkt_start();
	rsp_putpkt_hexprintf("Unknown monitor command (try \"monitor help\")\n");
	rsp_putpkt_end();
}

//------------------------------------------------------------------------------------------------
//Command decoding

typedef struct rsp_cmd_s
{
	const char *prefix;
	void (*handler)(const char *remain_buf, int remain_len);
} rsp_cmd_t;

//The prefix here is everything a packet starts with, up to and including any separator, because
//some packets separate their arguments with a colon and some with a comma
static const rsp_cmd_t rsp_cmd_table[] =
{
	{ .prefix = "qThreadExtraInfo,",     .handler = rsp_cmd_qthreadextrainfo },
	{ .prefix = "qfThreadInfo",          .handler = rsp_cmd_qfthreadinfo },
	{ .prefix = "qsThreadInfo",          .handler = rsp_cmd_qsthreadinfo },
	{ .prefix = "qSupported",            .handler = rsp_cmd_qsupported },
	{ .prefix = "qAttached",             .handler = rsp_cmd_qattached },
	{ .prefix = "qProcessInfo:",         .handler = rsp_cmd_qprocessinfo },
	{ .prefix = "qOffsets",              .handler = rsp_cmd_qoffsets },
	{ .prefix = "qSymbol:",              .handler = rsp_cmd_qsymbol },
	{ .prefix = "qRcmd,",                .handler = rsp_cmd_rcmd },
	{ .prefix = "qXfer:threads:read:",   .handler = NULL },
	{ .prefix = "qXfer:exec-file:read",  .handler = rsp_cmd_qxfer_execfile },
	{ .prefix = "vRun",                  .handler = rsp_cmd_vrun },
	{ .prefix = "vCont?",                .handler = rsp_cmd_vcontq },
	{ .prefix = "vCont",                 .handler = rsp_cmd_vcont },
	{ .prefix = "vKill;",                .handler = rsp_cmd_vkill },
	{ .prefix = "qC",                    .handler = rsp_cmd_qc },
	{ .prefix = "H",                     .handler = rsp_cmd_H },
	{ .prefix = "?",                     .handler = rsp_cmd_quest },
	{ .prefix = "g",                     .handler = rsp_cmd_g },
	{ .prefix = "G",                     .handler = rsp_cmd_G },
	{ .prefix = "m",                     .handler = rsp_cmd_mread },
	{ .prefix = "p",                     .handler = rsp_cmd_pread },
	{ .prefix = "P",                     .handler = rsp_cmd_pwrite },
	{ .prefix = "D",                     .handler = rsp_cmd_detach },
	{ .prefix = "T",                     .handler = rsp_cmd_talive },
	{ .prefix = "!",                     .handler = rsp_cmd_exclam },
	{ .prefix = "k",                     .handler = rsp_cmd_kill },
	{ .prefix = "R",                     .handler = rsp_cmd_restart },
	{  }, //Sentinel
};

//Command decoding - top level
static void rsp_cmd(const char *cmd_buf, int cmd_len)
{
	TDEBUG("RSP command received: %.*s\n", cmd_len, cmd_buf);

	//Commands whose name is a single character, which the table cannot separate from longer
	//ones that begin with it
	if(cmd_len >= 1 && (cmd_buf[0] == 'C' || cmd_buf[0] == 'S'))
	{
		rsp_cmd_continue(cmd_buf + 1, cmd_len - 1, cmd_buf[0] == 'S');
		return;
	}
	if(cmd_len >= 1 && (cmd_buf[0] == 'c' || cmd_buf[0] == 's'))
	{
		rsp_cmd_continue(cmd_buf + 1, cmd_len - 1, cmd_buf[0] == 's');
		return;
	}
	if(cmd_len >= 2 && (cmd_buf[0] == 'Z' || cmd_buf[0] == 'z'))
	{
		rsp_cmd_breakpoint(cmd_buf + 1, cmd_len - 1, cmd_buf[0] == 'Z');
		return;
	}
	if(cmd_len >= 2 && (cmd_buf[0] == 'M' || cmd_buf[0] == 'X'))
	{
		rsp_cmd_mwrite(cmd_buf + 1, cmd_len - 1, cmd_buf[0] == 'X');
		return;
	}

	//Threads as XML carry their offset and length after a second colon
	if(cmd_len > 19 && !memcmp(cmd_buf, "qXfer:threads:read:", 19))
	{
		int offset = 0;
		int length = 0;
		const char *args = cmd_buf + 19;
		int args_len = cmd_len - 19;
		if(args_len > 0 && args[0] == ':')
		{
			args++;
			args_len--;

			int consumed = rsp_parse_hex(args, args_len, &offset);
			args += consumed;
			args_len -= consumed;

			if(args_len > 0 && args[0] == ',')
			{
				args++;
				args_len--;
				rsp_parse_hex(args, args_len, &length);
				rsp_cmd_qxfer_threads(offset, length ? length : 0x1000);
				return;
			}
		}

		rsp_reply_empty();
		return;
	}

	for(int tt = 0; rsp_cmd_table[tt].prefix != NULL; tt++)
	{
		const char *prefix = rsp_cmd_table[tt].prefix;
		const int prefix_len = (int)strlen(prefix);
		if(prefix_len == 0 || rsp_cmd_table[tt].handler == NULL)
			continue;

		if(cmd_len >= prefix_len && !memcmp(cmd_buf, prefix, (size_t)prefix_len))
		{
			(*rsp_cmd_table[tt].handler)(cmd_buf + prefix_len, cmd_len - prefix_len);
			return;
		}
	}

	//Other, empty, or unknown command - respond with empty response
	rsp_reply_empty();
}

//Removes the escaping a client applies to the binary data in a packet
static int rsp_unescape(const char *in, int in_len, char *out, int out_size)
{
	int out_len = 0;
	for(int aa = 0; aa < in_len; aa++)
	{
		char bb = in[aa];
		if(bb == '}' && aa + 1 < in_len)
		{
			aa++;
			bb = (char)(in[aa] ^ 0x20);
		}

		if(out_len >= out_size - 1)
			break;

		out[out_len] = bb;
		out_len++;
	}

	out[out_len] = '\0';
	return out_len;
}

//Handles incoming bytes from RSP client (i.e. GDB)
static void rsp_gotbyte(char byte_in)
{
	if(byte_in == '$')
	{
		//Dollarsign always starts a new packet
		rsp_incoming_len = 0;
		rsp_incoming_buf[0] = '$';
		rsp_incoming_len = 1;
		return;
	}

	if(rsp_incoming_len == 0)
	{
		if(byte_in == '+' || byte_in == '-')
		{
			//The client has seen the reply to its last packet, so anything it sends now is new
			return;
		}

		//If GDB sent us a Ctrl-C, react with a stop reply
		if(byte_in == 0x03)
		{
			TDEBUG("%s", "Client asked the machine to stop.\n");
			rsp_stop_everyone(PROCESS_DBGSTOP_CTRLC, 0);
			rsp_step_pid = 0;
			rsp_step_count = 0;

			process_t *pptr = rsp_pick(rsp_pid_g);
			rsp_report_stop(pptr ? pptr->pid : 0, 2);
			return;
		}

		//Process nothing else until we see a dollarsign to start the packet
		return;
	}

	if(rsp_incoming_len >= (int)sizeof(rsp_incoming_buf) - 1)
	{
		//Overlong packet, discard and tell the client
		rsp_incoming_len = 0;
		rsp_putbyte('-');
		return;
	}

	//Append the new character to the received packet
	rsp_incoming_buf[rsp_incoming_len] = byte_in;
	rsp_incoming_len++;

	if(rsp_incoming_len >= 4 && rsp_incoming_buf[rsp_incoming_len-3] == '#')
	{
		//Got the terminating '#' and checksum for a packet. See if the packet is valid.
		unsigned char computed_checksum = 0;
		for(int bb = 1; bb < rsp_incoming_len - 3; bb++) //Skip leading '$' and trailing '#NN'
			computed_checksum = (unsigned char)(computed_checksum + (unsigned char)rsp_incoming_buf[bb]);

		unsigned char stated_checksum = 0;
		for(int cc = rsp_incoming_len - 2; cc < rsp_incoming_len; cc++)
		{
			stated_checksum = (unsigned char)(stated_checksum << 4);
			const char hexchar = rsp_incoming_buf[cc];
			if(hexchar >= '0' && hexchar <= '9')
				stated_checksum = (unsigned char)(stated_checksum + (hexchar - '0'));
			else if(hexchar >= 'A' && hexchar <= 'F')
				stated_checksum = (unsigned char)(stated_checksum + 10 + (hexchar - 'A'));
			else if(hexchar >= 'a' && hexchar <= 'f')
				stated_checksum = (unsigned char)(stated_checksum + 10 + (hexchar - 'a'));
		}

		if(computed_checksum == stated_checksum)
		{
			const int body_len = rsp_incoming_len - 4;
			const int decoded_len = rsp_unescape(rsp_incoming_buf + 1, body_len,
				rsp_packet_buf, (int)sizeof(rsp_packet_buf));

			//Packet seems valid, acknowledge it and carry it out. Every packet is carried out,
			//including one that repeats the last: a client has no way of telling a retransmission
			//from a command it means twice, and neither have we. Caching the last answer looked
			//like it made a repeated continue safe, and instead it silently dropped the second
			//half of every pair of identical commands - two reloads, two breakpoints on the same
			//address - which is a debugging session that stops at nothing and says nothing.
			rsp_putbyte('+');
			rsp_cmd(rsp_packet_buf, decoded_len);
		}
		else
		{
			//Packet seems invalid, nack it so that the client sends it again
			rsp_putbyte('-');
		}

		//Done with the packet
		rsp_incoming_len = 0;
		return;
	}
}

//Runs the one instruction a single-step asked for, between slices of emulated time so that the
//process can be stopped again before it runs any further
static void rsp_serve_step(void)
{
	if(rsp_step_pid < 0 || rsp_step_count <= 0)
		return;

	process_t *pptr = (rsp_step_pid > 0) ? process_find(rsp_step_pid) : rsp_pick(0);
	if(pptr == NULL || pptr->state != PROCESS_STATE_ALIVE || pptr->mem == NULL)
	{
		//The process it was stepping has gone
		rsp_step_pid = 0;
		rsp_step_count = 0;
		return;
	}

	process_run_insn(pptr->pid);
	pptr->dbgstop = PROCESS_DBGSTOP_CTRLC;

	rsp_step_count--;
	if(rsp_step_count <= 0)
	{
		rsp_step_pid = 0;
		rsp_report_stop(pptr->pid, 5);
	}
}

void rsp_init(const prefs_t *prefs)
{
	//Close existing sockets if any
	if(rsp_listen_sock != INVALID_SOCKET)
	{
		rsp_c_close(rsp_listen_sock);
		rsp_listen_sock = INVALID_SOCKET;
	}

	if(rsp_client_sock != INVALID_SOCKET)
	{
		rsp_c_close(rsp_client_sock);
		rsp_client_sock = INVALID_SOCKET;
	}

	if(!prefs->rsp_enabled)
	{
		TWARNING("%s", "GDB-RSP listener disabled in preferences. Not starting.\n");
		return;
	}

	rsp_port = prefs->rsp_port;
	TINFO("Setting up GDB-RSP listener on port %d\n", rsp_port);
}

void rsp_poll(void)
{
	if(rsp_port <= 0)
	{
		//RSP not configured
		return;
	}

	//Handle connection setup/acceptance
	if(rsp_client_sock == INVALID_SOCKET && rsp_listen_sock == INVALID_SOCKET)
	{
		struct addrinfo hints;
		memset(&hints, 0, sizeof(hints));
		hints.ai_family = AF_INET; //IPv4
		hints.ai_socktype = SOCK_STREAM; //TCP
		hints.ai_flags = AI_PASSIVE; //Fill in local IP automatically

		char portstr[16] = {0};
		snprintf(portstr, sizeof(portstr)-1, "%d", rsp_port);

		struct addrinfo *result_addr = NULL;
		int gai_result = rsp_c_getaddrinfo(NULL, portstr, &hints, &result_addr);
		if(gai_result < 0 || result_addr == NULL)
		{
			rsp_fatal("Failed to get address information for local port: %s\n", rsp_c_errstr());
			return;
		}

		rsp_listen_sock = rsp_c_socket(result_addr->ai_family, result_addr->ai_socktype, result_addr->ai_protocol);
		if(rsp_listen_sock == INVALID_SOCKET)
		{
			rsp_fatal("Failed to make listener socket: %s\n", rsp_c_errstr());
			rsp_c_freeaddrinfo(result_addr);
			return;
		}

		if(rsp_c_nonblock(rsp_listen_sock) < 0)
		{
			rsp_fatal("Failed to set listener socket to nonblocking: %s\n", rsp_c_errstr());
			rsp_c_freeaddrinfo(result_addr);
			rsp_c_close(rsp_listen_sock);
			rsp_listen_sock = INVALID_SOCKET;
			return;
		}

		if(rsp_c_bind(rsp_listen_sock, result_addr->ai_addr, result_addr->ai_addrlen) < 0)
		{
			rsp_fatal("Failed to bind to local port: %s\n", rsp_c_errstr());
			rsp_c_freeaddrinfo(result_addr);
			rsp_c_close(rsp_listen_sock);
			rsp_listen_sock = INVALID_SOCKET;
			return;
		}

		rsp_c_freeaddrinfo(result_addr);

		if(rsp_c_listen(rsp_listen_sock, 1) < 0)
		{
			rsp_fatal("Failed to listen on socket: %s\n", rsp_c_errstr());
			rsp_c_close(rsp_listen_sock);
			rsp_listen_sock = INVALID_SOCKET;
			return;
		}

		return;
	}
	else if(rsp_client_sock == INVALID_SOCKET && rsp_listen_sock != INVALID_SOCKET)
	{
		//No client, but we've made the listener. See if we can accept a connection.
		struct sockaddr_storage their_addr;
		memset(&their_addr, 0, sizeof(their_addr));

		socklen_t their_addrlen = sizeof(their_addr);

		rsp_client_sock = rsp_c_accept(rsp_listen_sock, (struct sockaddr*)(&their_addr), &their_addrlen);
		if(rsp_client_sock == INVALID_SOCKET)
			return; //Nobody connected to us yet

		if(rsp_c_nonblock(rsp_client_sock) < 0)
		{
			rsp_fatal("Failed to make client socket nonblocking: %s\n", rsp_c_errstr());
			rsp_c_close(rsp_client_sock);
			rsp_client_sock = INVALID_SOCKET;
			return;
		}

		if(getenv("NEMUL_LOG_RSP"))
			fprintf(stderr, "rsp: client connected\n");

		//Breakpoints belong to the client that asked for them. A new client arrives knowing of
		//none, so any left from the last one are cleared: a machine that stops at an address the
		//new debugger never asked about reports a stray SIGTRAP and looks broken.
		rsp_bp_count = 0;
		process_bp_clear();

		//A client that attaches is told the machine is stopped, so it has to be stopped: every
		//process stands still until the client says otherwise, which is what a debugger attaching
		//to a running program expects
		rsp_stop_everyone(PROCESS_DBGSTOP_CTRLC, 0);

		//A new client starts from a clean slate: nothing is left over from the last one
		rsp_pid_g = -1;
		rsp_pid_c = -1;
		rsp_step_pid = 0;
		rsp_step_count = 0;
		rsp_stop_pid = 0;
		rsp_stop_signal = 0;
		rsp_incoming_len = 0;
		rsp_threadinfo_step = 0;
		rsp_errtext_supported = 0;
		return;
	}
	else if(rsp_client_sock != INVALID_SOCKET && rsp_listen_sock != INVALID_SOCKET)
	{
		//Have both client and listener sockets. One client at a time, as gdbserver has it: the
		//port is closed while somebody is debugging, and opened again when they leave. A client
		//that arrives in the moment between one leaving and the port reopening is refused, which
		//is what a client that retries - gdb does - expects.
		rsp_c_close(rsp_listen_sock);
		rsp_listen_sock = INVALID_SOCKET;
		return;
	}

	//Okay, we have a client connected and further listening closed.
	rsp_giveup_count = 0;

	//Drain outgoing data to the client before reading more commands
	while(1)
	{
		if(rsp_outgoing_len == 0)
			break;

		if(rsp_outgoing_sent >= rsp_outgoing_len)
		{
			rsp_outgoing_sent = 0;
			rsp_outgoing_len = 0;
			break;
		}

		int nsent = rsp_c_write(rsp_client_sock, rsp_outgoing_buf + rsp_outgoing_sent, rsp_outgoing_len - rsp_outgoing_sent);
		if(nsent < 0 && rsp_c_wouldblock(errno))
			return; //Pipe is stuffed up, need to wait

		if(nsent < 0)
		{
			rsp_client_lost();
			return;
		}

		if(nsent == 0)
		{
			rsp_client_lost();
			return;
		}

		rsp_outgoing_sent += nsent;
	}

	//Read incoming data and process commands
	while(1)
	{
		char readbuf[256] = {0};
		int nread = rsp_c_read(rsp_client_sock, readbuf, sizeof(readbuf));
		if(nread < 0 && rsp_c_wouldblock(errno))
			break; //No more data at this time

		if(nread < 0)
		{
			rsp_client_lost();
			return;
		}

		if(nread == 0)
		{
			rsp_client_lost();
			return;
		}

		for(int bb = 0; bb < nread; bb++)
			rsp_gotbyte(readbuf[bb]);
	}

	//A single-step is carried out between slices of emulated time, so that the process can be
	//stopped again before it runs past the instruction it was stepped over
	rsp_serve_step();
}

void rsp_dbgstop(int pid, process_dbgstop_t reason)
{
	process_t *pptr = process_find(pid);
	if(pptr == NULL)
	{
		TERROR("%s", "Tried to set debug-stop for an invalid process");
		return;
	}

	pptr->dbgstop = reason;

	//With no client there is nobody to tell, and a packet left in the outgoing buffer would
	//never be read
	if(rsp_client_sock == INVALID_SOCKET)
		return;

	static const int signal_mapping[PROCESS_DBGSTOP_MAX] =
	{
		0,  //None
		2,  //Interrupted - SIGINT
		5,  //Stopped for a signal - SIGTRAP
		5,  //Breakpoint instruction - SIGTRAP
		11, //Data abort - SIGSEGV
		10, //Alignment check - SIGBUS
		11, //Prefetch abort - SIGSEGV
		32, //Interpreter failure - SIGPWR
	};

	int report_signal = 5;
	if(reason > 0 && reason < PROCESS_DBGSTOP_MAX && signal_mapping[reason] != 0)
		report_signal = signal_mapping[reason];

	//A client expects the whole machine to be stopped when any part of it stops
	rsp_stop_everyone(PROCESS_DBGSTOP_CTRLC, pid);

	rsp_report_stop(pptr->pid, report_signal);
}

void rsp_notify_exit(int pid)
{
	//Nothing to tell a client that is not there
	if(rsp_client_sock == INVALID_SOCKET)
		return;

	rsp_putpkt_start();
	rsp_putpkt_str("W00;process:");
	rsp_putpkt_tid(pid);
	rsp_putpkt_str(";");
	rsp_putpkt_end();

	//If this was the thread the client was following, it has to pick another one
	if(rsp_pid_g == pid)
		rsp_pid_g = -1;
	if(rsp_pid_c == pid)
		rsp_pid_c = -1;
	if(rsp_stop_pid == pid)
		rsp_stop_pid = 0;
}

void rsp_set_continue_hook(void (*hook)(void))
{
	rsp_continue_hook = hook;
}

//Front ends tell the server how to load a program, so that a client can restart the machine with
//a new build without the emulator being restarted around it
void rsp_set_image_hooks(int (*load)(const char *path, char *err, size_t errsz), const char *(*current)(void))
{
	rsp_hook_load = load;
	rsp_hook_image = current;
}

bool rsp_present(void)
{
	return (rsp_client_sock != INVALID_SOCKET);
}
