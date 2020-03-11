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

#define SET_AGG_CASE_TYPE(case_enum, value, ret)       \
                case case_enum:                        \
                    *ret = (uint64_t) value;           \
                    break;                             \
                case case_enum | STATSFS_SIGN:         \
                    *ret = (int64_t) value;            \
                    break;


static int is_val_signed(struct statsfs_value *val)
{
    return val->type & STATSFS_SIGN;
}

static inline int compare_names(struct statsfs_value *v1, char *name)
{
    if (strcmp(v1->name, name) == 0) {
        return 1;
    }
    return 0;
}


static inline int compare_refs(struct statsfs_value *v1, struct statsfs_value *el)
{
    if (el == v1) {
        assert(strcmp(v1->name, el->name) == 0);
        return 1;
    }
    return 0;
}


// static struct statsfs_value *_search_name_in_list(struct list_head* src,
//                                                   char *arg)
// {
//     struct statsfs_value *entry;
//     list_for_each_entry(entry, src, list_element) {
//         if (compare_names(entry, arg)) {
//             return entry;
//         }
//     }
//     return NULL;
// }


// static struct statsfs_value *_search_value_in_list(struct list_head* src,
//                                         struct statsfs_value *arg)
// {
//     struct statsfs_value *entry;
//     list_for_each_entry(entry, src, list_element) {
//         if (compare_refs(entry, arg)) {
//             return entry;
//         }
//     }
//     return NULL;
// }


static struct statsfs_value *_search_name_in_arr(
                                        struct statsfs_value_array* src,
                                        char *arg)
{
    struct statsfs_value *entry;
    for (int i=0; i < src->allocated; i++) {
        entry = src->elements[i];
        if (compare_names(entry, arg)) {
            return entry;
        }
    }
    return NULL;
}


static struct statsfs_value *_search_value_in_arr(
                                        struct statsfs_value_array* src,
                                        struct statsfs_value *arg)
{
    struct statsfs_value *entry;
    for (int i=0; i < src->allocated; i++) {
        entry = src->elements[i];
        if (compare_refs(entry, arg)) {
            return entry;
        }
    }
    return NULL;
}

static struct statsfs_value *search_simple_value(struct statsfs_source* src,
                                    struct statsfs_value *arg,
                                    struct statsfs_value_source **val_src)
{
    struct statsfs_value *entry;
    struct statsfs_value_source *src_entry;

    list_for_each_entry(src_entry, &src->simple_values_head, list_element) {
        entry = _search_value_in_arr(&src_entry->values, arg);
        if (entry) {
            if (val_src) {
                *val_src = src_entry;
            }
            return entry;
        }
    }

    return NULL;
}


static uint64_t get_correct_value(void *address, enum stat_type type)
{
    uint64_t value_found;

    switch (type) {
        case STATSFS_U8:
            value_found = *((uint8_t *) address);
            break;
        case STATSFS_U8 | STATSFS_SIGN:
            value_found = *((int8_t *) address);
            break;
        case STATSFS_U16:
            value_found = *((uint16_t *) address);
            break;
        case STATSFS_U16 | STATSFS_SIGN:
            value_found = *((int16_t *) address);
            break;
        case STATSFS_U32:
            value_found = *((uint32_t *) address);
            break;
        case STATSFS_U32 | STATSFS_SIGN:
            value_found = *((int32_t *) address);
            break;
        case STATSFS_U64:
            value_found = *((uint64_t *) address);
            break;
        case STATSFS_U64 | STATSFS_SIGN:
            value_found = *((int64_t *) address);
            break;
        case STATSFS_BOOL:
            value_found = *((uint8_t *) address);
            break;
        case STATSFS_BOOL | STATSFS_SIGN:
            value_found = *((int8_t *) address);
            break;
        default:
            value_found = 0;
            break;
    }

    return value_found;
}


static void search_all_simple_values(struct statsfs_source* src,
                                struct statsfs_aggregate_value *agg,
                                struct statsfs_value *val)
{
    struct statsfs_value *entry;
    struct statsfs_value_source *src_entry;
    uint64_t value_found;
    void *address;
    list_for_each_entry(src_entry, &src->simple_values_head, list_element) {
        entry = _search_name_in_arr(&src_entry->values, val->name);
        if (entry) { // found
            address = src_entry->base_addr + entry->offset;
            value_found = get_correct_value(address, val->type);
            agg->sum += value_found;
            agg->count++;
            agg->count_zero += (value_found == 0);

            if(is_val_signed(val)){
                agg->max = (((int64_t) value_found) >= ((int64_t) agg->max)) ?
                             value_found : agg->max;
                agg->min = (((int64_t) value_found) <= ((int64_t) agg->min)) ?
                             value_found : agg->min;
            } else {
                agg->max = (value_found >= agg->max) ? value_found : agg->max;
                agg->min = (value_found <= agg->min) ? value_found : agg->min;
            }
        }
    }
}


static void do_recursive_aggregation(struct statsfs_source *root,
                                struct statsfs_value *val,
                                struct statsfs_aggregate_value *agg)
{
    struct statsfs_source *subordinate;

    // search all simple values in this folder
    search_all_simple_values(root, agg, val);

    // recursively search in all subfolders
    list_for_each_entry(subordinate, &root->subordinates_head,
                            list_element) {
        do_recursive_aggregation(subordinate, val, agg);
    }
}


