all: main.o statsfs.o
	gcc -Wall -Werror -o statsfs main.o statsfs.o statsfs_internal.o test_helpers.c

main.o: main.c statsfs.h test_helpers.h
	gcc -Wall -Werror -c main.c test_helpers.c

statsfs.o: statsfs_internal.c statsfs.c statsfs_internal.h statsfs.h list.h
	gcc -Wall -Werror -c statsfs_internal.c statsfs.c test_helpers.c
