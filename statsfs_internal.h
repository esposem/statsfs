#ifndef _STATSFS_INTERNAL_H
#define _STATSFS_INTERNAL_H

#include "list.h"
#include "statsfs.h"

#define STATSFS_MAX_VALUES 100
typedef int kref_t; //TODO: remove in kernel

struct statsfs_aggregate_value {
    uint64_t sum, min, max;
    uint32_t count, count_zero;
};

struct statsfs_value_array {
    struct statsfs_value *elements[STATSFS_MAX_VALUES];
    uint16_t allocated;
};

struct statsfs_value_source {
    void *base_addr;
    // all values with this base
    struct statsfs_value_array values;
    struct list_head list_element;
};

struct statsfs_source {
    kref_t refcount;
    char *name;         /* dir name */

    /* list of struct statsfs_value_source */
    struct list_head simple_values_head; // values, grouped by base
    // aggregation, no need of base
    struct statsfs_value_array aggr_values;

    /* list of struct statsfs subordinate */
    struct list_head subordinates_head;

    struct list_head list_element;

    /* mutex o spinlock */
};


void statsfs_subordinates_debug_list(struct statsfs_source *parent);
void statsfs_values_debug_list(struct statsfs_source *parent);

/* statsfs_value_source related functions */
struct statsfs_value_source *create_value_source(void *base);

struct statsfs_value_source *search_value_source_by_base(
                                    struct statsfs_source* src, void *base);

struct statsfs_value *search_in_value_source_by_name(
                                struct statsfs_value_source * src, char *name);

struct statsfs_value *search_in_value_source_by_val(
                                struct statsfs_value_source * src,
                                struct statsfs_value * val);


/* search_in_source related functions */
struct statsfs_value *search_aggr_in_source_by_name
                                        (struct statsfs_source *src,
                                        char *arg);

struct statsfs_value *search_aggr_in_source_by_val
                                        (struct statsfs_source *src,
                                        struct statsfs_value *val);

struct statsfs_value *search_in_source_by_name(struct statsfs_source* src, char *name);

int search_in_source_by_value(struct statsfs_source *source,
                            struct statsfs_value *arg,
                            uint64_t *ret);

/* statsfs_value_array related functions */
void init_statsfs_value_array(struct statsfs_value_array *arr);
void add_statsfs_value_array(struct statsfs_value_array *arr,
                             struct statsfs_value *val);
#endif