#Makefile for all-assembly example for Neki32
#Bryan E. Topp <betopp@betopp.com> 2024

#Options passed to GCC - freestanding mode, no system libraries/headers
GCCOPTS += -ffreestanding -nostdlib -nostdinc

#Options passed to GCC - place compiled code at address 0
GCCOPTS += -Wl,--Ttext=0

#Options passed to GCC - look for includes in our source folder
GCCOPTS += -Isrc

#Make output ISO image from the compiled ELF, stripping the ELF format
bin/asmdata.iso: obj/asmdata.elf
	mkdir -p $(@D)
	arm-none-eabi-objcopy $< -O binary $@
	
#Compile assembly source into ELF
obj/asmdata.elf: src/asmdata.s src/image.data
	mkdir -p $(@D)
	arm-none-eabi-gcc $(GCCOPTS) $< -o $@
	
#Pseudotarget to clean project
clean :
	rm -rf bin
	rm -rf obj
