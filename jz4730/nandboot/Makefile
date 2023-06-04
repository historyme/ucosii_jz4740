#
# Makefile of the nandboot
#
# Copyright (c) 2005-2007  Ingenic Semiconductor Inc.
#

CROSS	:= mipsel-linux-

CFLAGS	:= -O2 -G 0 -mno-abicalls -fno-pic -mips32
LDFLAGS	:= -nostdlib -EL -Ttarget.ld

nandboot.bin: nandboot
	$(CROSS)objcopy -O binary $< $@

nandboot: head.o loader.o jz4730.o serial.o board-pmp.o
	$(CROSS)ld $(LDFLAGS) $^ -o $@
.c.o:
	$(CROSS)gcc $(CFLAGS) -c $< -o $@
.S.o:
	$(CROSS)gcc $(CFLAGS) -c $< -o $@

clean:
	rm -fr *.o nandboot nandboot.bin

