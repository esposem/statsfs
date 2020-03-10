#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>
#include "statsfs.h"
#include "statsfs_internal.h"


int source_has_subsource(struct statsfs_source *src,
                        struct statsfs_source *sub)
{
    struct statsfs_source *entry;
    list_for_each_entry(entry, &src->subordinates_head, list_element) {
        if(entry == sub){
            return 1;
        }
    }
    return 0;
}

int get_number_subsources(struct statsfs_source *src)
{
    struct statsfs_source *entry;
    int counter = 0;
    list_for_each_entry(entry, &src->subordinates_head, list_element) {
        counter++;
    }
    return counter;
}

int get_number_values(struct statsfs_source *src)
{
    struct statsfs_value_source *entry;
    int counter = 0;
    list_for_each_entry(entry, &src->simple_values_head, list_element) {
        for (int i = 0; i < entry->values.allocated; i++) {
            counter++;
        }
    }
    return counter;
}

int get_total_number_values(struct statsfs_source *src)
{
    struct statsfs_value_source *entry;
    struct statsfs_source *sub_entry;
    int counter = 0;
    // printf("Given src %s\n", src->name);

    list_for_each_entry(entry, &src->simple_values_head, list_element) {
        for (int i = 0; i < entry->values.allocated; i++) {
            counter++;
        }
    }


    list_for_each_entry(sub_entry, &src->subordinates_head, list_element) {
        counter += get_total_number_values(sub_entry);
    }


    return counter;
}

int get_number_aggregates(struct statsfs_source *src)
{
    int counter = 0;
    for (int i = 0; i < src->aggr_values.allocated; i++) {
            counter++;
    }
    return counter;
}
