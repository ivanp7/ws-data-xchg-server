IDIR = .
ODIR = .

CC     = gcc
CFLAGS = -I$(IDIR) -Wall
LIBS   = -lwebsockets -lcjson

.PHONY: all
all: echo-client bulletin-client json-client

echo-client: echo-client.c
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

bulletin-client: bulletin-client.c
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

json-client: json-client.c
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

.PHONY: clean 
clean:
	rm -f $(ODIR)/echo-client $(ODIR)/bulletin-client $(ODIR)/json-client

