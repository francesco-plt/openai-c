CC = gcc
DBGFLAGS = -g -Wall
CFLAGS = -lcurl

build: build/api

build/api: src/api.c
	mkdir -p build
	$(CC) $(CFLAGS) $(DBGFLAGS) -o $@ $<

run: build/api
	./build/api

clean:
	rm -rf build

.PHONY: build run clean
