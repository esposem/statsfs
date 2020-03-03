#ifndef _STATSFS_INTERNAL_H
#define _STATSFS_INTERNAL_H

#include "list.h"

typedef int kref_t; //TODO: remove in kernel

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
    enum stat_type type;	/* STAT_TYPE_{BOOL,U64,...} */
    /* Bitmask with zero or more of STAT_AGGR_{MIN,MAX,SUM,...} */
    enum stat_aggr aggr_kind;
    uint16_t mode;		        /* File mode */
    /* Offset from base address to field containing the value */
    int offset;

    struct list_head list_element;
};

struct statsfs_value_source {
    void *base_addr;
    struct list_head values_head;
    struct list_head list_element;
};

struct statsfs_source {
    kref_t refcount;
    char *name;         /* dir name */

    /* list of struct statsfs_value_source */
    struct list_head simple_values_head; // values, grouped by base
    struct list_head aggr_values_head; // aggregation, no need of base

    /* list of struct statsfs subordinate */
    struct list_head subordinates_head;

    struct list_head list_element;

    /* mutex o spinlock */
};

typedef int (*compare_f) (struct statsfs_value *v1, void *v2);
typedef void (*aggregate_f) (void *src, enum stat_type type, void **counter);


void statsfs_subordinates_debug_list(struct statsfs_source *parent);
void statsfs_values_debug_list(struct statsfs_source *parent);

struct statsfs_value_source *search_value_source_by_base(
                                    struct statsfs_source* src, void *base);

int compare_names(struct statsfs_value *v1, void *v2);
int compare_refs(struct statsfs_value *v1, void *v2);

struct statsfs_value *search_aggr_value(struct statsfs_source* src,
                                        compare_f compare, void *arg);
struct statsfs_value *search_simple_value( struct statsfs_source* src,
                                    compare_f compare, void *arg,
                                    struct statsfs_value_source **val_src);
void search_all_simple_values(struct statsfs_source* src,
                                aggregate_f aggregate,
                                void **aggregate_counter,
                                char *name);

struct statsfs_value_source *create_value_source(void *base);

int statsfs_source_get_value(struct statsfs_source *source,
                            compare_f compare, void *arg,
                            void **ret);

void do_recursive_aggregation(struct statsfs_source *root,
                                enum stat_aggr aggr_type, char *name,
                                void **ret);

void init_ret_on_aggr(void **ret, enum stat_type type,
                        enum stat_aggr aggr);






#endif