TARGET_NAME = ws-echo

IDIR = .
ODIR = .

CC     = gcc
CFLAGS = -I$(IDIR)
LIBS   = -lwebsockets

GGO    = gengetopt
GGO_FILE_PREFIX = cmdline

_DEPS = $(GGO_FILE_PREFIX).h echoserver.h
DEPS = $(patsubst %,$(IDIR)/%,$(_DEPS))

_OBJ = $(GGO_FILE_PREFIX).o main.o echoserver.o
OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))

$(TARGET_NAME): $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

$(ODIR)/%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

$(GGO_FILE_PREFIX).c: $(GGO_FILE_PREFIX).ggo
	$(GGO) --input=$<

.PHONY: clean clean-all
clean:
	rm -f $(ODIR)/*.o

clean-all:
	rm -f $(ODIR)/*.o $(TARGET_NAME)

