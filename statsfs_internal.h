#ifndef _STATSFS_INTERNAL_H
#define _STATSFS_INTERNAL_H

#include "list.h"
#include "statsfs.h"
#include <assert.h>


typedef int kref_t; //TODO: remove in kernel
#define WARN_ON(err) assert(err)
#define BUG_ON(err) assert(err)

struct statsfs_aggregate_value {
    uint64_t sum, min, max;
    uint32_t count, count_zero;
};

struct statsfs_value_source {
    void *base_addr;
    // all values with this base
    struct statsfs_value *values;
    struct list_head list_element;
};

struct statsfs_source {
    kref_t refcount;
    char *name;         /* dir name */

    /* list of struct statsfs_value_source */
    struct list_head values_head; // values, grouped by base

    /* list of struct statsfs subordinate */
    struct list_head subordinates_head;

    struct list_head list_element;

    /* mutex o spinlock */
};


#endif