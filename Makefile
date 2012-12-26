CFLAGS = -Wall -O2 -s -Werror
LDFLAGS = -lusb -lftdi -s
PROG = ftx_prog

all:	$(PROG)

$(PROG):	$(PROG).c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

clean:
	rm -f $(PROG)
