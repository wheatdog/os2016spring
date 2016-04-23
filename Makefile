all: main

%.o: %.c
	gcc -std=c99 $< -c -o $@

main: main.o psjf.o
	gcc -std=c99 $^ -o main
