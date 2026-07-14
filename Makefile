.PPHONY: all superc test clean install

CC      = gcc
CFLAGS  = -O2 -Wall -Wextra -fopenmp -march=native
LDFLAGS = -lm -fopenmp

SRC     = c/glm.c
BIN     = superc

all: $(BIN)

$(BIN): $(SRC) c/glm.h c/kernels.h c/tokenizer.h c/network.h
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

test: $(BIN)
	./$(BIN) selftest

clean:
	rm -f $(BIN)

install: $(BIN)
	install -d $(DESTDIR)/usr/local/bin
	install -m 755 $(BIN) $(DESTDIR)/usr/local/bin/$(BIN)