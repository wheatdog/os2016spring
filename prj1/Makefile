TARGET = main child
OBJ = main.o psjf.o fifo.o sjf.o rr.o
CFLAG = -std=c99

all: $(TARGET)

%.o: %.c
	gcc $(CFLAG) $< -c -o $@

main: $(OBJ)
	gcc $(CFLAG) $^ -o main

child: child.o
	gcc $(CFLAG) $^ -o $@

clean:
	rm $(OBJ) $(TARGET)
