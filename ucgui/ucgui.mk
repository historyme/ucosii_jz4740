
SOURCES	+= $(wildcard $(UCGUIDIR)/Core/*.c) $(wildcard $(UCGUIDIR)/LCDDriver/*.c) \
	   $(wildcard $(UCGUIDIR)/Font/*.c)        \
	   $(wildcard $(UCGUIDIR)/ConvertColor/*.c) \
	   $(wildcard $(UCGUIDIR)/WM/*.c) \
	   $(wildcard $(UCGUIDIR)/Widget/*.c) \
		 $(wildcard $(UCGUIDIR)/MemDev/*.c)	
CFLAGS	+= -I$(UCGUIDIR)/Font -I$(UCGUIDIR)/LCDDriver -I$(UCGUIDIR)/Config -I$(UCGUIDIR)/Core \
	   -I$(UCGUIDIR)/ConvertColor \
	   -I$(UCGUIDIR)/WM           \
	   -I$(UCGUIDIR)/Widget       \
	   -I$(UCGUIDIR)/MemDev
VPATH   += $(UCGUIDIR) $(CORE)  $(UCGUIDIR)/LCDDriver $(UCGUIDIR)/Font $(UCGUIDIR)/Config \
           $(UCGUIDIR)/ConvertColor \
	   $(UCGUIDIR)/WM           \
	   $(UCGUIDIR)/Widget       \
		 $(UCGUIDIR)/MemDev