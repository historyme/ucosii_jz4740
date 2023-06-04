SOURCES	+= $(wildcard $(MP3DIR)/*.c)
CFLAGS	+= -I$(MP3DIR)
VPATH  +=  $(MP3DIR)

SOURCES	+= $(wildcard $(ID3DIR)/*.c)
CFLAGS	+= -I$(ID3DIR)
VPATH  +=  $(ID3DIR)