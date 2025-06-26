#Makefile for Neki32 emulator
#Bryan E. Topp <betopp@betopp.com> 2025

#Directories in project
SRCDIR=src
OBJDIR=obj
BINDIR=bin

#Toolchain
CPP=c++
CPPFLAGS=-std=c++20 -pedantic -Wall -Werror -Wextra -gdwarf -O2
LINKFLAGS=-std=c++20
LIBS=-lm

UNAME_O := $(shell uname -o)
ifeq ($(UNAME_O),Msys)
	LIBS+=-lws2_32
endif

#Build information provided to compiler
BUILDVERSION := $(shell git describe --abbrev=4 --dirty --always --tags)
BUILDDATE := $(shell date -u +%FT%TZ)
BUILDUSER := $(shell whoami)@$(shell hostname)
UNIXTIME := $(shell date +%s)
CPPFLAGS += -DBUILDVERSION=\"$(BUILDVERSION)\"
CPPFLAGS += -DBUILDDATE=\"$(BUILDDATE)\"
CPPFLAGS += -DBUILDUSER=\"$(BUILDUSER)\"
CPPFLAGS += -D__TIME_UNIX__=$(UNIXTIME)
CPPFLAGS += -static
CPPFLAGS += -Wno-inconsistent-missing-override

LINKFLAGS += -static

#Use wxWidgets built as part of our build process, so we can static-link it
WXCFG=../wx/pfx/bin/wx-config

WXBITS=core,base
CPPFLAGS+=$(shell $(WXCFG) $(WXBITS) --static --cxxflags )
LIBS+=$(shell $(WXCFG) $(WXBITS) --static --libs)

#C++ sources to compile
CPPSRC:=$(shell find $(SRCDIR) -name *.cpp)

#Object files made from C++ sources
CPPOBJ:=$(patsubst %.cpp, $(OBJDIR)/%.cpp.o, $(CPPSRC))

#Top-level target linked from all C++ objects
$(BINDIR)/nemul.elf : $(CPPOBJ)
	mkdir -p $(@D)
	$(CPP) $(LINKFLAGS) $^ $(LIBS) -o $@

#Objects made from source files
$(OBJDIR)/$(SRCDIR)/%.cpp.o : $(SRCDIR)/%.cpp
	mkdir -p $(@D)
	$(CPP) $(CPPFLAGS) $< -c -o $@
	
clean : .
	rm -rf $(OBJDIR)
	rm -rf $(BINDIR)

