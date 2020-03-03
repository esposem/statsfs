#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>
#include <errno.h>
#include "list.h"
#include "statsfs.h"

typedef int kref_t; //TODO: remove in kernel

enum stat_type{
    U8 = 0,
    U16,
    U32,
    U64, // Max number
    BOOL,
    STR,
};

static int is_number(enum stat_type type)
{
    return type <= U64;
}

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


static struct statsfs_value_source *search_value_source_by_base(
                                    struct statsfs_source* src, void *base)
{
    struct statsfs_value_source *entry;
    list_for_each_entry(entry, &src->simple_values_head, list_element) {
        if(entry->base_addr == base)
            return entry;
    }
    return NULL;
}

static int compare_names(struct statsfs_value *v1, void *v2)
{
    char *name = (char *) v2;
    if(strcmp(v1->name, name) == 0){
        return 1;
    }
    return 0;
}

static int compare_refs(struct statsfs_value *v1, void *v2)
{
    struct statsfs_value *el = (struct statsfs_value *) v2;
    if(el == v1){
        assert(strcmp(v1->name, el->name) == 0);
        return 1;
    }
    return 0;
}

static struct statsfs_value *_search_value_in_list(
                                        struct list_head* src,
                                        compare_f compare,
                                        void *arg)
{
    struct statsfs_value *entry;
    list_for_each_entry(entry, src, list_element) {
        if(compare(entry, arg)) {
            return entry;
        }
    }
    return NULL;
}


static struct statsfs_value *search_aggr_value(
                                    struct statsfs_source* src,
                                    compare_f compare,
                                    void *arg)
{
    return _search_value_in_list(&src->aggr_values_head, compare, arg);
}



static struct statsfs_value *search_simple_value(
                                    struct statsfs_source* src,
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


static void search_all_simple_values(struct statsfs_source* src,
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


static struct statsfs_value_source *create_value_source(void *base)
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

    printf("#####NORLMAL#####\n");
    list_for_each_entry(src_entry, &parent->simple_values_head, list_element) {
        list_for_each_entry(entry, &src_entry->values_head, list_element) {
            printf("Value:\nname:%s\toff:%d\ttype:%d\taggr:%d\n", entry->name, entry->offset, entry->type, entry->aggr_kind);
        }
    }
}


struct statsfs_source *statsfs_source_create(const char *fmt, ...)
{
    va_list     ap;
    char    buf[100];
    struct statsfs_source *ret;

    va_start(ap, fmt);
    int char_needed = vsnprintf(buf, 100, fmt, ap);
    va_end(ap);

    ret = malloc(sizeof(struct statsfs_source));
    if(!ret) {
        printf("Error in creating statsfs_source!\n");
        return NULL;
    }

    ret->name = malloc(char_needed + 1);
    if(!ret) {
        printf("Error in creating statsfs_source name!\n");
        return NULL;
    }
    memcpy(ret->name,buf, char_needed);
    ret->name[char_needed] = '\0';
    printf("Name is %s\n", ret->name);

    ret->refcount = 0;
    ret->simple_values_head = (struct list_head)
                            LIST_HEAD_INIT(ret->simple_values_head);
    ret->aggr_values_head = (struct list_head)
                            LIST_HEAD_INIT(ret->aggr_values_head);
    ret->subordinates_head = (struct list_head)
                            LIST_HEAD_INIT(ret->subordinates_head);
    ret->list_element = (struct list_head) LIST_HEAD_INIT(ret->list_element);

    return ret;
}


void statsfs_source_destroy(struct statsfs_source *src)
{
    struct list_head *it, *safe;
    struct statsfs_value *val_entry;
    struct statsfs_source *src_entry;
    assert(src != NULL);
    assert(src->refcount == 0);
    free(src->name);
    list_del(&src->list_element); // deletes it from the list he's in

    // iterate through the values and delete them
    list_for_each_safe(it, safe, &src->simple_values_head) {
        val_entry = list_entry(it, struct statsfs_value, list_element);
        list_del_init(&val_entry->list_element);
    }

    list_for_each_safe(it, safe, &src->aggr_values_head) {
        val_entry = list_entry(it, struct statsfs_value, list_element);
        list_del_init(&val_entry->list_element);
    }

    // iterate through the subordinates and delete them
    list_for_each_safe(it, safe, &src->subordinates_head) {
        src_entry = list_entry(it, struct statsfs_source, list_element);
        statsfs_source_destroy(src_entry);
    }

    free(src);
}


void statsfs_source_add_values(struct statsfs_source *source,
                   const struct statsfs_value *stat, void *ptr)
{
    assert(source != NULL);
    assert(ptr != NULL);

    struct statsfs_value_source *val_src;

    val_src = search_value_source_by_base(source, ptr);
    if(!val_src) {
        val_src = create_value_source(ptr);
        // add the val_src to the source list
        list_add(&val_src->list_element, &source->simple_values_head);
    }

    struct statsfs_value *p;
    for(p = (struct statsfs_value *) stat; p->name; p++) {
        if(p->aggr_kind == 0) {
            // add the val to the val_src list
            list_add(&p->list_element, &val_src->values_head);
        }
    }
}


void statsfs_source_add_aggregate(struct statsfs_source *source,
                                const struct statsfs_value *stat)
{
    assert(source != NULL);
    struct statsfs_value *p;
    for(p = (struct statsfs_value *) stat; p->name; p++) {
        if(p->aggr_kind != 0) {
            list_add(&p->list_element, &source->aggr_values_head);
        }
    }

}

void statsfs_source_add_subordinate(
                struct statsfs_source *source,
                struct statsfs_source *sub)
{
    assert(source != NULL);
    assert(sub != NULL);
    list_add(&sub->list_element, &source->subordinates_head);
}


// Does not destroy the subordinate!
void statsfs_source_remove_subordinate(
                struct statsfs_source *source,
                struct statsfs_source *sub)
{
    struct list_head *it, *safe;
    struct statsfs_source *src_entry;

    assert(source != NULL);
    assert(sub != NULL);

    list_for_each_safe(it, safe, &source->subordinates_head) {
        src_entry = list_entry(it, struct statsfs_source, list_element);
        if(src_entry == sub){
            assert(strcmp(src_entry->name, sub->name) == 0);
            list_del_init(&src_entry->list_element);
            return;
        }
    }

    printf("WARINING: Subordinate %s not found in %s\n", sub->name, source->name);
}


static void do_recursive_aggregation(struct statsfs_source *root,
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


static void init_ret_on_aggr(void **ret, enum stat_type type,
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

int _statsfs_source_get_value( struct statsfs_source *source,
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

int statsfs_source_get_value_by_val(
                struct statsfs_source *source,
                struct statsfs_value *val, void **ret)
{
    return _statsfs_source_get_value(source, compare_refs, val, ret);
}

int statsfs_source_get_value_by_name(
                struct statsfs_source *source,
                const char *name, void **ret)
{
    return _statsfs_source_get_value(source, compare_names,
                                    (char *) name, ret);
}