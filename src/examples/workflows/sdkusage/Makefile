#Makefile for SDK usage example for Neki32
#Bryan E. Topp <betopp@betopp.com> 2024

#SDK toolchain location
SDKROOT=../../..
CC     =$(SDKROOT)/bin/pvmk-cc
OBJCOPY=$(SDKROOT)/bin/pvmk-objcopy
MKISOFS=$(SDKROOT)/bin/pvmk-xorriso -as mkisofs
FIND   =$(SDKROOT)/bin/pvmk-find
MKDIR  =$(SDKROOT)/bin/pvmk-mkdir
CP     =$(SDKROOT)/bin/pvmk-cp
RM     =$(SDKROOT)/bin/pvmk-rm

#Source files
CSRC:=$(shell $(FIND) src -name *.c)

#Object files
COBJ:=$(patsubst src/%.c, obj/%.o, $(CSRC))

#Make output ISO using mkisofs
img/game.iso: bin/boot.nne isoroot/BOOT.NNE isoroot/IMAGE.BIN
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

