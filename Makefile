CC=gcc
OUT=./build/cgs
SRC=cgs.c
FLAGS=-lpthread
VFLAGS=--leak-check=full --show-leak-kinds=all

default_recipe=cgs

cgs: cgs.c
	mkdir -p build && $(CC) $(FLAGS) $(SRC) -o $(OUT)

install: cgs
	sudo ln -s $(shell pwd)/build/cgs /usr/bin/cgs

val: cgs.c
	$(CC) $(FLAGS) $(SRC) -o $(OUT) && valgrind $(VFLAGS) $(OUT)

