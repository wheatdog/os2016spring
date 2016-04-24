all: main child
OBJ = main.o psjf.o fifo.o sjf.o
CFLAG = -std=c99

%.o: %.c
	gcc $(CFLAG) $< -c -o $@

main: $(OBJ)
	gcc $(CFLAG) $^ -o main

child: child.o
	gcc $(CFLAG) $^ -o $@

clean:
	rm $(OBJ)
