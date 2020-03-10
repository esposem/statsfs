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

/*
    TODO:
    - kref and mutex
    - files
*/

void statsfs_source_register(struct statsfs_source *source) {
    // TODO: creates file /sys/kernel/statsfs/kvm
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
    if (!ret) {
        printf("Error in creating statsfs_source!\n");
        return NULL;
    }

    ret->name = malloc(char_needed + 1);
    if (!ret) {
        printf("Error in creating statsfs_source name!\n");
        return NULL;
    }
    memcpy(ret->name,buf, char_needed);
    ret->name[char_needed] = '\0';

    ret->refcount = 1; //TODO: fix for kernel
    ret->simple_values_head = (struct list_head)
                            LIST_HEAD_INIT(ret->simple_values_head);

    init_statsfs_value_array(&ret->aggr_values);
    ret->subordinates_head = (struct list_head)
                            LIST_HEAD_INIT(ret->subordinates_head);
    ret->list_element = (struct list_head) LIST_HEAD_INIT(ret->list_element);

    return ret;
}


void statsfs_source_destroy(struct statsfs_source *src)
{
    struct list_head *it, *safe;
    struct statsfs_value_source *val_src_entry;
    struct statsfs_source *src_entry;
    assert(src != NULL);
    // assert(src->refcount == 0); // TODO: kernel
    free(src->name);
    list_del(&src->list_element); // deletes it from the list he's in

    // iterate through the values and delete them
    list_for_each_safe(it, safe, &src->simple_values_head) {
        val_src_entry = list_entry(it, struct statsfs_value_source,
                                    list_element);
        val_src_entry->values.allocated = 0;
        free(val_src_entry);
    }

    src->aggr_values.allocated = 0;

    // iterate through the subordinates and delete them
    list_for_each_safe(it, safe, &src->subordinates_head) {
        src_entry = list_entry(it, struct statsfs_source, list_element);
        statsfs_source_destroy(src_entry);
    }

    free(src);
}


int statsfs_source_add_values(struct statsfs_source *source,
                   const struct statsfs_value *stat, void *ptr)
{
    assert(source != NULL);
    assert(ptr != NULL);

    struct statsfs_value_source *val_src;
    int count = 0;

    val_src = search_value_source_by_base(source, ptr);
    if (!val_src) {
        val_src = create_value_source(ptr);
        // add the val_src to the source list
        list_add(&val_src->list_element, &source->simple_values_head);
    }

    struct statsfs_value *p, *same;
    for(p = (struct statsfs_value *) stat; p->name; p++) {
        same = search_in_value_source_by_name(val_src, p->name);
        if (!same && p->aggr_kind == STATSFS_NONE) {
            // add the val to the val_src list
            add_statsfs_value_array(&val_src->values, p);
            count++;
        }
    }
    // printf("statsfs_source_add_values on %s\n", source->name);
    // statsfs_values_debug_list(source);
    return count;
}


int statsfs_source_add_aggregate(struct statsfs_source *source,
                                const struct statsfs_value *stat)
{
    assert(source != NULL);
    struct statsfs_value *p;
    int count = 0;
    for(p = (struct statsfs_value *) stat; p->name; p++) {
        if (p->aggr_kind != STATSFS_NONE) {
            add_statsfs_value_array(&source->aggr_values, p);
            count++;
        }
    }
    // printf("statsfs_source_add_aggregates on %s\n", source->name);
    // statsfs_values_debug_list(source);

    return count;
}


void statsfs_source_add_subordinate(
                struct statsfs_source *source,
                struct statsfs_source *sub)
{
    assert(source != NULL);
    assert(sub != NULL);
    list_add(&sub->list_element, &source->subordinates_head);
    // printf("statsfs_source_add_subordinate on %s\n", source->name);
    // statsfs_subordinates_debug_list(source);
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
        if (src_entry == sub) {
            assert(strcmp(src_entry->name, sub->name) == 0);
            list_del_init(&src_entry->list_element);
            return;
        }
    }

    printf("WARINING: Subordinate %s not found in %s\n", sub->name, source->name);
}


int statsfs_source_get_value_by_val(
                struct statsfs_source *source,
                struct statsfs_value *val, uint64_t*ret)
{
    return search_in_source_by_value(source, val, ret);
}


int statsfs_source_get_value_by_name(
                struct statsfs_source *source,
                char *name, uint64_t*ret)
{
    struct statsfs_value *val;
    *ret = 0;
    
    // search in my values
    val = search_in_source_by_name(source, name);

    if (!val) {
        // search in my aggregates
        val = search_aggr_in_source_by_name(source, name);
    }

    if (!val) {
        return -ENOENT;
    }

    return statsfs_source_get_value_by_val(source, val, ret);
}

void statsfs_source_get(struct statsfs_source *source)
{
    // TODO: fix for kernel
    source->refcount++;
}


void statsfs_source_put(struct statsfs_source *source)
{
    // TODO: fix for kernel
    if (source->refcount) {
        source->refcount--;
        if (source->refcount == 0) {
            statsfs_source_destroy(source);
        }
    }
}