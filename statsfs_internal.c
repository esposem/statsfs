#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>
#include <errno.h>
#include "list.h"
#include "statsfs_internal.h"
#include "statsfs.h"

/* Helpers */

static int is_number(enum stat_type type)
{
    return type <= U64;
}


static struct statsfs_value *_search_value_in_list(struct list_head* src,
                                        compare_f compare, void *arg)
{
    struct statsfs_value *entry;
    list_for_each_entry(entry, src, list_element) {
        if(compare(entry, arg)) {
            return entry;
        }
    }
    return NULL;
}


static void aggregate_sum(void *entry, uint64_t *ret)
{
    *ret += *((uint64_t *) entry);
}


static void aggregate_max(void *entry, uint64_t *ret)
{
    uint64_t value = *((uint64_t *) entry);
    if(value >= *ret) {
        *ret = value;
    }
}


static void aggregate_min(void *entry, uint64_t *ret)
{
    uint64_t value = *((uint64_t *) entry);
    if(value <= *ret) {
        *ret = value;
    }
}


static void aggregate_count_zero(void *entry, uint64_t *ret)
{
    // read as uint64 even though might be a boolean
    uint64_t value = *((uint64_t *) entry);
    if(value == 0) {
        (*ret)++;
    }
}


static void aggregate_avg(void *entry, uint64_t *ret)
{
    uint64_t *count = &ret[1];
    aggregate_sum(entry, ret);
    (*count)++;
}


void statsfs_subordinates_debug_list(struct statsfs_source *parent)
{
    struct statsfs_source *entry;
    printf("#####SUBFOLDERS#####\n");
    printf("Source %s\n", parent->name);
    list_for_each_entry(entry, &parent->subordinates_head, list_element) {
        printf("Dir:\tname:%s\trefc:%d\n", entry->name, entry->refcount);
    }
}


void statsfs_values_debug_list(struct statsfs_source *parent)
{
    struct statsfs_value *entry;
    struct statsfs_value_source *src_entry;
    printf("#####AGGREGATES#####\n");
    list_for_each_entry(entry, &parent->aggr_values_head, list_element) {
        printf("Value:\nname:%s\toff:%d\ttype:%d\taggr:%d\n", entry->name, entry->offset, entry->type, entry->aggr_kind);
    }

    printf("#####NORMAL#####\n");
    list_for_each_entry(src_entry, &parent->simple_values_head, list_element) {
        list_for_each_entry(entry, &src_entry->values_head, list_element) {
            printf("Value:\nname:%s\toff:%d\ttype:%d\taggr:%d\n", entry->name, entry->offset, entry->type, entry->aggr_kind);
        }
    }
}


int compare_names(struct statsfs_value *v1, void *v2)
{
    char *name = (char *) v2;
    if(strcmp(v1->name, name) == 0){
        return 1;
    }
    return 0;
}


int compare_refs(struct statsfs_value *v1, void *v2)
{
    struct statsfs_value *el = (struct statsfs_value *) v2;
    if(el == v1){
        assert(strcmp(v1->name, el->name) == 0);
        return 1;
    }
    return 0;
}


struct statsfs_value_source *search_value_source_by_base(
                                    struct statsfs_source* src, void *base)
{
    struct statsfs_value_source *entry;
    list_for_each_entry(entry, &src->simple_values_head, list_element) {
        if(entry->base_addr == base) {
            return entry;
        }
    }
    return NULL;
}

struct statsfs_value *search_in_value_source_by_name(
                                struct statsfs_value_source * src, char *name)
{
    return _search_value_in_list(&src->values_head, compare_names, name);
}


struct statsfs_value *search_in_value_source_by_val(
                                struct statsfs_value_source * src,
                                struct statsfs_value * val)
{
    return _search_value_in_list(&src->values_head, compare_refs, val);
}


struct statsfs_value *search_simple_value( struct statsfs_source* src,
                                    compare_f compare, void *arg,
                                    struct statsfs_value_source **val_src)
{
    struct statsfs_value *entry;
    struct statsfs_value_source *src_entry;

