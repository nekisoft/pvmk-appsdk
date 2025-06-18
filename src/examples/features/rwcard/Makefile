#Makefile for card-writing example for Neki32
#Bryan E. Topp <betopp@betopp.com> 2024

#Find toolchain by locating "bin/pvmk-sdkversion" in the PATH or parent directories
WHICH:=$(if $(findstring Windows, $(OS)), where, which)
SDKVER:=$(if $(strip $(SDKVER)), $(SDKVER), $(firstword $(shell $(WHICH) pvmk-sdkversion)))
SDKVER:=$(if $(strip $(SDKVER)), $(SDKVER), $(realpath ../bin/pvmk-sdkversion))
SDKVER:=$(if $(strip $(SDKVER)), $(SDKVER), $(realpath ../../bin/pvmk-sdkversion))
SDKVER:=$(if $(strip $(SDKVER)), $(SDKVER), $(realpath ../../../bin/pvmk-sdkversion))
SDKVER:=$(if $(strip $(SDKVER)), $(SDKVER), $(realpath ../../../../bin/pvmk-sdkversion))
PVMKSDK?=$(realpath $(dir $(SDKVER))/..)
SDKROOT?=$(PVMKSDK)

CC     =$(SDKROOT)/bin/pvmk-cc
OBJCOPY=$(SDKROOT)/bin/pvmk-objcopy
MKISOFS=$(SDKROOT)/bin/pvmk-xorriso -as mkisofs
FIND   =$(SDKROOT)/bin/pvmk-find
MKDIR  =$(SDKROOT)/bin/pvmk-mkdir
CP     =$(SDKROOT)/bin/pvmk-cp
RM     =$(SDKROOT)/bin/pvmk-rm

#Options to pass to compiler
GCCOPTS += -std=c99 -Wall -Werror -Wextra -pedantic -g -O1

#Source files
CSRC:=$(shell $(FIND) src -name *.c)

#Object files
COBJ:=$(patsubst src/%.c, obj/%.o, $(CSRC))

#Make output ISO using mkisofs
img/game.iso: bin/boot.nne isoroot/BOOT.NNE
	$(MKDIR) -p $(@D)
	$(MKISOFS) -follow-links -eltorito-platform 0x92 -no-emul-boot -eltorito-boot BOOT.NNE -o $@ isoroot
	
#Including binary as built into the ISO
isoroot/BOOT.NNE : bin/boot.nne
	$(CP) $< $@

#Make output ISO image from the compiled ELF, stripping the ELF format
bin/boot.nne: bin/boot.elf
	$(MKDIR) -p $(@D)
	$(OBJCOPY) $< -O binary $@
	
#Link C source with SDK into ELF
bin/boot.elf: $(COBJ)
	$(MKDIR) -p $(@D)
	$(CC) $(GCCOPTS) $< -o $@

#Compile C source with SDK
obj/%.o : src/%.c
	$(MKDIR) -p $(@D)
	$(CC) $(GCCOPTS) $< -c -o $@
	
#Pseudotarget to clean project
clean :
	$(RM) -rf obj
	$(RM) -rf bin
	$(RM) -rf img

