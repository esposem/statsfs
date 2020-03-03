#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "statsfs.h"

int main(int argc, char *argv[]){
    statsfs_source_create("Ciao amico %d\n", 43);
}