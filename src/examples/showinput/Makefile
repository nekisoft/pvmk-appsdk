#Makefile for input events example for Neki32
#Bryan E. Topp <betopp@betopp.com> 2024

#SDK toolchain location
SDKROOT=../../
CC=$(SDKROOT)/bin/pvmk-cc
OBJCOPY=$(SDKROOT)/bin/pvmk-objcopy
MKISOFS=$(SDKROOT)/bin/pvmk-xorriso -as mkisofs

#Options to pass to compiler
GCCOPTS += -std=c99 -Wall -Werror -Wextra -pedantic -g -O1

#Source files
CSRC:=$(shell find src -name *.c)

#Object files
COBJ:=$(patsubst src/%.c, obj/%.o, $(CSRC))

#Make output ISO using mkisofs
img/game.iso: bin/boot.nne isoroot/BOOT.NNE
	mkdir -p $(@D)
	$(MKISOFS) -follow-links -eltorito-platform 0x92 -no-emul-boot -eltorito-boot BOOT.NNE -o $@ isoroot
	
#Including binary as built into the ISO, as a symlink
isoroot/BOOT.NNE : bin/boot.nne
	echo ""

#Make output ISO image from the compiled ELF, stripping the ELF format
bin/boot.nne: bin/boot.elf
	mkdir -p $(@D)
	$(OBJCOPY) $< -O binary $@
	
#Link C source with SDK into ELF
bin/boot.elf: $(COBJ)
	mkdir -p $(@D)
	$(CC) $(GCCOPTS) $< -o $@

#Compile C source with SDK
obj/%.o : src/%.c
	mkdir -p $(@D)
	$(CC) $(GCCOPTS) $< -c -o $@
	
#Pseudotarget to clean project
clean :
	rm -rf obj
	rm -rf bin
	rm -rf img

