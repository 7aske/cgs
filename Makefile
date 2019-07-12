CC=gcc
OUT=./build/cgs
SRC=cgs.c
FLAGS=-lpthread
VFLAGS=--leak-check=full --show-leak-kinds=all

default_recipe=cgs

cgs: cgs.c
	$(CC) $(FLAGS) $(SRC) -o $(OUT) && $(OUT)

val: cgs.c
	$(CC) $(FLAGS) $(SRC) -o $(OUT) && valgrind $(VFLAGS) $(OUT)

