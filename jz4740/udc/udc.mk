SOURCES	+= $(wildcard $(UDCDIR)/*.c)
CFLAGS += -I$(UDCDIR)
VPATH  += $(UDCDIR)
