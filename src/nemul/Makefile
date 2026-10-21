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

#Headless build: the same emulation core, no wxWidgets and no display.
#The core - interp, process, sysc, rsp, trace - never mentions wx; only the front end
#(nemul.cpp), the config window, the pad binding dialog and prefs.cpp do. So the console
#front end links the core plus its own main and its own prefs.
HLESS_CPPSRC := $(filter-out $(SRCDIR)/nemul.cpp $(SRCDIR)/cfgwin.cpp $(SRCDIR)/bindprompt.cpp $(SRCDIR)/compare_interp.cpp $(SRCDIR)/prefs.cpp, $(CPPSRC))
HLESS_CPPSRC += $(SRCDIR)/nemul_headless.cpp $(SRCDIR)/prefs_headless.cpp
HLESS_CPPOBJ := $(patsubst $(SRCDIR)/%.cpp, $(OBJDIR)/headless/%.cpp.o, $(HLESS_CPPSRC))

CPPFLAGS_HEADLESS = -std=c++20 -pedantic -Wall -Werror -Wextra -gdwarf -O2
CPPFLAGS_HEADLESS += -Wno-inconsistent-missing-override
CPPFLAGS_HEADLESS += -DBUILDVERSION=\"$(BUILDVERSION)\"
CPPFLAGS_HEADLESS += -DBUILDDATE=\"$(BUILDDATE)\"
CPPFLAGS_HEADLESS += -DBUILDUSER=\"$(BUILDUSER)\"
CPPFLAGS_HEADLESS += -D__TIME_UNIX__=$(UNIXTIME)

$(BINDIR)/nemul-headless.elf : $(HLESS_CPPOBJ)
	mkdir -p $(@D)
	$(CPP) $(LINKFLAGS) $^ -lm -o $@

$(OBJDIR)/headless/%.cpp.o : $(SRCDIR)/%.cpp
	mkdir -p $(@D)
	$(CPP) $(CPPFLAGS_HEADLESS) $< -c -o $@

#Header dependencies are not tracked, so rebuild everything when any source is newer than the
#binary: far better than wondering why a change did nothing.
headless : $(BINDIR)/nemul-headless.elf
	@if [ -n "$$(find $(SRCDIR) -name '*.cpp' -newer $(BINDIR)/nemul-headless.elf -print -quit 2>/dev/null)" ]; then \
		rm -f $(BINDIR)/nemul-headless.elf; \
		$(MAKE) --no-print-directory $(BINDIR)/nemul-headless.elf; \
	fi

clean : .
	rm -rf $(OBJDIR)
	rm -rf $(BINDIR)

