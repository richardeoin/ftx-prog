
ifeq ($(USE_LIBFTDI1),1)
CFLAGS_FTDI = -DUSE_LIBFTDI1
LDFLAGS_FTDI = -lftdi1
else
LDFLAGS_FTDI = -lftdi
endif

override CFLAGS += -Wall -O2 -s -pedantic $(CFLAGS_FTDI)
override LDFLAGS += -lusb-1.0 $(LDFLAGS_FTDI) -s

PROG = ftx_prog

all:	$(PROG)

$(PROG):	$(PROG).c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

clean:
	rm -f $(PROG)
