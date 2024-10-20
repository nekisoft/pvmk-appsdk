//asmisofs.s
//Example of accessing an ISO9660 filesystem from an assembly program
//Bryan E. Topp <betopp@betopp.com> 2024

//See the allasm and asmdata examples for an explanation of the basics.
//In this example, we do not need to build the ISO9660 and ElTorito structures here.
//Instead, the output of this assembly code is a No-Nonsense Executable (.nne) file.
//Then, mkisofs puts the .nne file into an ISO9660 filesystem and an ElTorito record.

//Code section at the beginning of the NNE file
.org 0
nne_start:
	.ascii "NNEARM32" //Magic number
	.long 0x1000     //Entry point

//Entry point at +4KBytes, entered on program start and on signals
.global _start
.equ _start, 0x1000
.org 0x1000
	//Check if this is the initial program start, or a signal
	cmp r0, #0
	bne handle_signal
	b process_setup
	.ltorg

handle_signal:
	//On a signal, immediately give up and quit.
	//Note that we do not unblock any signals, so this should not happen.
	mov r2, r0    //Syscall parameter - signal causing termination
	mov r1, #0xFF //Syscall parameter - exit code
	mov r0, #0x07 //Syscall number - _sc_exit()
	udf #0x92     //Run syscall
	.ltorg
	
process_setup:	
	ldr sp, =stack_top //Set up stack
	b main_example
	.ltorg

main_example:

	//Load directory entry for the root directory from the ISO9660 Primary Volume Descriptor
	ldr r0, =#(32768 + 156) //Byte offset of directory entry for the root directory in the PVD
	ldr r1, =dirent //Buffer to hold the root directory entry
	mov r2, #34 //Size of root directory entry
	bl load_bytes
	
	//Find the LBA and length of the root directory contents
	ldr r5, =(dirent + 2) //Location of LBA field in root directory entry
	ldr r5, [r5] //Load LBA
	mov r5, r5, lsl #11 //turn LBA into byte offset
	
	#error this explodes because of a misaligned access - fix the debugger stub to report it nicely!
	
	ldr r6, =(dirent + 10) //Location of length field in root directory entry
	ldr r6, [r6] //Load length
	
	//r5 and r6 now contain the byte offset and length of the root directory
	//Work through the root directory and look for the file we want
	search_loop:
		//Fill the buffer with the next directory entry
		
		//Load that range of bytes into our buffer
		mov r0, r5      //Parameter - byte offset on disk
		ldr r1, =dirent //Parameter - destination in memory
		mov r2, #256    //Parameter - number of bytes to read
		
		//Call to load the bytes
		stmfd sp!, {r5, r6, lr} //Preserve registers
		bl load_bytes
		ldmfd sp!, {r5, r6, lr}
		
		//The directory entry, as loaded, will have its name at offset 33
		//See if it matches the filename we wanted
		ldr r7, =(dirent+33)  //Filename of what we see on disk
		ldr r8, str_imagefile //Filename we are looking for
		search_loop_strcmp_loop:
			ldrb r9,  [r7]
			add r7, #1
			ldrb r10, [r8]
			add r8, #1
			
			cmp r10, #0 //Check if we got to the end of the desired string
			bne search_loop_strcmp_continue
			
				//Got all the way to the end of the string comparison.
				//This is the file we were looking for.
				//Load its contents into our framebuffer
				
				ldr r0, =(dirent + 2) //Location of LBA field in directory entry
				ldr r0, [r0] //Load LBA
				mov r0, r0, lsl #11 //turn LBA into byte offset
				
				ldr r2, =(dirent + 10) //Location of length field in directory entry
				ldr r2, [r2] //Load length
				
				ldr r1, =framebuffer
				
				b main_example_loaddone
			
			search_loop_strcmp_continue:
			cmp r9, r10 //See if the strings continue to match
			beq search_loop_strcmp_loop //Strings match so far, keep looking
		
		//This file is not the one we are looking for.
		//Move past the directory entry and keep looking.
		
		//Size of directory entry is the first byte of the entry
		ldr r0, =dirent
		ldrb r0, [r0]
		
		add r5, r0
		sub r6, r0
		
		//See if we are done with the directory
		cmp r6, #0
		bgt search_loop
		
		//If we failed to find it... maybe handle the error somehow. For now just try again.
		b main_example
	
	
	main_example_loaddone:
	
	//Show what we loaded
	mov r0, #0x30          //Syscall number - _sc_gfx_flip()
	mov r1, #2             //Syscall parameter - graphics mode: 320x240 16bpp
	ldr r2, =framebuffer   //Syscall parameter - framebuffer to display
	udf #0x92              //Run syscall
	
	//Loop
	b main_example
	.ltorg
	
