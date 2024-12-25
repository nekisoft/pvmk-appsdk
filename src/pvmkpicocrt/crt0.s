//crt0.s
//Entry point for 32-bit ARM userland
//Bryan E. Topp <betopp@betopp.com> 2021

//Placed at beginning of file
.section .header

//Takes up the first 1 page, as the kernel doesn't allow user access to this (catches NULL access)
.global _header
_header:
	.ascii "NNEARM32" //Signature
	.word  _start //Entry point
	.balign 4096 //Consume the rest of the first page
	_header.end:

.section .text.first

//Entry point - on initial entry and when signalled.
.global _start
_start:
	//The kernel only cares about a single entry point - used for both signals and on program startup.
	//Check if this is a signal or not.
	cmp r0, #0
	beq _start.initial
	
	//It's a signal. Use signal-handling stack.
	ldr sp, =_sigstack.top
	
	//Call libc signal-handling to run user handlers, on the signal-handling stack.
	.extern raise //int raise (int sig)
	ldr r7, =raise
	blx r7
	
	//Return from signal
	ldr r0, =#0x22 //_sc_sig_return
	udf 0x92       //system call
	
_start.initial:

	//The program is newly started.
	
	//Use initial stack
	ldr sp, =_stack.top
	
	//Call libc to get argv/envp set up and call main
	.extern _pvmk_callmain
	ldr r7, =_pvmk_callmain
	bx r7

	.ltorg	

//Long-jumping
.global _setjmp //extern int _setjmp(jmp_buf env);
_setjmp:
	//These always happen as part of a function call.
	//So, only registers preserved across function calls must be saved/loaded.
	stmia r0, {r4-r14}
	
	//First return - _setjmp returns 0.
	mov r0, #0
	bx lr
	
.global _longjmp //extern void _longjmp(jmp_buf env, int val);
_longjmp:
	//Restore saved registers - note, doesn't clobber "val"
	ldmia r0, {r4-r14}
	
	//Return as if _setjmp is now returning "val".
	//But - ensure that's not 0, because that means _setjmp is returning the first time.
	mov r0, r1
	cmp r0, #0
	bne _longjmp.noinc
	add r0, #1
	_longjmp.noinc:
	bx lr

.global sigsetjmp //int sigsetjmp(sigjmp_buf env, int savemask)
sigsetjmp:
	
	mov r3, #0 //Initialize value of signal mask to save
	str r1, [r0] //store savemask value to *env
	cmp r1, #0 //check if we want to save signals
	beq sigsetjmp.nosigs
		push {r0, lr} //preserve env
		
		mov r0, #0x20 //_sc_sig_mask
		mov r1, #0 //SIG_BLOCK - but we're passing 0, so nothing is blocked
		mov r2, #0 //Block nothing
		udf 0x92 //System call
		
		mov r3, r0 //Set aside return value
		
		pop {r0, lr} //restore buf
	sigsetjmp.nosigs:
	str r3, [r0, #8] //store signals mask
	
	//Continue to preserving registers
	add r0, #16
	b _setjmp


.global siglongjmp //void siglongjmp(sigjmp_buf env, int val)
siglongjmp:

	ldr r2, [r0] //get old value of savemask - figure out if we'll restore signals
	cmp r2, #0
	beq siglongjmp.nosigs
		push {r0, lr} //preserve env
		
		//Get previously-saved signals mask
		ldr r2, [r0, #8]
		
		mov r0, #0x20 //_sc_sig_mask
		mov r1, #2 //SIG_SETMASK
		mov r2, r2 //Saved mask
		udf 0x92 //System call
		
		pop {r0, lr} //restore env
	siglongjmp.nosigs:
	
	//Do the rest of the jump
	add r0, #16
	b _longjmp

.ltorg

.global setjmp //int setjmp(jmp_buf env)
setjmp:
	mov r1, #1 //setjmp is just sigsetjmp with savemask == true.
	b sigsetjmp

.global longjmp //void longjmp(jmp_buf env, int val)
longjmp:
	b siglongjmp


//For C++...? Why doesn't libcxxabi define this?
.global __cxa_get_globals
.global __cxa_get_globals_fast
__cxa_get_globals:
__cxa_get_globals_fast:
	ldr r0, =__cxa_globals_storage
	bx lr

//I guess these don't exist in libgcc? clang seems to emit them while compiling libcxx.
//type __sync_val_compare_and_swap (type *ptr, type oldval, type newval, ...)
.global __sync_val_compare_and_swap_1
__sync_val_compare_and_swap_1:
	ldrb r3, [r0] //Load current value from pointer
	cmp r3, r1 //Compare with expected old value
	bne __sync_val_compare_and_swap_1.done //If they're not equal, do nothing
	strb r2, [r0] //Store new value to pointer
	__sync_val_compare_and_swap_1.done:
	mov r0, r3 //Return old contents
	bx lr

//We don't support multithreading but python3 needs this
//TLS access will just refer to where the .tdata and .tbss thread-local sections are linked in the .nne file
.global __aeabi_read_tp
__aeabi_read_tp:
	.extern _PVMK_LINKED_TP
	ldr r0, =_PVMK_LINKED_TP //Defined in linker script
	bx lr


.ltorg

.section .bss
	
//Space for stack... need anything more complex?
.balign 65536
.global _stack
_stack:
	.space 65536
_stack.top:

//Space for signal-handling stack
.balign 65536
.global _sigstack
_sigstack:
	.space 65536
_sigstack.top:

//Space for exception handling from C++ (why doesn't libcxxabi define this?)
.balign 64
__cxa_globals_storage:
	.space 64

