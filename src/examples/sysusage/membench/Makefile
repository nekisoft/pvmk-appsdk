#Makefile for the Neki32 game template
#Bryan E. Topp <betopp@betopp.com> 2026

#Find the toolchain by locating "bin/pvmk-sdkversion" in PATH or above this directory,
#or use PVMKSDK (recommended for scripts and CI).
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
CAT    =$(SDKROOT)/bin/pvmk-cat

GCCOPTS += -std=c99 -Wall -Werror -Wextra -pedantic -g -O2

#Which shared benchmark workload this build runs, to compare nemul's timing model
#against the same code measured on the board. 0 does no work and is the baseline.
ifneq ($(WORKLOAD),)
GCCOPTS += -DWORKLOAD=$(WORKLOAD)
endif

CSRC:=$(shell $(FIND) src -name "*.c")
COBJ:=$(patsubst src/%.c, obj/%.o, $(CSRC))

#The volume name is the game's identity in the save system: without a usable one, the game
#gets no NVM record. Keep it short, uppercase and different from every other game.
VOLID := $(shell $(CAT) isoroot/name.txt)

img/game.iso: bin/boot.nne isoroot/BOOT.NNE isoroot/name.txt
	$(MKDIR) -p $(@D)
	$(MKISOFS) -V "$(VOLID)" -follow-links -eltorito-platform 0x92 -no-emul-boot \
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

#The harness: build, run 300 frames, assert the events, compare frame hashes.
test: img/game.iso
	$(SDKROOT)/bin/pvmk-run -C . --no-build --frames 300 \
		--input tests/input.txt --expect '\[EVENT\] BOOT' --expect '\[EVENT\] COLLECT' \
		--baseline tests/golden.txt

clean :
	$(RM) -rf obj
	$(RM) -rf bin
	$(RM) -rf img

.PHONY: test clean
