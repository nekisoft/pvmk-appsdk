#Makefile for the neki3d2 fixed-point 3D renderer
#Bryan E. Topp <betopp@betopp.com> 2026

#Find toolchain by locating "bin/pvmk-sdkversion" in the PATH or parent directories,
#or use PVMKSDK explicitly.
WHICH:=$(if $(findstring Windows, $(OS)), where, which)
SDKVER:=$(if $(strip $(SDKVER)), $(SDKVER), $(firstword $(shell $(WHICH) pvmk-sdkversion)))
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

#-O2, and -g so the profiler can attribute to source lines and a debugger can show variables
GCCOPTS += -std=c99 -Wall -Werror -Wextra -pedantic -g -O2
LIBS += -lm

CSRC:=$(shell $(FIND) src -name "*.c")
COBJ:=$(patsubst src/%.c, obj/%.o, $(CSRC))

#The card image has no save record: the volume name starts with '-' on purpose
img/game.iso: bin/boot.nne isoroot/BOOT.NNE
	$(MKDIR) -p $(@D)
	$(MKISOFS) -follow-links -eltorito-platform 0x92 -no-emul-boot \
		-eltorito-boot BOOT.NNE -o $@ isoroot

isoroot/BOOT.NNE : bin/boot.nne
	$(CP) $< $@

bin/boot.nne: bin/boot.elf
	$(MKDIR) -p $(@D)
	$(OBJCOPY) $< -O binary $@

bin/boot.elf: $(COBJ)
	$(MKDIR) -p $(@D)
	$(CC) $^ $(LIBS) -o $@

obj/%.o : src/%.c
	$(MKDIR) -p $(@D)
	$(CC) $(GCCOPTS) $< -c -o $@

clean :
	$(RM) -rf obj
	$(RM) -rf bin
	$(RM) -rf img

.PHONY: clean