static void init_aggregate_value(struct statsfs_aggregate_value *agg,
                                struct statsfs_value *val)
{
    agg->count = agg->count_zero = agg->sum = 0;
    if (is_val_signed(val)){
        agg->max =  INT64_MIN;
        agg->min =  INT64_MAX;
    } else {
        agg->max = 0;
        agg->min = UINT64_MAX;
    }
}


static void set_final_value(struct statsfs_aggregate_value *agg,
                            struct statsfs_value *val,
                            uint64_t *ret)
{
    int operation = val->aggr_kind | is_val_signed(val);
    uint64_t value = 0;


    switch (operation) {
        case STATSFS_AVG:
            *ret = (uint64_t) agg->count ? agg->sum / agg->count : 0;
            break;
        case STATSFS_AVG | STATSFS_SIGN:
            *ret = (int64_t) agg->count ? ((int64_t) agg->sum) / agg->count : 0;
            break;
        SET_AGG_CASE_TYPE(STATSFS_SUM, agg->sum, ret)
        SET_AGG_CASE_TYPE(STATSFS_MIN, agg->min, ret)
        SET_AGG_CASE_TYPE(STATSFS_MAX, agg->max, ret)
        SET_AGG_CASE_TYPE(STATSFS_COUNT_ZERO, agg->count_zero, ret)
    default:
        break;
    }
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
    for (int i=0; i < parent->aggr_values.allocated; i++) {
        entry = parent->aggr_values.elements[i];
        printf("Value:\nname:%s\toff:%d\ttype:%d\taggr:%d\n", entry->name, entry->offset, entry->type, entry->aggr_kind);
    }

    printf("#####NORMAL#####\n");
    list_for_each_entry(src_entry, &parent->simple_values_head, list_element) {
        for (int i=0; i < src_entry->values.allocated; i++) {
            entry = src_entry->values.elements[i];
            printf("Value:\nname:%s\toff:%d\ttype:%d\taggr:%d\n", entry->name, entry->offset, entry->type, entry->aggr_kind);
        }
    }
}


struct statsfs_value_source *create_value_source(void *base)
{
    struct statsfs_value_source *val_src;

    val_src = malloc(sizeof(struct statsfs_value_source));
    if (!val_src) {
        printf("Error in making the value source!\n");
        return NULL;
    }

    val_src->base_addr = base;
    init_statsfs_value_array(&val_src->values);
    val_src->list_element = (struct list_head)
                                    LIST_HEAD_INIT(val_src->list_element);

    return val_src;
}


struct statsfs_value_source *search_value_source_by_base(
                                    struct statsfs_source* src, void *base)
{
    struct statsfs_value_source *entry;
    list_for_each_entry(entry, &src->simple_values_head, list_element) {
        if (entry->base_addr == base) {
            return entry;
        }
    }
    return NULL;
}


struct statsfs_value *search_in_value_source_by_name(
                                struct statsfs_value_source * src,
                                char *val)
{
    return _search_name_in_arr(&src->values, val);
}


struct statsfs_value *search_in_value_source_by_val(
                                struct statsfs_value_source * src,
                                struct statsfs_value * val)
{
    return _search_value_in_arr(&src->values, val);
}


struct statsfs_value *search_aggr_in_source_by_name
                                        (struct statsfs_source *src,
                                        char *name)
{
    return _search_name_in_arr(&src->aggr_values, name);
}

struct statsfs_value *search_aggr_in_source_by_val
                                        (struct statsfs_source *src,
                                        struct statsfs_value *val)
{
    return _search_value_in_arr(&src->aggr_values, val);
}


struct statsfs_value *search_in_source_by_name(struct statsfs_source* src,
                                                char *name)
{
    struct statsfs_value *entry;
    struct statsfs_value_source *src_entry;

    list_for_each_entry(src_entry, &src->simple_values_head, list_element) {
        entry = _search_name_in_arr(&src_entry->values, name);
        if (entry) {
            return entry;
        }
    }

    return NULL;
}


int search_in_source_by_value(struct statsfs_source *source,
                            struct statsfs_value *arg,
                            uint64_t *ret)
{
    struct statsfs_value_source *entry;
    struct statsfs_value *found;
    struct statsfs_aggregate_value aggr;
    void *address;
    assert(ret != NULL);
    assert(source != NULL);

    if (!arg) {
        return -ENOENT;
    }

     // look in simple values
    found = search_simple_value(source, arg, &entry);
    if (found) {
        address = entry->base_addr + found->offset;
        *ret = get_correct_value(address, found->type);
        return 0;
    }

    // look in aggregates
    found = search_aggr_in_source_by_val(source, arg);
    if (found) {
        assert(found->aggr_kind != STATSFS_NONE);
        init_aggregate_value(&aggr, found);
        do_recursive_aggregation(source, found, &aggr);
        set_final_value(&aggr, found, ret);
        return 0;
    }

    printf("ERROR: Value in source \"%s\" not found!\n", source->name);
    return -ENOENT;
}


void init_statsfs_value_array(struct statsfs_value_array *arr)
{
    arr->allocated = 0;
    memset(arr->elements, 0, STATSFS_MAX_VALUES * sizeof(struct statsfs_value *));
}

void add_statsfs_value_array(struct statsfs_value_array *arr,
                             struct statsfs_value *val)
{
    arr->elements[arr->allocated++] = val;
}

