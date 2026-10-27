#Makefile for Neki32 emulator
#Bryan E. Topp <betopp@betopp.com> 2025

#Directories in project
SRCDIR=src
OBJDIR=obj
BINDIR=bin
IMGUISRC=third_party/imgui

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
CPPFLAGS += -Wno-inconsistent-missing-override

#The graphical front end needs SDL2 and nothing else: Dear ImGui is vendored in
#third_party/imgui and draws through SDL_Renderer, so there is no OpenGL, no native widget
#toolkit and no per-platform window code. Find SDL2 with pkg-config, then sdl2-config, and
#fall back to where ports and Homebrew put it.
SDL2_CFLAGS := $(shell pkg-config --cflags sdl2 2>/dev/null)
SDL2_LIBS := $(shell pkg-config --libs sdl2 2>/dev/null)
ifeq ($(strip $(SDL2_LIBS)),)
SDL2_CFLAGS := $(shell sdl2-config --cflags 2>/dev/null)
SDL2_LIBS := $(shell sdl2-config --libs 2>/dev/null)
endif
ifeq ($(strip $(SDL2_LIBS)),)
SDL2_CFLAGS := -I/usr/local/include/SDL2
SDL2_LIBS := -L/usr/local/lib -lSDL2
endif

#Static SDL2, for the copy that ships in the SDK: a packaged emulator should not need SDL2
#installed on the machine that runs it. Only libSDL2 itself comes out of its archive -
#everything it stands on (X11, Wayland, DRM, pthread) stays shared, because desktop systems do
#not ship static versions of those, and SDL2 is the one library a user would not already have.
#Ask for it with "gmake SDL2STATIC=1"; nemul_build.sh does, and falls back to the shared library
#if the static one is not installed.
#
#Two rewrites of what pkg-config reports are needed. "-lSDL2" becomes a bracket around the
#static library so that only it is taken from an archive. And pkg-config's static list names its
#dependencies as "-l/usr/local/lib/libX11.so", which the linker reads as a library *name* and
#cannot find - a plain path to the same file is what it wants. GNU Make has no comma, hence
#$(comma).
comma := ,
ifeq ($(SDL2STATIC),1)
SDL2_LIBS := $(shell pkg-config --libs --static sdl2 2>/dev/null | sed -e 's|-lSDL2|-Wl$(comma)-Bstatic -lSDL2 -Wl$(comma)-Bdynamic|' -e 's|-l\(/[^ ]*\.so\)|\1|g')
ifeq ($(strip $(SDL2_LIBS)),)
SDL2_LIBS := -Wl$(comma)-Bstatic -L/usr/local/lib -lSDL2 -Wl$(comma)-Bdynamic -pthread -lm
endif
endif

#Dear ImGui is third-party code and does not promise to compile warning-free under -Werror,
#so it gets its own flags. Everything of ours still has to be clean.
IMGUIPFLAGS=-std=c++20 -Wall -Wextra -gdwarf -O2 -Wno-unused-parameter -Wno-missing-field-initializers
IMGUICPP := $(IMGUISRC)/imgui.cpp $(IMGUISRC)/imgui_draw.cpp $(IMGUISRC)/imgui_tables.cpp \
	$(IMGUISRC)/imgui_widgets.cpp $(IMGUISRC)/backends/imgui_impl_sdl2.cpp \
	$(IMGUISRC)/backends/imgui_impl_sdlrenderer2.cpp
IMGUIOBJ := $(patsubst $(IMGUISRC)/%.cpp, $(OBJDIR)/imgui/%.cpp.o, $(IMGUICPP))

#C++ sources to compile
CPPSRC:=$(shell find $(SRCDIR) -name *.cpp)

#Graphical front end: the core plus the ImGui/SDL front end, its dialogs and its prefs.
#nemul_headless.cpp and prefs_headless.cpp belong to the console build instead, because they
#provide the other main() and the other prefs_read()/prefs_write().
GUI_CPPSRC := $(filter-out $(SRCDIR)/nemul_headless.cpp $(SRCDIR)/prefs_headless.cpp, $(CPPSRC))
GUI_CPPOBJ := $(patsubst $(SRCDIR)/%.cpp, $(OBJDIR)/gui/%.cpp.o, $(GUI_CPPSRC))

#Top-level target linked from the front end, the core and the vendored UI library
$(BINDIR)/nemul.elf : $(GUI_CPPOBJ) $(IMGUIOBJ)
	mkdir -p $(@D)
	$(CPP) $(LINKFLAGS) $^ $(LIBS) $(SDL2_LIBS) -o $@

#Objects made from source files
#Our own include paths come first: a machine with another Dear ImGui installed (a port puts
#one in /usr/local/include, and SDL2's own cflags drag that directory in) must not be able to
#shadow the vendored copy, or the front end and the library disagree about struct layout
$(OBJDIR)/gui/%.cpp.o : $(SRCDIR)/%.cpp
	mkdir -p $(@D)
	$(CPP) $(CPPFLAGS) -I$(IMGUISRC) -I$(IMGUISRC)/backends $(SDL2_CFLAGS) $< -c -o $@

#Objects made from the vendored UI library
$(OBJDIR)/imgui/%.cpp.o : $(IMGUISRC)/%.cpp
	mkdir -p $(@D)
	$(CPP) $(IMGUIPFLAGS) -I$(IMGUISRC) -I$(IMGUISRC)/backends $(SDL2_CFLAGS) $< -c -o $@

#Headless build: the same emulation core, no display at all.
#The core - interp, process, sysc, rsp, trace - never mentions the front end; only nemul.cpp,
#the dialogs (ui_prefs, ui_files) and prefs.cpp do. So the console front end links the core
#plus its own main and its own prefs.
HLESS_CPPSRC := $(filter-out $(SRCDIR)/nemul.cpp $(SRCDIR)/ui_prefs.cpp $(SRCDIR)/ui_files.cpp $(SRCDIR)/ui_debug.cpp $(SRCDIR)/prefs.cpp, $(CPPSRC))
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
gui : $(BINDIR)/nemul.elf
	@if [ -n "$$(find $(SRCDIR) $(IMGUISRC) -name '*.cpp' -newer $(BINDIR)/nemul.elf -print -quit 2>/dev/null)" ]; then \
		rm -f $(BINDIR)/nemul.elf; \
		$(MAKE) --no-print-directory $(BINDIR)/nemul.elf; \
	fi

headless : $(BINDIR)/nemul-headless.elf
	@if [ -n "$$(find $(SRCDIR) -name '*.cpp' -newer $(BINDIR)/nemul-headless.elf -print -quit 2>/dev/null)" ]; then \
		rm -f $(BINDIR)/nemul-headless.elf; \
		$(MAKE) --no-print-directory $(BINDIR)/nemul-headless.elf; \
	fi

clean : .
	rm -rf $(OBJDIR)
	rm -rf $(BINDIR)
