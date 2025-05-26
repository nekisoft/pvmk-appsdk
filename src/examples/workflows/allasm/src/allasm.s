//allasm.s
//Example of writing a Neki32 game card entirely in assembly
//Bryan E. Topp <betopp@betopp.com> 2024

//This assembly file creates a whole Neki32 game card ISO image.
//It contains the game executable itself, as well as enough ISO9660 and El Torito data to boot.

//We place the executable at the beginning of the disk, at sector 0.
//This way, the offsets in the disk image are the same as those in the executable.
//The offsets in the process memory are the same as in the executable, as always on Neki32.

//Header at the beginning
//  ISO9660 and El Torito both leave a gap here.
//  When running, the first 4KBytes are inaccessible.
//  So this space is purely used for the executable header.
.org 0
disk_start:
	.ascii "NNEARM32" //Magic number
	.long 0x1000     //Entry point
	
//Code entry point
//  ISO9660 and El Torito still leave a gap here.
//  Execution begins 0x1000 bytes into the executable file.
//  Here, that is also 0x1000 bytes into the disk image.
//  We need to define a _start symbol for GCC to not vomit.
.global _start
.equ _start, 0x1000
.org 0x1000
	//Check if this is the initial entry or a signal entry
	cmp r0, #0
	bne handle_signal
	b process_setup
	.ltorg
	
//Signal handler
//  Code entry point jumps here if entered with r0 != 0
handle_signal:
	//Note that we do not unblock any signals, so this should not happen.
	//We could do something more intelligent here, like show an error during development
	mov r2, r0    //Syscall parameter - signal causing termination
	mov r1, #0xFF //Syscall parameter - exit code
	mov r0, #0x07 //Syscall number - _sc_exit()
	udf 0x92      //Run syscall
	.ltorg
	
//Process setup
//  Code entry point jumps here if entered with r0 == 0
process_setup:

	//Make room for our variables.
	//The process, once loaded, will extend to load_end.
	//We want it to also encompass memory for our variables, to vars_end.
	mov r0, #0x40                //Syscall number - _sc_mem_sbrk()
	.equ extra_memory, vars_end - load_end
	ldr r1, =extra_memory            //Syscall parameter - memory to add
	udf 0x92                     //Run syscall
	
	//Just assume that it worked. Go ahead to run main.
	b main_example
	.ltorg
	
//Main loop of example
//  The process_setup code jumps here
main_example:

	//Draw an image
	mov r0, #0x07E0        //Green color pixel
	ldr r1, =framebuffer   //Destination
	ldr r2, =#(320*240)    //Count
	main_example_pxloop:   //Fill pixels
		strh r0, [r1]
		add r1, #2
		sub r2, #1
		cmp r2, #0
		bne main_example_pxloop
	
	//Show it
	mov r0, #0x30          //Syscall number - _sc_gfx_flip()
	mov r1, #2             //Syscall parameter - graphics mode: 320x240 16bpp
	ldr r2, =framebuffer   //Syscall parameter - framebuffer to display
	udf 0x92               //Run syscall
	
	//Loop
	b main_example
	.ltorg


//El Torito Boot Catalog, giving location of our executable (i.e. the beginning of the image)
//  This is referenced by the Boot Volume Descriptor at Sector 17 / +34KBytes
//  Must be aligned to a CD sector (2048 bytes) as the reference is in units of sector LBA
.balign 2048
boot_catalog:

	//First entry
	.byte 0x01                         //Header ID
	.byte 0x92                         //Platform ID - Neki32 bootable
	.hword 0x00                        //Reserved
	.fill 24, 1, 0x00                  //ID string (empty)
	.hword 0x10000 - (0x9201 + 0xAA55) //Checksum
	.byte 0x55                         //Magic number
	.byte 0xAA                         //Magic number
	
	//Second entry
	.byte 0x88                             //Bootable entry ID
	.byte 0x00                             //No emulation
	.hword 0x00                            //Default load-segment (ignored)
	.byte 0x92                             //System type
	.byte 0x00                             //Unused
	.hword ((load_end - disk_start) / 512) //Number of 512-byte blocks loaded
	.long 0                                //Start of loaded image - 0, so our executable + memory + disk addresses are the same

	//Third entry
	.fill 32, 1, 0x00      //All zeroes
	
//ISO9660 Primary Volume Descriptor, at sector 16 (+32KBytes)
//  This is only used to determine the name of the game, from the Volume label
.org 0x8000
	.byte 0x01     //Primary Volume Descriptor
	.ascii "CD001" //Magic number
	.byte 0x01     //Version of PVD
	.byte 0x00     //Unused
	.ascii "NEKI32 BY NEKISOFT              " //System ID (32 bytes)
	.ascii "-All-assembly Neki32 Example    " //Volume ID (32 bytes), start with '-' so we do not get savegame space
	
	
//El Torito Boot Volume Descriptor, at sector 17 (+34KBytes)
//  This references the El Torito Boot Catalog
.org 0x8800
	.byte 0x00     //Boot Record Identifier
	.ascii "CD001" //Magic number
	.byte 0x01     //Version of BVD
	.ascii "EL TORITO SPECIFICATION" //Magic number
	.byte 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 //Padding of magic number
	.fill 32, 1, 0x00 //Unused
	.long ((boot_catalog - disk_start) / 2048) //Pointer to El Torito Boot Catalog, as sector number
	
//Symbol representing the end of data loaded at boot
//i.e. the end of the executable file
.balign 2048
load_end:

//Variables that are zeroed initially follow
//  These are not stored in the executable file but are given space in the process using an _sc_mem_sbrk call later.
.balign 2048
vars_start:

//Framebuffer storage
.balign 4096
framebuffer:
	.space 320*240*2

.balign 2048
vars_end:

