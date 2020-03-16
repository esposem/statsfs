#ifndef _STATSFS_H
#define _STATSFS_H

#include "list.h"

#define STATSFS_SIGN 0x8000

struct statsfs_source;

enum stat_type {
	STATSFS_U8 = 0,
	STATSFS_U16 = 1,
	STATSFS_U32 = 2,
	STATSFS_U64 = 3,
	STATSFS_BOOL = 4,
	STATSFS_S8 = STATSFS_U8 | STATSFS_SIGN,
	STATSFS_S16 = STATSFS_U16 | STATSFS_SIGN,
	STATSFS_S32 = STATSFS_U32 | STATSFS_SIGN,
	STATSFS_S64 = STATSFS_U64 | STATSFS_SIGN,
};

enum stat_aggr {
	STATSFS_NONE = 0,
	STATSFS_SUM,
	STATSFS_MIN,
	STATSFS_MAX,
	STATSFS_COUNT_ZERO,
	STATSFS_AVG,
};

struct statsfs_value {
	char *name;
	/* Offset from base address to field containing the value */
	int offset;

	enum stat_type type; /* STAT_TYPE_{BOOL,U64,...} */
	/* Bitmask with zero or more of STAT_AGGR_{MIN,MAX,SUM,...} */
	enum stat_aggr aggr_kind;
	uint16_t mode; /* File mode */
};

struct statsfs_source *statsfs_source_create(const char *fmt, ...);
void statsfs_source_destroy(struct statsfs_source *src);

// returns 0 if successful, -EEXIST if value already present
int statsfs_source_add_values(struct statsfs_source *source,
			      const struct statsfs_value *stat, void *ptr);

void statsfs_source_add_subordinate(struct statsfs_source *source,
				    struct statsfs_source *sub);

// Does not destroy the subordinate!
void statsfs_source_remove_subordinate(struct statsfs_source *source,
				       struct statsfs_source *sub);

int statsfs_source_get_value_by_val(struct statsfs_source *source,
				    struct statsfs_value *val, uint64_t *ret);

int statsfs_source_get_value_by_name(struct statsfs_source *source, char *name,
				     uint64_t *ret);

void statsfs_source_register(struct statsfs_source *source);

// vedi kref_get e kref_put in include/linux/kref.h
void statsfs_source_get(struct statsfs_source *);
void statsfs_source_put(struct statsfs_source *);

#endif