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

# select which module to compile, set value to 1 for the module to compile, 0 for the module not to compile
# -----------------------------
# CAMERA dependent on JPEG
# LWIP dependent on UCFS
# UCGUI denpedent on JPEG
# UCFS denpedent on MMC
# JPEG dependent on UCFS

CAMERA = 0
LWIP   = 1
UCGUI  = 0
UCFS   = 1
MMC    = 1
JPEG   = 0
TOUCH  = 0
NAND   = 1
MP3    = 1
I2C    = 1
#LCDTYPE :
#           0. No LCD Driver
#           1. Samsung tft 480 * 272
#           2. Truly-tft-g320*240drsw
#           3. FoxConn_pt035tn01
LCDTYPE  = 1
#CODECTYPE :
#           0. No Codec Driver
#           1. jz4740 Codec
#           2. akm4642
#           3. ac97
CODECTYPE    = 2
#lwip
LWIP30 = 1

# ------------------------------
CC	:= mipsel-linux-gcc
AR	:= mipsel-linux-ar rcsv
LD	:= mipsel-linux-ld
OBJCOPY	:= mipsel-linux-objcopy
NM	:= mipsel-linux-nm
OBJDUMP	:= mipsel-linux-objdump

CFLAGS	:= -mips32 -O2 -mno-abicalls -fno-pic -fno-builtin \
	   -fno-exceptions -ffunction-sections -finit-priority \
	   -fomit-frame-pointer -msoft-float -G 0

LIBS	:= -lstdc++ -lc -lm -lgcc

TOP	:= ..

OSDIR	:= $(TOP)/src
ARCHDIR	:= $(TOP)/mips
SOCDIR	:= $(TOP)/jz4730
MMCDIR	:= $(TOP)/mmc
UCFSDIR := $(TOP)/ucfs
LWIPDIR := $(TOP)/lwip/src
UCGUIDIR := $(TOP)/ucgui
NANDDIR  := $(TOP)/jz4730/nand
LCDDIR   := $(TOP)/jz4730/drv/lcd
CODECDIR := $(TOP)/jz4730/drv/codec
MP3DIR   := $(TOP)/madplay/libmad-0.15.1b
ID3DIR   := $(TOP)/madplay/libid3tag-0.15.1b
I2CDIR   := $(TOP)/jz4730/drv/i2c
LIBDIR	:= 

