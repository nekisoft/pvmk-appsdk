#Makefile for xorriso, PVMK SDK copy
#Bryan E. Topp <betopp@betopp.com> 2025

#Directories in this folder
SRCDIR=src
OBJDIR?=obj
BINDIR?=bin

#Toolchain location (for host system)
CC=gcc

#Tool flags
CFLAGS+=-std=c99 -Wall -Werror -Wextra -D_DEFAULT_SOURCE -D_GNU_SOURCE -D_FILE_OFFSET_BITS=64 
CFLAGS+=-g -O0
CFLAGS+=-I$(SRCDIR)
CFLAGS+=-DHAVE_CONFIG_H=1
CFLAGS+=-static

HASICONV=$(shell ldd `which iconv` | grep iconv)
LIBS+=$(if $(HASICONV), -liconv, )
LINKFLAGS+=-static

#Ugh just build, I don't know why they do these things
CFLAGS+=-Wno-unused-parameter
CFLAGS+=-Wno-stringop-overflow
CFLAGS+=-Wno-format-overflow
CFLAGS+=-Wno-pointer-arith

#Name of executable we're building
OUTFILE=$(BINDIR)/pvmk-xorriso

#Location of source files to build, and their corresponding object files
CSRC=$(shell find $(SRCDIR) -name \*.c)
COBJ=$(patsubst $(SRCDIR)/%.c, $(OBJDIR)/%.o, $(CSRC))

#Final executable made from all object files
$(OUTFILE) : $(COBJ)
	mkdir -p $(@D)
	$(CC) $(LINKFLAGS) $^ $(LIBS) -o $@

#Each object file built from a source file
$(OBJDIR)/%.o : $(SRCDIR)/%.c
	mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJDIR) $(BINDIR)
