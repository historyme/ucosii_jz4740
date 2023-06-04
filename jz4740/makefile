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
JZ4740_PAV = 1
#
#TOUCHTYPE :
#           0. No Touch driver
#           1. AK4182
#           2. JZ4740SADC
#
TOUCHTYPE  = 2

#LCDTYPE :
#           0. No LCD Driver
#           1. Samsung tft 480 * 272
LCDTYPE    = 1
#RTCTYPE :
#           0. No RTC Driver
#           1. jz4740RTC
RTCTYPE    = 1

#CODECTYPE :
#           0. No Codec Driver
#           1. jz4740 Codec
CODECTYPE = 1
WAVDECODE = 0
#KeyCTYPE :
#           0. No Key Driver
#           1. jz4740 Key
KEYTYPE = 1

CAMERA = 0
LWIP   = 0
UCGUI  = 1
UCFS   = 1
MMC    = 1
JPEG   = 1
UDC    = 1
MP3    = 1
NAND   = 1
UDCCPU = 0
GUIDEMO = 1
DM = 0
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
COREDIR	 := $(TOP)/jz4740/core
OSDIR	 := $(TOP)/src
ARCHDIR	 := $(TOP)/mips
SOCDIR	 := $(TOP)/jz4740
MMCDIR	 := $(TOP)/mmc
UCFSDIR  := $(TOP)/ucfs
LWIPDIR  := $(TOP)/lwip/src
UCGUIDIR := $(TOP)/ucgui
CORE     := $(TOP)/ucgui/Core
TOUCHDIR := $(TOP)/jz4740/drv/touch
UDCDIR   := $(TOP)/jz4740/udc_new
LCDDIR   := $(TOP)/jz4740/drv/lcd
RTCDIR   := $(TOP)/jz4740/drv/rtc
CODECDIR := $(TOP)/jz4740/drv/codec
WAVDIR   := $(TOP)/audio/wave
KEYDIR   := $(TOP)/jz4740/drv/key
MP3DIR   := $(TOP)/madplay/libmad-0.15.1b
ID3DIR   := $(TOP)/madplay/libid3tag-0.15.1b
NANDDIR  := $(TOP)/jz4740/nand
UDCCPUDIR := $(TOP)/jz4740/udc
LIBDIR	:= $(TOP)/LIBS
ifeq ($(GUIDEMO),1)
LIBS += $(wildcard $(LIBDIR)/*.a)
endif
CFLAGS  += -DJZ4740_PAV=$(JZ4740_PAV)

CFLAGS += -DCAMERA=$(CAMERA) \
          -DUCGUI=$(UCGUI)   \
          -DUCFS=$(UCFS)     \
          -DMMC=$(MMC)       \
          -DJPEG=$(JPEG)     \
          -DTOUCH=$(TOUCHTYPE)


include  $(COREDIR)/core.mk

ifeq ($(UDCCPU),1)
include  $(UDCCPUDIR)/udc.mk
endif

ifeq ($(NAND),1)
include  $(NANDDIR)/nand.mk
endif


ifneq ($(KEYTYPE),0)
include $(KEYDIR)/key.mk
endif

ifeq ($(WAVDECODE),1)
include $(WAVDIR)/wave.mk
endif

ifneq ($(CODECTYPE),0)
include $(CODECDIR)/codec.mk
endif

ifneq ($(RTCTYPE),0)
include  $(RTCDIR)/rtc.mk
endif

ifneq ($(LCDTYPE),0)
include $(LCDDIR)/lcd.mk
endif

ifneq ($(TOUCHTYPE),0)
include $(TOUCHDIR)/touch.mk
endif

ifeq ($(MMC),1)
include $(MMCDIR)/mmc.mk
endif

ifeq ($(UCFS),1)
include $(UCFSDIR)/ucfs.mk
endif

ifeq ($(LWIP),1)
include $(LWIPDIR)/lwip.mk
endif

ifeq ($(UCGUI),1)
include $(UCGUIDIR)/ucgui.mk
endif

ifeq ($(CAMERA),1)
SOURCES	+= $(wildcard $(SOCDIR)/camera/*.c) $(SOCDIR)/drv/i2c.c 
CFLAGS += -I$(SOCDIR)/camera
VPATH  += $(SOCDIR)/camera
endif

ifeq ($(JPEG),1)
SOURCES	+= $(wildcard $(UCGUIDIR)/JPEG/*.c)
CFLAGS += -I$(UCGUIDIR)/JPEG
VPATH  += $(UCGUIDIR)/JPEG
endif

ifeq ($(UDC),1)
include $(UDCDIR)/udc.mk
endif

ifeq ($(MP3),1)
include $(wildcard $(MP3DIR)/mp3.mk)
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
	rm -fr *.o $(APP) $(OBJS) $(OTHER) *.sym *.map *.dump *.bin *.lib

