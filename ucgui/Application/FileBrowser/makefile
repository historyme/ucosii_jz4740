# 
# 
# Author: Seeger Chin
# e-mail: seeger.chin@gmail.com
# 
# Copyright (C) 2006 Ingenic Semiconductor Inc.
# 
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 2 as
# published by the Free Software Foundation.
# 
# 

# select which module to compile, set value to 1 for the module to compile, remain blank for the module not to compile
# -----------------------------

# ------------------------------

CC	:= mipsel-linux-gcc
AR	:= mipsel-linux-ar  rcsv
LD	:= mipsel-linux-ld
OBJCOPY	:= mipsel-linux-objcopy
NM	:= mipsel-linux-nm
OBJDUMP	:= mipsel-linux-objdump

CFLAGS	:= -mips32 -O2 -mno-abicalls -fno-pic -fno-builtin \
	   -fno-exceptions -ffunction-sections -finit-priority \
	   -fomit-frame-pointer -msoft-float -G 0

LIBS := -lstdc++ -lc -lm -lgcc
COMMONDIR := ../Common
RESDIR := ../Res
UCGUIDIR := ../..
UCFSDIR  := ../../../ucfs
TOP	:= ..
LIBDIR	:= ../../../LIBS

LIBNAME := filebrs.a

SOURCES	= $(wildcard *.c)
CFLAGS	+= -I$(UCGUIDIR)/Font -I$(UCGUIDIR)/LCDDriver -I$(UCGUIDIR)/Config -I$(UCGUIDIR)/Core \
	   -I$(UCGUIDIR)/ConvertColor \
	   -I$(UCGUIDIR)/WM           \
	   -I$(UCGUIDIR)/Widget       \
	   -I$(UCGUIDIR)/MemDev
CFLAGS	+= -I$(UCFSDIR)
CFLAGS	+= -I$(COMMONDIR)
CFLAGS	+= -I$(RESDIR)


VPATH   += $(UCGUIDIR) $(CORE)  $(UCGUIDIR)/LCDDriver $(UCGUIDIR)/Font $(UCGUIDIR)/Config \
           $(UCGUIDIR)/ConvertColor \
	   $(UCGUIDIR)/WM           \
	   $(UCGUIDIR)/Widget       \
		 $(UCGUIDIR)/MemDev
VPATH   +=  $(UCFSDIR)
VPATH   +=  $(COMMONDIR)
VPATH   +=  $(RESDIR)

OBJS	:= $(addsuffix .o , $(basename $(notdir $(SOURCES))))

.SUFFIXES: .c .o
all: $(OBJS)
	$(CC) -c $(CFLAGS) -o $@ $<
	$(AR) $(LIBDIR)/$(LIBNAME) $(OBJS)
#	$(RANLIB) $(LIBDIR)//$(LIBNAME)	
.c.o:
	$(CC) $(CFLAGS) -o $@ -c $<

.cpp.o:
	$(CC) $(CFLAGS) -fno-rtti -fvtable-gc -o $@ -c $<
.S.o:
	$(CC) $(CFLAGS) -D_ASSEMBLER_ -D__ASSEMBLY__ -o $@ -c $<
clean:
	rm -f *~ *.o $(LIBDIR)/$(LIBNAME) makefile.bak .depend

dep:    depend

depend:
	$(CC) -MM $(CFLAGS) $(SRCS) 1>.depend