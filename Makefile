all: main.o statsfs.o
	gcc -Werror -o statsfs main.o statsfs.o statsfs_internal.o

main.o: main.c statsfs.h
	gcc -Werror -c main.c

statsfs.o: statsfs_internal.c statsfs.c statsfs_internal.h statsfs.h list.h
	gcc -Werror -c statsfs_internal.c statsfs.c
