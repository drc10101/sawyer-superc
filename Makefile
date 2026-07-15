.PHONY: all superc test clean install

CC      = gcc
CFLAGS  = -O2 -Wall -Wextra -fopenmp -march=native
LDFLAGS = -lm -fopenmp

# Colibri engine: rename main() to coli_main() so superc.c owns the entry point
COLIBRI_CFLAGS = -Dmain=coli_main

# Colibri engine (unmodified upstream + our network extension)
COLIBRI_SRC = c/colibri/glm.c
COLIBRI_HDR = c/colibri/st.h c/colibri/tok.h c/colibri/tok_unicode.h \
              c/colibri/tier.h c/colibri/grammar.h c/colibri/schema_gbnf.h \
              c/colibri/decode_batch.h c/colibri/compat.h c/colibri/json.h

# Sawyer network layer (our code)
SAWYER_SRC = c/sawyer_net.c
SAWYER_HDR = c/sawyer_net.h

# SuperC CLI (our entry point, wraps Colibri engine + Sawyer network)
SUPER_SRC = c/superc.c
SUPER_HDR = c/superc.h

BIN     = superc

all: $(BIN)

$(BIN): $(SUPER_SRC) $(SAWYER_SRC) $(COLIBRI_SRC) $(SUPER_HDR) $(SAWYER_HDR) $(COLIBRI_HDR)
	$(CC) $(CFLAGS) -c -o coli_main.o $(COLIBRI_SRC) $(COLIBRI_CFLAGS)
	$(CC) $(CFLAGS) -o $@ $(SUPER_SRC) $(SAWYER_SRC) coli_main.o $(LDFLAGS)
	rm -f coli_main.o

test: $(BIN)
	./$(BIN) selftest

clean:
	rm -f $(BIN)

install: $(BIN)
	install -d $(DESTDIR)/usr/local/bin
	install -m 755 $(BIN) $(DESTDIR)/usr/local/bin/$(BIN)