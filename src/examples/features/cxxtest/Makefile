#Makefile for card-based application
#Bryan E. Topp <betopp@betopp.com> 2024

#Toolchain location
PVMKSDK=../../..
CC     =$(PVMKSDK)/bin/pvmk-cc
CXX    =$(PVMKSDK)/bin/pvmk-c++
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
COBJ:=$(patsubst $(SRCDIR)/%.c, $(OBJDIR)/%.c.o, $(CSRC))

#C++ source files and resulting objects
CXXSRC:=$(shell $(FIND) -L $(SRCDIR) -name "*.cc")
CXXOBJ:=$(patsubst $(SRCDIR)/%.cc, $(OBJDIR)/%.cc.o, $(CXXSRC))

#Flags to use when compiling
CFLAGS += -std=c99 -pedantic
CFLAGS += -Wall -Werror -Wextra
CFLAGS += -O2

CXXFLAGS += -std=c++11 -pedantic
CXXFLAGS += -Wall -Werror -Wextra
CXXFLAGS += -O0
CXXFLAGS += -g

#Flags to use when linking
LINKFLAGS += 

#Build information provided to compiler
BUILDDATE := $(shell $(DATE) -u +%Y-%m-%dT%H:%M:%SZ)
BUILDUSER := $(shell whoami)@$(shell hostname)
CFLAGS += -DBUILDDATE=\"$(BUILDDATE)\"
CFLAGS += -DBUILDUSER=\"$(BUILDUSER)\"
CXXFLAGS += -DBUILDDATE=\"$(BUILDDATE)\"
CXXFLAGS += -DBUILDUSER=\"$(BUILDUSER)\"

#Name of ISO image to build
GAMEIMG=$(IMGDIR)/game.iso

#Build filesystem with the game contents
$(GAMEIMG) : $(CARDROOT)/boot.nne $(CARDROOT)/name.txt $(shell $(FIND) -H $(CARDROOT))
	$(MKDIR) -p $(@D)
	$(MKISOFS) -V "`cat $(CARDROOT)/name.txt`" -eltorito-platform 0x92 -no-emul-boot -eltorito-boot boot.nne -eltorito-platform 0x22 -no-emul-boot -eltorito-boot neki_upd.tar -R -follow-links -o $@ $(CARDROOT)
	
#Copy executable into card files
$(CARDROOT)/boot.nne : $(BINDIR)/boot.nne
	$(CP) $< $@

#Link post-process target - no-nonsense executable made by stripping the ELF
$(BINDIR)/boot.nne : $(BINDIR)/boot.elf
	$(MKDIR) -p $(@D)
	$(OBJCOPY) -O binary $< $@

#Link stage target - ELF made from linking all objects
$(BINDIR)/boot.elf : $(COBJ) $(CXXOBJ)
	$(MKDIR) -p $(@D)
	$(CC) $(LINKFLAGS) $^ -o $@

#Compile targets - object for each C file
$(OBJDIR)/%.c.o : $(SRCDIR)/%.c
	$(MKDIR) -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@
	
#Compile targets - object for each C++ file
$(OBJDIR)/%.cc.o : $(SRCDIR)/%.cc
	$(MKDIR) -p $(@D)
	$(CXX) $(CXXFLAGS) -c $< -o $@
	
#Cleanup target
clean:
	$(RM) -rf $(BINDIR) $(OBJDIR) $(IMGDIR)
