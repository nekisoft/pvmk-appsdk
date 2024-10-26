//setup.s
//Example of assembly-mode runtime for freestanding C example
//Bryan E. Topp <betopp@betopp.com> 2024

//As with the "asmisofs" example, we compile this into a no-nonsense executable (.nne) file.
//We also link against some C code in the file "freestanding.c" however.

//Code section at the beginning of the NNE file
.text
.org 0
nne_start:
	.ascii "NNEARM32" //Magic number
	.long 0x1000     //Entry point
	.fill 4096 - 8 - 8 //Fill rest of 4KBytes

//Entry point at +4KBytes, entered on program start and on signals
.global _start
.equ _start, 0x1000
.org 0x1000
	//Check if this is the initial program start, or a signal
	cmp r0, #0
	bne handle_signal
	b process_setup
	.ltorg

//Handler from entry point if a signal is being handled
handle_signal:
	//On a signal, immediately give up and quit.
	//Note that we do not unblock any signals, so this should not happen.
	mov r2, r0    //Syscall parameter - signal causing termination
	mov r1, #0xFF //Syscall parameter - exit code
	mov r0, #0x07 //Syscall number - _sc_exit()
	udf #0x92     //Run syscall
	.ltorg

//Handler from entry point if the process is starting fresh
process_setup:	
	//Set up stack
	ldr sp, =stack_top 
	
	//Call main procedure from compiled C code
	.extern freestanding_main
	bl freestanding_main
	
	//Exit if the main procedure returns
	mov r2, #0    //Syscall parameter - signal causing termination, none
	mov r1, r0    //Syscall parameter - exit code, reusing return value from main
	mov r0, #0x07 //Syscall number - _sc_exit()
	udf #0x92     //Run syscall
	
	.ltorg
	
//Shim so the C code can run a system-call
.global freestanding_sc
freestanding_sc:
	//Assume parameters are already passed in registers
	//Run the system call
	udf #0x92
	
	//Return to caller
	bx lr
	
	.ltorg

	
//Stack space for our C code
.balign 2048
stack_bottom:
	.space 4096
stack_top:

