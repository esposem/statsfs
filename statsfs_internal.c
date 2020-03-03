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


static void aggregate_sum(void *entry, enum stat_type type, void **ret)
{
    switch (type) {
        case U64: {
            uint64_t **counter = (uint64_t **) ret;
            uint64_t value = *((uint64_t *) entry);
            *(*counter) += value;
            break;
        }
        default:
            printf("SUM for stat type %d not implemented!\n", type);
            break;
    }
}

static void aggregate_max(void *entry, enum stat_type type, void **ret)
{
    switch (type) {
        case U64: {
            uint64_t **counter = (uint64_t **) ret;
            uint64_t value = *((uint64_t *) entry);
            if(*(*counter) < value) {
                *(*counter) = value;
            }
            break;
        }
        default:
            printf("SUM for stat type %d not implemented!\n", type);
            break;
    }
}


static void aggregate_min(void *entry, enum stat_type type, void **ret)
{
    switch (type) {
    case U64: {
        uint64_t **counter = (uint64_t **) ret;
        uint64_t value = *((uint64_t *) entry);
        if(*(*counter) > value) {
            *(*counter) = value;
        }
        break;
    }
    default:
        printf("SUM for stat type %d not implemented!\n", type);
        break;
    }
}


static void aggregate_avg(void *entry, enum stat_type type, void **ret)
{
    switch (type) {
        case U64: {
            uint64_t **avg_arr = (uint64_t **) ret;
            uint64_t *value = (uint64_t *) entry;
            avg_arr[0] += *value;
            avg_arr[1]++;
            break;
        }
        default:
            printf("SUM for stat type %d not implemented!\n", type);
            break;
    }
}

/* Exported functions */

void statsfs_subordinates_debug_list(struct statsfs_source *parent)
{
    struct statsfs_source *entry;
    list_for_each_entry(entry, &parent->subordinates_head, list_element) {
        printf("Dir:\nname:%s\refc:%d\n", entry->name, entry->refcount);
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
        if(entry->base_addr == base)
            return entry;
    }
    return NULL;
}


struct statsfs_value *search_aggr_value(struct statsfs_source* src,
                                        compare_f compare, void *arg)
{
    return _search_value_in_list(&src->aggr_values_head, compare, arg);
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
                                void **aggregate_counter,
                                char *name)
{
    struct statsfs_value *entry;
    struct statsfs_value_source *src_entry;

    list_for_each_entry(src_entry, &src->simple_values_head, list_element) {
        entry = _search_value_in_list(&src_entry->values_head, compare_names, name);
        if(entry){ // found
            void *ptr = src_entry->base_addr + entry->offset;
            aggregate(ptr, entry->type, aggregate_counter);
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


int statsfs_source_get_value( struct statsfs_source *source,
                            compare_f compare, void *arg,
                            void **ret)
{
    struct statsfs_value_source *entry;
    struct statsfs_value *found;
    // look in simple values
    found = search_simple_value(source, compare, arg, &entry);
    if(found) {
        *ret = entry->base_addr + found->offset;
        return 0;
    }
    // look in aggregates
    found = search_aggr_value(source, compare, arg);
    if(found && is_number(found->type)){ // TODO: work only on numbers?
        init_ret_on_aggr(ret, found->type, found->aggr_kind);

        do_recursive_aggregation(source, found->aggr_kind,
                                (char *) found->name, ret);
        return 0;
    }
    printf("Value in source %s not found!\n", source->name);
    return -ENOENT;
}


void do_recursive_aggregation(struct statsfs_source *root,
                                enum stat_aggr aggr_type, char *name,
                                void **ret)
{
    assert(aggr_type != NONE);

    switch (aggr_type) {
    case SUM:
        search_all_simple_values(root, aggregate_sum, ret, name);
        break;
    case MIN:
        search_all_simple_values(root, aggregate_min, ret, name);
        break;
    case MAX:
        search_all_simple_values(root, aggregate_max, ret, name);
        break;
    case AVG:
        search_all_simple_values(root, aggregate_avg, ret, name);
        break;
    default:
        printf("Unrecognized aggr_type %d\n", aggr_type);
        break;
    }
}


void init_ret_on_aggr(void **ret, enum stat_type type,
                        enum stat_aggr aggr)
{
    switch (aggr) {
    case SUM: {
        switch (type) {
        case U64: {
            uint64_t **val = (uint64_t **) ret;
            *(*val) = 0;
            break;
        }
        default:
            break;
        }
        break;
    }
    case MAX: {
        switch (type) {
        case U64: {
            uint64_t **val = (uint64_t **) ret;
            *(*val) = 0;
            break;
        }
        default:
            break;
        }
        break;
    }
    case MIN: {
        switch (type) {
        case U64: {
            uint64_t **val = (uint64_t **) ret;
            *(*val) = UINT64_MAX;
            break;
        }
        default:
            break;
        }
        break;
    }
    case AVG: {
        switch (type) {
        case U64: {
            uint64_t **val = (uint64_t **) ret;
            (*val)[0] = 0;
            (*val)[1] = 0;
            break;
        }
        default:
            break;
        }
        break;
    }
    default:
        break;
    }
}
