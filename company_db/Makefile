CC=gcc
CFLAGS=-Wall -O2 `pkg-config --cflags --libs libmongoc-1.0`
.PHONY: debug

sepheaders: sepheaders.c
	$(CC) $(CFLAGS) $(DEBUG) sepheaders.c -o sepheaders

debug:
	$(MAKE) DEBUG=-g
