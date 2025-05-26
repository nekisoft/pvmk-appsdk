#Makefile for all-assembly example for Neki32
#Bryan E. Topp <betopp@betopp.com> 2024

#Options passed to GCC - freestanding mode, no system libraries/headers
GCCOPTS += -ffreestanding -nostdlib -nostdinc

#Options passed to GCC - place compiled code at address 0
GCCOPTS += -Wl,--Ttext=0

#Make output ISO image from the compiled ELF, stripping the ELF format
bin/allasm.iso: obj/allasm.elf
	mkdir -p $(@D)
	arm-none-eabi-objcopy $< -O binary $@
	
#Compile assembly source into ELF
obj/allasm.elf: src/allasm.s
	mkdir -p $(@D)
	arm-none-eabi-gcc $(GCCOPTS) $^ -o $@
	
#Pseudotarget to clean project
clean :
	rm -rf bin
	rm -rf obj
