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

static int is_val_signed(struct statsfs_value *val)
{
	return val->type & STATSFS_SIGN;
}

static struct statsfs_value *
find_value_by_name(struct statsfs_value_source *src, char *val)
{
	struct statsfs_value *entry;

	for (entry = src->values; entry->name; entry++) {
		if (!strcmp(entry->name, val)) {
			return entry;
		}
	}
	return NULL;
}

static struct statsfs_value *find_value(struct statsfs_value_source *src,
					struct statsfs_value *val)
{
	struct statsfs_value *entry;
	for (entry = src->values; entry->name; entry++) {
		if (entry == val) {
			BUG_ON(strcmp(entry->name, val->name) == 0);
			return entry;
		}
	}
	return NULL;
}

static struct statsfs_value *
search_statsfs_in_source(struct statsfs_source *src, struct statsfs_value *arg,
			 struct statsfs_value_source **val_src, uint8_t is_aggr)
{
	struct statsfs_value *entry;
	struct statsfs_value_source *src_entry;
	uint8_t is_entry_aggr;

	list_for_each_entry (src_entry, &src->values_head, list_element) {
		entry = find_value(src_entry, arg);
		is_entry_aggr = entry->aggr_kind != STATSFS_NONE;
		if (entry && (is_entry_aggr == is_aggr)) {
			if (val_src) {
				*val_src = src_entry;
			}
			return entry;
		}
	}

	return NULL;
}

void statsfs_source_register(struct statsfs_source *source)
{
	// TODO: creates file /sys/kernel/statsfs/kvm
}

struct statsfs_source *statsfs_source_create(const char *fmt, ...)
{
	va_list ap;
	char buf[100];
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
	memcpy(ret->name, buf, char_needed);
	ret->name[char_needed] = '\0';

	ret->refcount = 1; //TODO: fix for kernel
	ret->values_head = (struct list_head)LIST_HEAD_INIT(ret->values_head);

	ret->subordinates_head =
		(struct list_head)LIST_HEAD_INIT(ret->subordinates_head);
	ret->list_element = (struct list_head)LIST_HEAD_INIT(ret->list_element);

	return ret;
}

void statsfs_source_destroy(struct statsfs_source *src)
{
	statsfs_source_put(src);
}

static struct statsfs_value_source *create_value_source(void *base)
{
	struct statsfs_value_source *val_src;

	val_src = malloc(sizeof(struct statsfs_value_source));
	if (!val_src) {
		printf("Error in making the value source!\n");
		return NULL;
	}

	val_src->base_addr = base;
	val_src->list_element =
		(struct list_head)LIST_HEAD_INIT(val_src->list_element);

	return val_src;
}

int statsfs_source_add_values(struct statsfs_source *source,
			      const struct statsfs_value *stat, void *ptr)
{
	BUG_ON(source != NULL);
	BUG_ON(ptr != NULL);

	struct statsfs_value_source *val_src;
	struct statsfs_value_source *entry;

	list_for_each_entry (entry, &source->values_head, list_element) {
		if (entry->base_addr == ptr && entry->values == stat) {
			return -EEXIST;
		}
	}

	val_src = create_value_source(ptr);
	val_src->values = (struct statsfs_value *)stat;
	// add the val_src to the source list
	list_add(&val_src->list_element, &source->values_head);

	return 0;
}

void statsfs_source_add_subordinate(struct statsfs_source *source,
				    struct statsfs_source *sub)
{
	BUG_ON(source != NULL);
	BUG_ON(sub != NULL);
	statsfs_source_get(sub);
	list_add(&sub->list_element, &source->subordinates_head);
}

// Does not destroy the subordinate!
void statsfs_source_remove_subordinate(struct statsfs_source *source,
				       struct statsfs_source *sub)
{
	struct list_head *it, *safe;
	struct statsfs_source *src_entry;

	BUG_ON(source != NULL);
	BUG_ON(sub != NULL);

	list_for_each_safe (it, safe, &source->subordinates_head) {
		src_entry = list_entry(it, struct statsfs_source, list_element);
		if (src_entry == sub) {
			BUG_ON(strcmp(src_entry->name, sub->name) == 0);
			list_del_init(&src_entry->list_element);
			statsfs_source_put(src_entry);
			return;
		}
	}

	printf("WARINING: Subordinate %s not found in %s\n", sub->name,
	       source->name);
}

static struct statsfs_value *
search_value_in_source(struct statsfs_source *src, struct statsfs_value *arg,
		       struct statsfs_value_source **val_src)
{
	return search_statsfs_in_source(src, arg, val_src, 0);
}

static uint64_t get_correct_value(void *address, enum stat_type type)
{
	uint64_t value_found;

	switch (type) {
	case STATSFS_U8:
		value_found = *((uint8_t *)address);
		break;
	case STATSFS_U8 | STATSFS_SIGN:
		value_found = *((int8_t *)address);
		break;
	case STATSFS_U16:
		value_found = *((uint16_t *)address);
		break;
	case STATSFS_U16 | STATSFS_SIGN:
		value_found = *((int16_t *)address);
		break;
	case STATSFS_U32:
		value_found = *((uint32_t *)address);
		break;
	case STATSFS_U32 | STATSFS_SIGN:
		value_found = *((int32_t *)address);
		break;
	case STATSFS_U64:
		value_found = *((uint64_t *)address);
		break;
	case STATSFS_U64 | STATSFS_SIGN:
		value_found = *((int64_t *)address);
		break;
	case STATSFS_BOOL:
		value_found = *((uint8_t *)address);
		break;
	default:
		value_found = 0;
		break;
	}

	return value_found;
}

static void search_all_simple_values(struct statsfs_source *src,
				     struct statsfs_aggregate_value *agg,
				     struct statsfs_value *val)
{
	struct statsfs_value *entry;
	struct statsfs_value_source *src_entry;
	uint64_t value_found;
	void *address;

	list_for_each_entry (src_entry, &src->values_head, list_element) {
		entry = find_value_by_name(src_entry, val->name);
		if (entry && entry->aggr_kind == STATSFS_NONE) { // found
			address = src_entry->base_addr + entry->offset;
			value_found = get_correct_value(address, val->type);
			agg->sum += value_found;
			agg->count++;
			agg->count_zero += (value_found == 0);

			if (is_val_signed(val)) {
				agg->max = (((int64_t)value_found) >=
					    ((int64_t)agg->max)) ?
						   value_found :
						   agg->max;
				agg->min = (((int64_t)value_found) <=
					    ((int64_t)agg->min)) ?
						   value_found :
						   agg->min;
			} else {
				agg->max = (value_found >= agg->max) ?
						   value_found :
						   agg->max;
				agg->min = (value_found <= agg->min) ?
						   value_found :
						   agg->min;
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
	list_for_each_entry (subordinate, &root->subordinates_head,
			     list_element) {
		do_recursive_aggregation(subordinate, val, agg);
	}
}

static void init_aggregate_value(struct statsfs_aggregate_value *agg,
				 struct statsfs_value *val)
{
	agg->count = agg->count_zero = agg->sum = 0;
	if (is_val_signed(val)) {
		agg->max = INT64_MIN;
		agg->min = INT64_MAX;
	} else {
		agg->max = 0;
		agg->min = UINT64_MAX;
	}
}

static void set_final_value(struct statsfs_aggregate_value *agg,
			    struct statsfs_value *val, uint64_t *ret)
{
	int operation = val->aggr_kind | is_val_signed(val);

	switch (operation) {
	case STATSFS_AVG:
		*ret = agg->count ? agg->sum / agg->count : 0;
		break;
	case STATSFS_AVG | STATSFS_SIGN:
		*ret = agg->count ? ((int64_t)agg->sum) / agg->count : 0;
		break;
	case STATSFS_SUM:
	case STATSFS_SUM | STATSFS_SIGN:
		*ret = agg->sum;
		break;
	case STATSFS_MIN:
	case STATSFS_MIN | STATSFS_SIGN:
		*ret = agg->min;
		break;
	case STATSFS_MAX:
	case STATSFS_MAX | STATSFS_SIGN:
		*ret = agg->max;
		break;
	case STATSFS_COUNT_ZERO:
	case STATSFS_COUNT_ZERO | STATSFS_SIGN:
		*ret = agg->count_zero;
		break;
	default:
		break;
	}
}

static struct statsfs_value *
search_aggr_in_source_by_val(struct statsfs_source *src,
			     struct statsfs_value *val)
{
	return search_statsfs_in_source(src, val, NULL, 1);
}

int statsfs_source_get_value(struct statsfs_source *source,
			     struct statsfs_value *arg, uint64_t *ret)
{
	struct statsfs_value_source *entry;
	struct statsfs_value *found;
	struct statsfs_aggregate_value aggr;
	void *address;
	BUG_ON(ret != NULL);
	BUG_ON(source != NULL);

	if (!arg) {
		return -ENOENT;
	}

	// look in simple values
	found = search_value_in_source(source, arg, &entry);
	if (found) {
		address = entry->base_addr + found->offset;
		*ret = get_correct_value(address, found->type);
		return 0;
	}

	// look in aggregates
	found = search_aggr_in_source_by_val(source, arg);
	if (found) {
		BUG_ON(found->aggr_kind != STATSFS_NONE);
		init_aggregate_value(&aggr, found);
		do_recursive_aggregation(source, found, &aggr);
		set_final_value(&aggr, found, ret);
		return 0;
	}

	printf("ERROR: Value in source \"%s\" not found!\n", source->name);
	return -ENOENT;
}

static struct statsfs_value *
search_in_source_by_name(struct statsfs_source *src, char *name)
{
	struct statsfs_value *entry;
	struct statsfs_value_source *src_entry;

	list_for_each_entry (src_entry, &src->values_head, list_element) {
		entry = find_value_by_name(src_entry, name);
		if (entry) {
			return entry;
		}
	}

	return NULL;
}

int statsfs_source_get_value_by_name(struct statsfs_source *source, char *name,
				     uint64_t *ret)
{
	struct statsfs_value *val;
	*ret = 0;

	// search in my values
	val = search_in_source_by_name(source, name);

	if (!val) {
		return -ENOENT;
	}
	// printf("Found %s %d\n", val->name, val->aggr_kind);
	return statsfs_source_get_value(source, val, ret);
}

void statsfs_source_get(struct statsfs_source *source)
{
	// TODO: fix for kernel
	source->refcount++;
}

void statsfs_source_put(struct statsfs_source *source)
{
	struct list_head *it, *safe;
	struct statsfs_value_source *val_src_entry;
	struct statsfs_source *src_entry;
	BUG_ON(source != NULL);

	// TODO: fix for kernel
	if (!source->refcount) {
		return;
	}
	source->refcount--;

	if (source->refcount == 0) {
		free(source->name);
		list_del(
			&source->list_element); // deletes it from the list he's in

		// iterate through the values and delete them
		list_for_each_safe (it, safe, &source->values_head) {
			val_src_entry = list_entry(
				it, struct statsfs_value_source, list_element);
			free(val_src_entry);
		}

		// iterate through the subordinates and delete them
		list_for_each_safe (it, safe, &source->subordinates_head) {
			src_entry = list_entry(it, struct statsfs_source,
					       list_element);
			statsfs_source_put(src_entry);
		}

		free(source);
	}
}