SOURCES	:= $(wildcard $(SOCDIR)/*.c) \
	   $(wildcard $(ARCHDIR)/*.c) \
	   $(OSDIR)/ucos_ii.c
SOURCES	+= $(wildcard $(ARCHDIR)/*.S)

HEADS	:= $(SOCDIR)/head.S $(SOCDIR)/sleep.S
CFLAGS	+= -I$(OSDIR) -I$(ARCHDIR) -I$(SOCDIR)/include  

CFLAGS += -DCAMERA=$(CAMERA) \
          -DUCGUI=$(UCGUI)   \
          -DUCFS=$(UCFS)     \
          -DMMC=$(MMC)       \
          -DJPEG=$(JPEG)     \
          -DTOUCH=$(TOUCH)   \
          -DNAND=$(NAND)     \
          -DCODECTYPE=$(CODECTYPE)     \
          -DMP3=$(MP3)  \
          -DI2C=$(I2C)	\
          -DLWIP30=$(LWIP30)
CFLAGS	+= -DLCDTYPE=$(LCDTYPE)
CFLAGS	+= -DCODECDIR=$(CODECDIR)

VPATH	:= $(ARCHDIR) $(SOCDIR) $(OSDIR) 

ifneq ($(CODECTYPE),0)
SOURCES	+= $(wildcard $(CODECDIR)/*.c)
CFLAGS	+= -I$(CODECDIR)
VPATH  +=  $(CODECDIR)
endif

ifneq ($(LCDTYPE),0)
SOURCES	+= $(wildcard $(LCDDIR)/*.c)
CFLAGS	+= -I$(LCDDIR)
VPATH  +=  $(LCDDIR)
endif

ifeq ($(MMC),1)
SOURCES	+= $(MMCDIR)/mmc.c
SOURCES	+= $(MMCDIR)/mmc_jz.c
CFLAGS	+= -I$(MMCDIR)         
VPATH  +=  $(MMCDIR)
endif

ifeq ($(UCFS),1)
SOURCES	+= $(UCFSDIR)/ucfs.c
#SOURCES += $(wildcard $(UCFSDIR)/*.c)
CFLAGS	+= -I$(UCFSDIR)
VPATH  += $(UCFSDIR)
endif

ifeq ($(LWIP),1)
SOURCES	+= $(LWIPDIR)/lwip.c 
CFLAGS	+= -I$(LWIPDIR)  -I$(LWIPDIR)/include -I$(LWIPDIR)/include/lwip 
VPATH  += $(LWIPDIR)
endif

ifeq ($(UCGUI),1)

SOURCES	+= $(wildcard $(UCGUIDIR)/Core/*.c)         \
           $(wildcard $(UCGUIDIR)/LCDDriver/*.c)    \
           $(wildcard $(UCGUIDIR)/Font/*.c)         \
	   $(wildcard $(UCGUIDIR)/ConvertColor/*.c) \
           $(wildcard $(UCGUIDIR)/WM/*.c)           \
           $(wildcard $(UCGUIDIR)/Widget/*.c)       \
           $(wildcard $(UCGUIDIR)/JPEG/*.c)         
CFLAGS	+= -I$(UCGUIDIR)/Font                       \
           -I$(UCGUIDIR)/LCDDriver                  \
           -I$(UCGUIDIR)/Config                     \
           -I$(UCGUIDIR)/Core                       \
           -I$(UCGUIDIR)/ConvertColor               \
           -I$(UCGUIDIR)/WM                         \
           -I$(UCGUIDIR)/Widget                     \
           -I$(UCGUIDIR)/JPEG
VPATH  += $(UCGUIDIR) $(UCGUIDIR)/Core              \
          $(UCGUIDIR)/LCDDriver $(UCGUIDIR)/Font    \
          $(UCGUIDIR)/Config                        \
          $(UCGUIDIR)/ConvertColor                  \
          $(UCGUIDIR)/WM  $(UCGUIDIR)/Widget        \
          $(UCGUIDIR)/JPEG
endif

ifeq ($(CAMERA),1)
SOURCES	+= $(wildcard $(SOCDIR)/camera/*.c) $(SOCDIR)/drv/i2c/i2c.c 
CFLAGS	+= -I$(SOCDIR)/camera 
CFLAGS	+= -DUSE_CAMERA
VPATH   += $(SOCDIR)/camera $(SOCDIR)/drv/i2c
endif

ifeq ($(JPEG),1)
ifeq ($(UCGUI),0)
SOURCES	+= $(wildcard $(UCGUIDIR)/JPEG/*.c)
CFLAGS	+= -I$(UCGUIDIR)/JPEG
VPATH   += $(UCGUIDIR)/JPEG
endif
endif

ifeq ($(TOUCH),1)
SOURCES	+= $(wildcard $(SOCDIR)/drv/touch/touch.c)
CFLAGS	+= -I$(SOCDIR)/drv/touch
CFLAGS	+= -DTOUCH
VPATH  +=   $(SOCDIR)/drv/touch  
endif 

ifeq ($(NAND),1)
SOURCES	+= $(wildcard $(NANDDIR)/*.c)
CFLAGS	+= -I$(NANDDIR)
VPATH  +=  $(NANDDIR)
endif 

ifeq ($(MP3),1)
SOURCES	+= $(wildcard $(MP3DIR)/*.c)
CFLAGS	+= -I$(MP3DIR)
VPATH  +=  $(MP3DIR)
SOURCES	+= $(wildcard $(ID3DIR)/*.c)
CFLAGS	+= -I$(ID3DIR)
VPATH  +=  $(ID3DIR)
endif 

ifeq ($(I2C),1)
SOURCES	+= $(wildcard $(I2CDIR)/*.c)
CFLAGS	+= -I$(I2CDIR)
VPATH  +=  $(I2CDIR)
endif

OBJS	:= $(addsuffix .o , $(basename $(notdir $(SOURCES))))
HEADO	:= $(addsuffix .o , $(basename $(notdir $(HEADS))))

APP	:= ucos.elf
all:	$(APP)
	$(OBJCOPY) -O binary $(APP) ucos.bin
	$(OBJDUMP) -d $(APP) > ucos.dump
	$(NM) $(APP) | sort > ucos.sym
	$(OBJDUMP) -h $(APP) > ucos.map

$(APP):	$(HEADO) $(OBJS) $(EXTLIBS) link.xn makefile
	$(CC) -nostdlib -T link.xn -o $@ $(HEADO) $(OBJS) $(EXTLIBS) $(LIBS)

.c.o:
	$(CC) $(CFLAGS) -o $@ -c $<
.cpp.o:
	$(CC) $(CFLAGS) -fno-rtti -fvtable-gc -o $@ -c $<
.S.o:
	$(CC) $(CFLAGS) -D_ASSEMBLER_ -D__ASSEMBLY__ -o $@ -c $<

clean:
	rm -fr *.o $(APP) $(OBJS) core $(OTHER) *.sym *.map *.dump *.bin *.lib

