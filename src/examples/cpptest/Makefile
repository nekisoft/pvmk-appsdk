#Makefile for card-based application
#Bryan E. Topp <betopp@betopp.com> 2024

#Toolchain location
PVMKSDK=../..
CC     =$(PVMKSDK)/bin/pvmk-cc
CXX    =$(PVMKSDK)/bin/pvmk-cxx
OBJCOPY=$(PVMKSDK)/bin/pvmk-objcopy

#Directories used locally
SRCDIR=./src
OBJDIR=./obj
BINDIR=./bin
IMGDIR=./img
CARDROOT=./cardroot

#C source files and resulting objects
CSRC:=$(shell find -L $(SRCDIR) -name "*.c")
COBJ:=$(patsubst $(SRCDIR)/%.c, $(OBJDIR)/%.c.o, $(CSRC))

#C++ source files and resulting objects
CXXSRC:=$(shell find -L $(SRCDIR) -name "*.cpp")
CXXOBJ:=$(patsubst $(SRCDIR)/%.cpp, $(OBJDIR)/%.cpp.o, $(CXXSRC))

#Flags to use when compiling C
CFLAGS += -std=c99 -pedantic
CFLAGS += -Wall -Werror -Wextra
CFLAGS += -O2

#Flags to use when compiling C++
CXXFLAGS += -std=c++20 
CXXFLAGS += -Werror

#Flags to use when linking
LINKFLAGS += 

#Build information provided to compiler
BUILDVERSION := $(shell git describe --abbrev=4 --dirty --always --tags)
BUILDDATE := $(shell date -u +%FT%TZ)
BUILDUSER := $(shell whoami)@$(shell hostname)
UNIXTIME := $(shell date +%s)
CFLAGS += -DBUILDVERSION=\"$(BUILDVERSION)\"
CFLAGS += -DBUILDDATE=\"$(BUILDDATE)\"
CFLAGS += -DBUILDUSER=\"$(BUILDUSER)\"
CFLAGS += -D__TIME_UNIX__=$(UNIXTIME)

CXXFLAGS += -DBUILDVERSION=\"$(BUILDVERSION)\"
CXXFLAGS += -DBUILDDATE=\"$(BUILDDATE)\"
CXXFLAGS += -DBUILDUSER=\"$(BUILDUSER)\"
CXXFLAGS += -D__TIME_UNIX__=$(UNIXTIME)

#Name of ISO image to build
GAMEIMG=$(IMGDIR)/game.iso

#Which savegame image to append after ISO image
SAVEIMG=$(PVMKSDK)/savepart/save64mb.jpfs

#Append game filesystem + save filesystem to make card image
$(IMGDIR)/card.img : $(GAMEIMG) $(SAVEIMG)
	mkdir -p $(@D)
	cp $(GAMEIMG) $(GAMEIMG).padded
	truncate -s%4096 $(GAMEIMG).padded
	cat $(GAMEIMG).padded $(SAVEIMG) > $@

#Build filesystem with the game contents
$(GAMEIMG) : $(BINDIR)/boot.nne $(shell find -H $(CARDROOT))
	mkdir -p $(@D)
	mkisofs -R -posix-L -o $@ $(CARDROOT)
	
#This symlink already exists but Make gets confused. We're building bin/boot.nne and using it as cardroot/boot.nne.
$(CARDROOT)/boot.nne : $(BINDIR)/boot.nne

#Link post-process target - no-nonsense executable made by stripping the ELF
$(BINDIR)/boot.nne : $(BINDIR)/boot.elf
	mkdir -p $(@D)
	$(OBJCOPY) -O binary $< $@

#Link stage target - ELF made from linking all objects
$(BINDIR)/boot.elf : $(COBJ) $(CXXOBJ)
	mkdir -p $(@D)
	$(CXX) $(LINKFLAGS) $^ -o $@

#Compile targets - object for each C file
$(OBJDIR)/%.c.o : $(SRCDIR)/%.c
	mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@

#Compile targets - object for each C++ file
$(OBJDIR)/%.cpp.o : $(SRCDIR)/%.cpp
	mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -c $< -o $@
	
#Cleanup target
clean:
	rm -rf $(BINDIR) $(OBJDIR) $(IMGDIR)
