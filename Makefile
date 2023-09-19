CC = gcc
CFLAGS = -Wall -g -Wextra -pedantic
OBJ = minicron.o list.o task.o

all: minicron

list.o: list.c list.h
	$(CC) $(CFLAGS) -c list.c

task.o: task.c list.h
	$(CC) $(CFLAGS) -c task.c

minicron.o: minicron.c list.h task.h
	$(CC) $(CFLAGS) -c minicron.c list.h task.h

minicron: minicron.o list.o task.o
	$(CC) $(CFLAGS) -o minicron minicron.o list.o task.o
	
leaks:
	valgrind --leak-check=full ./minicron a.txt b

test:
	./minicron a.txt b

clean:
	rm -f *.o minicron
