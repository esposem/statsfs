all: main.o statsfs.o
	gcc -Wall -Werror -o statsfs main.o statsfs.o test_helpers.c

main.o: main.c statsfs.h test_helpers.h
	gcc -Wall -Werror -c main.c test_helpers.c

statsfs.o: statsfs.c statsfs.h list.h statsfs_internal.h
	gcc -Wall -Werror -c statsfs.c test_helpers.c
