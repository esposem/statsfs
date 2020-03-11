#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>
#include "statsfs.h"
#include "statsfs_internal.h"
#include "test_helpers.h"

static void get_stats_at_addr(struct statsfs_source *src, void *addr, int *aggr,
                        int *val)
{
    struct statsfs_value *entry;
    struct statsfs_value_source *src_entry;
    int counter_val =0, counter_aggr = 0;

    list_for_each_entry(src_entry, &src->values_head, list_element) {
        if(addr && src_entry->base_addr != addr){
            continue;
        }

        for (entry = src_entry->values; entry->name; entry++) {
            if(entry->aggr_kind == STATSFS_NONE){
                counter_val++;
            } else {
                counter_aggr++;
            }
        }
    }

    if (aggr) {
        *aggr = counter_aggr;
    }

    if (val) {
        *val = counter_val;
    }
}

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
    int counter = 0;
    get_stats_at_addr(src, NULL, NULL, &counter);
    return counter;
}

int get_total_number_values(struct statsfs_source *src)
{
    struct statsfs_value_source *src_entry;
    struct statsfs_value *entry;
    struct statsfs_source *sub_entry;
    int counter = 0;
    // printf("Given src %s\n", src->name);

    get_stats_at_addr(src, NULL, NULL, &counter);

    list_for_each_entry(sub_entry, &src->subordinates_head, list_element) {
        counter += get_total_number_values(sub_entry);
    }


    return counter;
}

int get_number_aggregates(struct statsfs_source *src)
{
    int counter = 0;
    get_stats_at_addr(src, NULL, &counter, NULL);
    return counter;
}

int get_number_values_with_base(struct statsfs_source *src, void *addr)
{
    int counter = 0;
    get_stats_at_addr(src, addr, NULL, &counter);
    return counter;
}


int get_number_aggr_with_base(struct statsfs_source *src, void *addr)
{
    int counter = 0;
    get_stats_at_addr(src, addr, &counter, NULL);
    return counter;
}