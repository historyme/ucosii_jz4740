SOURCES	+= $(wildcard $(RTCDIR)/*.c)
CFLAGS	+= -DRTCTYPE=$(RTCTYPE)
CFLAGS	+= -I$(RTCDIR)
VPATH   += $(RTCDIR)
