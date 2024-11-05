//asmdata.s
//Example of loading extra data from an all-assembly program on Neki32
//Bryan E. Topp <betopp@betopp.com> 2024

//See the allasm example for an explanation of the basics here.
//We will additionally place some binary data on the disc, beyond what gets loaded by the system on boot.
//Then, we can load and display it with _sc_disk_read2k.

//Code section at the beginning of the disc
.org 0
disk_start:
	.ascii "NNEARM32" //Magic number
	.long 0x1000     //Entry point

//Entry point
.global _start
.equ _start, 0x1000
.org 0x1000
	cmp r0, #0
	bne handle_signal
	b process_setup
	.ltorg

handle_signal:
	//Note that we do not unblock any signals, so this should not happen.
	mov r2, r0    //Syscall parameter - signal causing termination
	mov r1, #0xFF //Syscall parameter - exit code
	mov r0, #0x07 //Syscall number - _sc_exit()
	udf #0x92     //Run syscall
	.ltorg
	
process_setup:
	mov r0, #0x40                    //Syscall number - _sc_mem_sbrk()
	.equ extra_memory, vars_end - load_end
	ldr r1, =extra_memory            //Syscall parameter - memory to add
	udf #0x92                        //Run syscall
	
	b main_example
	.ltorg

main_example:

	//Load an image from the disc into the framebuffer
	main_example_loadloop:
	
		mov r0, #0x91 //Syscall number - _sc_disk_read2k()
		ldr r1, =image_sector //Sector number of data to read
		ldr r2, =framebuffer //Location to put result
		mov r3, #((320*240*2)/2048) //Number of sectors to load
		
		udf #0x92 //Trigger system call
		
		cmp r0, #0 //Check if we got some other error
		bne main_example_loadloop //Retry in that case
	
	//Show it
	mov r0, #0x30          //Syscall number - _sc_gfx_flip()
	mov r1, #2             //Syscall parameter - graphics mode: 320x240 16bpp
	ldr r2, =framebuffer   //Syscall parameter - framebuffer to display
	udf #0x92              //Run syscall
	
	//Loop
	b main_example
	.ltorg


//El Torito Boot Catalog, giving location of our executable (i.e. the beginning of the image)
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
.org 0x8000
	.byte 0x01     //Primary Volume Descriptor
	.ascii "CD001" //Magic number
	.byte 0x01     //Version of PVD
	.byte 0x00     //Unused
	.ascii "NEKI32 BY NEKISOFT              " //System ID (32 bytes)
	.ascii "-Assembly+data Neki32 Example   " //Volume ID (32 bytes), start with '-' so we do not get savegame space
	
//El Torito Boot Volume Descriptor, at sector 17 (+34KBytes)
.org 0x8800
	.byte 0x00     //Boot Record Identifier
	.ascii "CD001" //Magic number
	.byte 0x01     //Version of BVD
	.ascii "EL TORITO SPECIFICATION" //Magic number
	.byte 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 //Padding of magic number
	.fill 32, 1, 0x00 //Unused
	.long ((boot_catalog - disk_start) / 2048) //Pointer to El Torito Boot Catalog, as sector number
	
//Symbol representing the end of data loaded at boot
.balign 2048
load_end:

//Variables that are zeroed initially follow
.balign 2048
vars_start:

//Framebuffer storage
.balign 4096
framebuffer:
	.space 320*240*2

.balign 2048
vars_end:

//Anything after this label does not get loaded on boot, nor does it have memory allocated for it.
//However, we can still refer to it as a sector on disc, and load it into other memory.

//Put an image on disc, aligned to a sector boundary
.balign 2048
.equ image_sector, (image_data - disk_start) / 2048
image_data:
	//A binary file containing a framebuffer-ready image prepared earlier.
	//This is just a 320x240 array of 16-bit values, with no headers or anything.
	.incbin "image.data"

