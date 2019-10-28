gShell: main.o
	gcc main.o -o gShell -lreadline
main.o: main.c
	gcc -Wall -c main.c 

