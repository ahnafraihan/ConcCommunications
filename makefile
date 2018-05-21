# Ahnaf Raihan

all:	prog3ipc

prog3ipc:	main.o main.c
	gcc  main.o -o prog3ipc -lrt

main.o:	main.c
	gcc -std=c11 -c -w -g main.c

clean:
	rm -f *.o prog3ipc

