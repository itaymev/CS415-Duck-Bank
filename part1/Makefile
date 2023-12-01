CC=gcc
CFLAGS=-g
DEPS = string_parser.h bank.h account.h
OBJECTS = bank.o string_parser.o
PROGRAMS = bank

all: $(PROGRAMS)

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

bank: bank.o string_parser.o
	$(CC) -o $@ $^ $(CFLAGS)

clean:
	rm -f $(OBJECTS) $(PROGRAMS)