CC=gcc
OUT=./build/cgs
SRC=cgs.c
FLAGS=-lpthread
VFLAGS=--leak-check=full --show-leak-kinds=all

default_recipe=cgs

release:
	mkdir -p build && $(CC) -static -s $(SRC) -o $(OUT) $(FLAGS) 

cgs: cgs.c
	mkdir -p build && $(CC) $(SRC) -o $(OUT) $(FLAGS) 

install: cgs
	sudo cp $(shell pwd)/build/cgs /usr/bin/

uninstall:
	sudo rm /usr/bin/cgs

val: cgs.c
	$(CC) $(FLAGS) $(SRC) -o $(OUT) && valgrind $(VFLAGS) $(OUT)

