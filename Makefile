all: main.o statsfs.o
	gcc -Wall -o statsfs main.o statsfs.o

main.o: main.c statsfs.h
	gcc -Wall -c main.c

statsfs.o: statsfs.c statsfs.h list.h
	gcc -Wall -c statsfs.c
