#Makefile for card-based application
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

CC     =$(PVMKSDK)/bin/pvmk-cc
OBJCOPY=$(PVMKSDK)/bin/pvmk-objcopy
MKISOFS=$(PVMKSDK)/bin/pvmk-xorriso -as mkisofs
DATE   =$(PVMKSDK)/bin/pvmk-date
FIND   =$(PVMKSDK)/bin/pvmk-find
MKDIR  =$(PVMKSDK)/bin/pvmk-mkdir
CP     =$(PVMKSDK)/bin/pvmk-cp
RM     =$(PVMKSDK)/bin/pvmk-rm

#Directories used locally
SRCDIR=./src
OBJDIR=./obj
BINDIR=./bin
IMGDIR=./img
CARDROOT=./cardroot

#C source files and resulting objects
CSRC:=$(shell $(FIND) -L $(SRCDIR) -name "*.c")
COBJ:=$(patsubst $(SRCDIR)/%.c, $(OBJDIR)/%.o, $(CSRC))

#Flags to use when compiling
CFLAGS += -std=c99 -pedantic
CFLAGS += -Wall -Werror -Wextra
CFLAGS += -O2

#Flags to use when linking
LINKFLAGS += 
LIBS += -lsdl3

#Build information provided to compiler
BUILDDATE := $(shell $(DATE) -u +%Y-%m-%dT%H:%M:%SZ)
BUILDUSER := $(shell whoami)@$(shell hostname)
CFLAGS += -DBUILDDATE=\"$(BUILDDATE)\"
CFLAGS += -DBUILDUSER=\"$(BUILDUSER)\"

#Name of ISO image to build
GAMEIMG=$(IMGDIR)/game.iso

#Build filesystem with the game contents
$(GAMEIMG) : $(CARDROOT)/boot.nne $(CARDROOT)/name.txt $(shell $(FIND) -H $(CARDROOT))
	$(MKDIR) -p $(@D)
	$(MKISOFS) -V "`cat $(CARDROOT)/name.txt`" -eltorito-platform 0x92 -no-emul-boot -eltorito-boot boot.nne -eltorito-platform 0x22 -no-emul-boot -eltorito-boot neki_upd.tar -R -follow-links -o $@ $(CARDROOT)
	
#Copy built executable to card directory
$(CARDROOT)/boot.nne : $(BINDIR)/boot.nne
	$(CP) $< $@

#Link post-process target - no-nonsense executable made by stripping the ELF
$(BINDIR)/boot.nne : $(BINDIR)/boot.elf
	$(MKDIR) -p $(@D)
	$(OBJCOPY) -O binary $< $@

#Link stage target - ELF made from linking all objects
$(BINDIR)/boot.elf : $(COBJ)
	$(MKDIR) -p $(@D)
	$(CC) $(LINKFLAGS) $^ $(LIBS) -o $@

#Compile targets - object for each C file
$(OBJDIR)/%.o : $(SRCDIR)/%.c
	$(MKDIR) -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@
	
#Cleanup target
clean:
	$(RM) -rf $(BINDIR) $(OBJDIR) $(IMGDIR)
