#Makefile for freestanding-C example for Neki32
#Bryan E. Topp <betopp@betopp.com> 2024

#Toolchain location
PVMKSDK=../../..
MKISOFS=$(PVMKSDK)/bin/pvmk-xorriso -as mkisofs
MKDIR  =$(PVMKSDK)/bin/pvmk-mkdir

#Options passed to GCC - freestanding mode, no system libraries/headers
GCCOPTS += -ffreestanding -nostdlib -nostdinc

#Options passed to GCC - place compiled code at address 0
GCCOPTS += -Wl,--Ttext=0

#Options passed to GCC - look for includes in our source folder
GCCOPTS += -Isrc

#Options passed to GCC - debug info
GCCOPTS += -g

#Make output ISO using mkisofs
img/game.iso: bin/boot.nne isoroot/BOOT.NNE
	$(MKDIR) -p $(@D)
	$(MKISOFS) -follow-links -eltorito-platform 0x92 -no-emul-boot -eltorito-boot BOOT.NNE -o $@ isoroot
	
#Including binary as built into the ISO
isoroot/BOOT.NNE : bin/boot.nne
	cp $< $@

#Make output ISO image from the compiled ELF, stripping the ELF format
bin/boot.nne: bin/boot.elf
	$(MKDIR) -p $(@D)
	arm-none-eabi-objcopy $< -O binary $@
	
#Compile assembly and C source into ELF
#Note that the assembly containing the header must come first!
bin/boot.elf: src/setup.s src/freestanding.c
	$(MKDIR) -p $(@D)
	arm-none-eabi-gcc $(GCCOPTS) $^ -o $@
	
#Pseudotarget to clean project
clean :
	rm -rf obj
	rm -rf bin
	rm -rf img

