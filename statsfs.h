#ifndef _STATSFS_H
#define _STATSFS_H

#include "statsfs_internal.h"

struct statsfs_value;
struct statsfs_source;

struct statsfs_source *statsfs_source_create(const char *fmt, ...);
void statsfs_source_destroy(struct statsfs_source *src);

void statsfs_source_add_values(struct statsfs_source *source,
                   const struct statsfs_value *stat,void *ptr);

void statsfs_source_add_aggregate(struct statsfs_source *source,
                                const struct statsfs_value *stat);

void statsfs_source_add_subordinate(
                struct statsfs_source *source,
                struct statsfs_source *sub);

// Does not destroys the subordinate!
void statsfs_source_remove_subordinate(
                struct statsfs_source *source,
                struct statsfs_source *sub);


int statsfs_source_get_value_by_val(
                struct statsfs_source *source,
                struct statsfs_value *val, void **ret);


int statsfs_source_get_value_by_name(
                struct statsfs_source *source,
                const char *name, void **val);

void statsfs_source_register(struct statsfs_source *source);

// vedi kref_get e kref_put in include/linux/kref.h
void statsfs_source_get(struct statsfs_source *);
void statsfs_source_put(struct statsfs_source *);


#endif