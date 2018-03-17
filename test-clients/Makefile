IDIR = .
ODIR = .

CC     = gcc
CFLAGS = -I$(IDIR) -Wall
LIBS   = -lwebsockets

.PHONY: all
all: echo-client bulletin-client

echo-client: echo-client.c
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

bulletin-client: bulletin-client.c
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

.PHONY: clean 
clean:
	rm -f $(ODIR)/echo-client
	rm -f $(ODIR)/bulletin-client