    list_for_each_entry(src_entry, &src->simple_values_head, list_element) {
        entry = _search_value_in_list(&src_entry->values_head, compare, arg);
        if(entry){
            if(val_src){
                *val_src = src_entry;
            }
            return entry;
        }
    }

    return NULL;
}


void search_all_simple_values(struct statsfs_source* src,
                                aggregate_f aggregate,
                                uint64_t *aggregate_counter,
                                char *name)
{
    struct statsfs_value *entry;
    struct statsfs_value_source *src_entry;

    list_for_each_entry(src_entry, &src->simple_values_head, list_element) {
        entry = _search_value_in_list(&src_entry->values_head, compare_names, name);
        if(entry){ // found
            void *ptr = src_entry->base_addr + entry->offset;
            aggregate(ptr, aggregate_counter);
        }
    }
}


struct statsfs_value_source *create_value_source(void *base)
{
    struct statsfs_value_source *val_src;

    val_src = malloc(sizeof(struct statsfs_value_source));
    if(!val_src){
        printf("Error in making the value source!\n");
        return NULL;
    }

    val_src->base_addr = base;
    val_src->values_head = (struct list_head)
                                    LIST_HEAD_INIT(val_src->values_head);
    val_src->list_element = (struct list_head)
                                    LIST_HEAD_INIT(val_src->list_element);

    return val_src;
}


int statsfs_source_get_value(struct statsfs_source *source,
                            compare_f compare, void *arg,
                            uint64_t *ret)
{
    struct statsfs_value_source *entry;
    struct statsfs_value *found;
    uint64_t ret_values[2];


    // look in aggregates
    found = _search_value_in_list(&source->aggr_values_head, compare, arg);
    if(found){
        // if(is_number(found->type) || found->aggr_kind == COUNT_ZERO) {
            init_ret_on_aggr(ret_values, found->type, found->aggr_kind);

            do_recursive_aggregation(source, found->aggr_kind,
                                    found->name, ret_values);

            if(found->aggr_kind == AVG){
                *ret = ret_values[0] / ret_values[1];
            } else {
                *ret = ret_values[0];
            }
            return 0;
        // }
        printf("No operations available for non-number type %d in \
                %s\n", found->type, found->name);
        return -ENOENT;
    }else{
        printf("Not found in aggregates\n");
    }

    // look in simple values
    found = search_simple_value(source, compare, arg, &entry);
    if(found) {
        *ret = *((uint64_t *) (entry->base_addr + found->offset));
        return 0;
    }else{
        printf("Not found in values\n");
    }

    printf("ERROR: Value in source %s not found!\n", source->name);
    return -ENOENT;
}


void do_recursive_aggregation(struct statsfs_source *root,
                                enum stat_aggr aggr_type, char *name,
                                uint64_t *ret)
{
    assert(aggr_type != NONE);
    aggregate_f function = NULL;

    switch (aggr_type) {
        case SUM:
            function = aggregate_sum;
            break;
        case MIN:
            function = aggregate_min;
            break;
        case MAX:
            function = aggregate_max;
            break;
        case AVG:
            function = aggregate_avg;
            break;
        case COUNT_ZERO:
            function = aggregate_count_zero;
            break;
        default:
            printf("Unrecognized aggr_type %d\n", aggr_type);
            break;
    }

    if(function) {
        search_all_simple_values(root, function, ret, name);
        struct statsfs_source *subordinate;

        list_for_each_entry(subordinate, &root->subordinates_head,
                             list_element) {
            do_recursive_aggregation(subordinate, aggr_type, name, ret);
        }
    }
}


void init_ret_on_aggr(uint64_t*ret, enum stat_type type,
                        enum stat_aggr aggr)
{
    switch (aggr) {
        case AVG:
            ret[1] = 0;
        case MAX:
        case SUM:
        case COUNT_ZERO:
            *ret = 0;
            break;
        case MIN: {
            *ret = UINT64_MAX;
            break;
        }
        default:
            break;
    }
}
