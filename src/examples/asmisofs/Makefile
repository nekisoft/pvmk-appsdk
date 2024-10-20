#Makefile for assembly+ISO9660 for Neki32
#Bryan E. Topp <betopp@betopp.com> 2024

#Options passed to GCC - freestanding mode, no system libraries/headers
GCCOPTS += -ffreestanding -nostdlib -nostdinc

#Options passed to GCC - place compiled code at address 0
GCCOPTS += -Wl,--Ttext=0

#Options passed to GCC - look for includes in our source folder
GCCOPTS += -Isrc

#Make output ISO using mkisofs
img/asmisofs.iso: bin/boot.nne isoroot/BOOT.NNE isoroot/IMAGE.BIN
	mkdir -p $(@D)
	mkisofs -posix-L -eltorito-platform 0x92 -no-emul-boot -eltorito-boot BOOT.NNE -o $@ isoroot
	
#Including binary as built into the ISO, as a symlink
isoroot/BOOT.NNE : bin/boot.nne
	echo ""

#Make output ISO image from the compiled ELF, stripping the ELF format
bin/boot.nne: bin/boot.elf
	mkdir -p $(@D)
	arm-none-eabi-objcopy $< -O binary $@
	
#Compile assembly source into ELF
bin/boot.elf: src/asmisofs.s
	mkdir -p $(@D)
	arm-none-eabi-gcc $(GCCOPTS) $< -o $@
	
#Pseudotarget to clean project
clean :
	rm -rf obj
	rm -rf bin
	rm -rf img