//Subroutine for reading a sector from disk, retrying as necessary
//Parameters - r0: sector number, r1: address of buffer to store data
load_sector:

	//Set aside parameters in case they get clobbered
	stmfd sp!, {r0, r1}
	
	//Run system call
	mov r2, r1 //Syscall parameter - buffer location
	mov r1, r0 //Syscall parameter - sector number
	mov r0, #0x91 //Syscall number - _sc_disk_read2k()
	udf #0x92
	
	//Check return value, see if we read the sector
	cmp r0, #2048
	
	//Pop the parameters off the stack, whether we read it successfully or not
	ldmfd sp!, {r0, r1}
	
	//If we were successful, return
	bxeq lr
	
	//Otherwise, retry
	b load_sector

//Subroutine for reading bytes from a disk, across sectors
//Parameters - r0: byte offset on disk, r1: destination buffer in memory, r2: bytes length to read
load_bytes:
	
	//Figure out how many bytes we can read from a sector - at most, all those remaining, or to the end of the sector
	ldr r4, =#(2048-1)
	and r5, r0, r4 //Misalignment
	mov r6, #2048
	sub r5, r6, r5 //To end of sector
	cmp r5, r2
	movhi r5, r2 //Cap to remaining length
	
	//Figure out the sector number we need to load from - the byte offset divided by the sector size
	mov r4, r0, lsr #11
	
	//See if we already have the sector in question loaded
	ldr r6, =load_bytes_lba
	ldr r6, [r6]
	cmp r6, r4
	beq load_bytes_havesector
	
		//Have to load the sector
		
		//Preserve parameters
		stmfd sp!, {r0, r1, r2, r4, r5, r6, lr}
		
		//Call load_sector subroutine to load the sector we need
		mov r0, r4 //Sector number
		ldr r1, =load_bytes_buf //Buffer address
		bl load_sector
		
		//Restore parameters
		ldmfd sp!, {r0, r1, r2, r4, r5, r6, lr}
		
		//Note which sector is now in our buffer
		ldr r6, =load_bytes_lba
		str r4, [r6]
	
	load_bytes_havesector:
	
	//We have the sector we need in our buffer.
	//Copy the appropriate bytes out of it.
	ldr r9, =#(2048-1)
	and r8, r0, r9 //Misalignment in sector
	ldr r9, =load_bytes_buf //Beginning of sector buffer
	add r8, r9 //Location to read from in buffer
	
	load_bytes_copyloop:
	
		//Check if we have any bytes left to copy from this sector
		cmp r5, #0
		ble load_bytes_copydone
		
		//Copy the next byte
		ldrb r7, [r8]
		strb r7, [r1]
		
		//Continue copying
		add r8, #1 //Move forward in sector-buffer to copy from
		sub r5, #1 //One less byte to copy in this sector
		
		add r0, #1 //Move forward in overall location to read
		add r1, #1 //Move forward in destination buffer to copy to
		sub r2, #1 //One less byte to copy overall
		
		b load_bytes_copyloop
		
	load_bytes_copydone:
	
	//See if we need to keep reading from another sector
	cmp r2, #0
	bgt load_bytes
	
	//Done!
	bx lr
	
	.ltorg

//Name of the file we look for
//(Note the revision suffix ";1" that mkisofs places on the filename to conform to ISO9660.)
str_imagefile:
	.ascii "IMAGE.BIN;1\0"

//Variables that are zeroed initially follow
.balign 2048
vars_start:

//Space for ISO9660 Primary Volume Descriptor
.balign 2048
pvd:
	.space 2048
	
//Space for an ISO9660 directory entry
.balign 256
dirent:
	.space 256
	
//Which sector number is last loaded by load_bytes
.balign 4
load_bytes_lba:
	.space 4
	
//Space for loading bytes from a sector in load_bytes
.balign 2048
load_bytes_buf:
	.space 2048
	
//Stack space
.balign 2048
stack_bottom:
	.space 2048
stack_top:

//Framebuffer storage
.balign 4096
framebuffer:
	.space 320*240*2

.balign 2048
vars_end:

