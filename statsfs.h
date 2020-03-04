#ifndef _STATSFS_H
#define _STATSFS_H

#include "list.h"

struct statsfs_source;

enum stat_type{
    U8 = 0,
    U16,
    U32,
    U64, // Max number
    BOOL,
    STR,
};

enum stat_aggr{
    NONE = 0,
    SUM,
    MIN,
    MAX,
    AVG, // TODO: for now, avg needs arr[2] that has sum[0] and count[1]
};

struct statsfs_value {
    const char *name;
    /* Offset from base address to field containing the value */
    int offset;

    enum stat_type type;	/* STAT_TYPE_{BOOL,U64,...} */
    /* Bitmask with zero or more of STAT_AGGR_{MIN,MAX,SUM,...} */
    enum stat_aggr aggr_kind;
    uint16_t mode;		        /* File mode */

    struct list_head list_element;

};


struct statsfs_source *statsfs_source_create(const char *fmt, ...);
void statsfs_source_destroy(struct statsfs_source *src);

// returns how many values have been added
int statsfs_source_add_values(struct statsfs_source *source,
                                const struct statsfs_value *stat,void *ptr);
                                
// returns how many aggregates have been added
int statsfs_source_add_aggregate(struct statsfs_source *source,
                                    const struct statsfs_value *stat);

void statsfs_source_add_subordinate(struct statsfs_source *source,
                                    struct statsfs_source *sub);

// Does not destroys the subordinate!
void statsfs_source_remove_subordinate(struct statsfs_source *source,
                                        struct statsfs_source *sub);


int statsfs_source_get_value_by_val(struct statsfs_source *source,
                                    struct statsfs_value *val, void **ret);


int statsfs_source_get_value_by_name(struct statsfs_source *source,
                                        const char *name, void **val);

void statsfs_source_register(struct statsfs_source *source);

// vedi kref_get e kref_put in include/linux/kref.h
void statsfs_source_get(struct statsfs_source *);
void statsfs_source_put(struct statsfs_source *);


#endif