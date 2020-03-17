#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>
#include <errno.h>
#include "statsfs.h"
#include "statsfs_internal.h"
#include "test_helpers.h"

#define STATSFS_STAT(el, x, ...)                                               \
	{                                                                      \
		.name = #x, .offset = offsetof(struct container, el.x),        \
		##__VA_ARGS__                                                  \
	}

#define ARR_SIZE(el) (sizeof(el) / sizeof(struct statsfs_value) - 1)

struct test_values_struct {
	uint64_t u64;
	int32_t s32;
	bool bo;
	uint8_t u8;
	int16_t s16;
};

struct container {
	struct test_values_struct vals;
};

struct statsfs_value test_values[6] = {
	STATSFS_STAT(vals, u64, .type = STATSFS_U64, .aggr_kind = STATSFS_NONE,
		     .mode = 0),
	STATSFS_STAT(vals, s32, .type = STATSFS_S32, .aggr_kind = STATSFS_NONE,
		     .mode = 0),
	STATSFS_STAT(vals, bo, .type = STATSFS_BOOL, .aggr_kind = STATSFS_NONE,
		     .mode = 0),
	STATSFS_STAT(vals, u8, .type = STATSFS_U8, .aggr_kind = STATSFS_NONE,
		     .mode = 0),
	STATSFS_STAT(vals, s16, .type = STATSFS_S16, .aggr_kind = STATSFS_NONE,
		     .mode = 0),
	{ NULL },
};

struct statsfs_value test_aggr[4] = {
	STATSFS_STAT(vals, s32, .type = STATSFS_S32, .aggr_kind = STATSFS_MIN,
		     .mode = 0),
	STATSFS_STAT(vals, bo, .type = STATSFS_BOOL, .aggr_kind = STATSFS_MAX,
		     .mode = 0),
	STATSFS_STAT(vals, u64, .type = STATSFS_U64, .aggr_kind = STATSFS_SUM,
		     .mode = 0),
	{ NULL },
};

struct statsfs_value test_same_name[3] = {
	STATSFS_STAT(vals, s32, .type = STATSFS_S32, .aggr_kind = STATSFS_NONE,
		     .mode = 0),
	STATSFS_STAT(vals, s32, .type = STATSFS_S32, .aggr_kind = STATSFS_MIN,
		     .mode = 0),
	{ NULL },
};

struct statsfs_value test_all_aggr[6] = {
	STATSFS_STAT(vals, s32, .type = STATSFS_S32, .aggr_kind = STATSFS_MIN,
		     .mode = 0),
	STATSFS_STAT(vals, bo, .type = STATSFS_BOOL,
		     .aggr_kind = STATSFS_COUNT_ZERO, .mode = 0),
	STATSFS_STAT(vals, u64, .type = STATSFS_U64, .aggr_kind = STATSFS_SUM,
		     .mode = 0),
	STATSFS_STAT(vals, u8, .type = STATSFS_U8, .aggr_kind = STATSFS_AVG,
		     .mode = 0),
	STATSFS_STAT(vals, s16, .type = STATSFS_S16, .aggr_kind = STATSFS_MAX,
		     .mode = 0),
	{ NULL },
};

struct kvm;

struct test_vcpu {
	uint64_t u64;
	int32_t s32;
	bool bo;
	uint8_t u8;
	int16_t s16;
};

struct kvm_vcpu {
	char more_useless[10];
	struct kvm *kvm;
	void *more_more_useless;
	struct test_vcpu vals_vcpu;
	int useless;
};

struct kvm_vcpu vcpu = {
	.kvm = NULL,
	.vals_vcpu = {
		.u64 = 1234,
		.s32 = -10,
		.bo = true,
		.u8 = 255,
		.s16 = -1232,
	},
};

struct test_vm {
	uint64_t vm_u64;
	int32_t vm_s32;
	bool vm_bo;
	uint8_t vm_u8;
	int16_t vm_s16;
};

struct kvm {
	int useless;
	char more_useless[10];
	struct kvm_vcpu *vcpus;
	struct test_vm vals_vm;
	void *more_more_useless;
};

struct kvm kvm = {
	.vcpus = &vcpu,
	.vals_vm = {
		.vm_u64 = 9843223,
		.vm_s32 = -93223,
		.vm_bo = false,
		.vm_u8 = 0,
		.vm_s16 = -9999,
	},
};

#define STATSFS_VCPU(x, ...)                                               \
	{                                                                      \
		.name = #x, .offset = offsetof(struct kvm_vcpu, vals_vcpu.x),        \
		##__VA_ARGS__                                                  \
	}

#define STATSFS_VM(x, ...)                                               \
	{                                                                      \
		.name = #x, .offset = offsetof(struct kvm, vals_vm.x),        \
		##__VA_ARGS__                                                  \
	}


struct statsfs_value test_vcpu[6] = {
	STATSFS_VCPU(s32, .type = STATSFS_S32, .aggr_kind = STATSFS_NONE,
		     .mode = 0),
	STATSFS_VCPU(bo, .type = STATSFS_BOOL,
		     .aggr_kind = STATSFS_NONE, .mode = 0),
	STATSFS_VCPU(u64, .type = STATSFS_U64, .aggr_kind = STATSFS_NONE,
		     .mode = 0),
	STATSFS_VCPU(u8, .type = STATSFS_U8, .aggr_kind = STATSFS_NONE,
		     .mode = 0),
	STATSFS_VCPU(s16, .type = STATSFS_S16, .aggr_kind = STATSFS_NONE,
		     .mode = 0),
	{ NULL },
};

struct statsfs_value test_vm[6] = {
	STATSFS_STAT(vm_s32, .type = STATSFS_S32, .aggr_kind = STATSFS_MIN,
		     .mode = 0),
	STATSFS_STAT(vm_bo, .type = STATSFS_BOOL,
		     .aggr_kind = STATSFS_COUNT_ZERO, .mode = 0),
	STATSFS_STAT(vm_u64, .type = STATSFS_U64, .aggr_kind = STATSFS_SUM,
		     .mode = 0),
	STATSFS_STAT(vm_u8, .type = STATSFS_U8, .aggr_kind = STATSFS_AVG,
		     .mode = 0),
	STATSFS_STAT(vm_s16, .type = STATSFS_S16, .aggr_kind = STATSFS_MAX,
		     .mode = 0),
	{ NULL },
};


#define def_u64 64

#define def_val_s32 INT32_MIN
#define def_val_bool true
#define def_val_u8 127
#define def_val_s16 10000

#define def_val2_s32 INT16_MAX
#define def_val2_bool false
#define def_val2_u8 255
#define def_val2_s16 -20000

struct container cont = {
	.vals =
		{
			.u64 = def_u64,
			.s32 = def_val_s32,
			.bo = def_val_bool,
			.u8 = def_val_u8,
			.s16 = def_val_s16,
		},
};

struct container cont2 = {
	.vals =
		{
			.u64 = def_u64,
			.s32 = def_val2_s32,
			.bo = def_val2_bool,
			.u8 = def_val2_u8,
			.s16 = def_val2_s16,
		},
};

static void test_empty_folder()
{
	struct statsfs_source *src;

	src = statsfs_source_create("kvm_%d", 123);
	assert(!strcmp(src->name, "kvm_123"));
	assert(get_number_subsources(src) == 0);
	assert(get_number_values(src) == 0);
	assert(get_number_aggregates(src) == 0);
	statsfs_source_destroy(src);
}

static void test_add_subfolder()
{
	struct statsfs_source *src, *sub;
	src = statsfs_source_create("parent");
	sub = statsfs_source_create("child");
	statsfs_source_add_subordinate(src, sub);
	assert(source_has_subsource(src, sub));
	assert(get_number_subsources(src) == 1);
	assert(get_number_values(src) == 0);
	assert(get_number_aggregates(src) == 0);
	assert(get_number_values(sub) == 0);
	assert(get_number_aggregates(sub) == 0);
	assert(get_total_number_values(src) == 0);

	sub = statsfs_source_create("not a child");
	assert(!source_has_subsource(src, sub));
	assert(get_number_subsources(src) == 1);

	statsfs_source_destroy(src);
}

static void test_add_value()
{
	struct statsfs_source *src;
	int n;

	src = statsfs_source_create("parent");

	// add values
	n = statsfs_source_add_values(src, test_values, &cont);
	assert(n == 0);
	n = get_number_values_with_base(src, &cont);
	assert(n == ARR_SIZE(test_values));

	// add same values, nothing happens
	n = statsfs_source_add_values(src, test_values, &cont);
	assert(n == -EEXIST);
	n = get_number_values_with_base(src, &cont);
	assert(n == ARR_SIZE(test_values));

	// size is invaried
	assert(get_number_values(src) == ARR_SIZE(test_values));

	// no aggregates
	n = get_number_aggr_with_base(src, &cont);
	assert(n == 0);
	assert(get_number_values(src) == ARR_SIZE(test_values));
	assert(get_number_aggregates(src) == 0);
	statsfs_source_destroy(src);
}

static void test_add_value_in_subfolder()
{
	struct statsfs_source *src, *sub, *sub_not;
	int n;

	src = statsfs_source_create("parent");
	sub = statsfs_source_create("child");

	// src -> sub
	statsfs_source_add_subordinate(src, sub);

	// add values
	n = statsfs_source_add_values(sub, test_values, &cont);
	assert(n == 0);
	n = get_number_values_with_base(sub, &cont);
	assert(n == ARR_SIZE(test_values));
	assert(get_number_values(src) == 0);
	assert(get_number_aggregates(src) == 0);
	assert(get_total_number_values(src) == ARR_SIZE(test_values));

	assert(get_number_values(sub) == ARR_SIZE(test_values));
	// no values in sub
	assert(get_number_aggregates(sub) == 0);

	// different folder
	sub_not = statsfs_source_create("not a child");

	// add values
	n = statsfs_source_add_values(sub_not, test_values, &cont);
	assert(n == 0);
	n = get_number_values_with_base(sub_not, &cont);
	assert(n == ARR_SIZE(test_values));
	assert(get_number_values(src) == 0);
	assert(get_number_aggregates(src) == 0);
	assert(get_total_number_values(src) == ARR_SIZE(test_values));

	// remove sub, check values is 0
	statsfs_source_remove_subordinate(src, sub);
	assert(get_total_number_values(src) == 0);

	// re-add sub, check value are added
	statsfs_source_add_subordinate(src, sub);
	assert(get_total_number_values(src) == ARR_SIZE(test_values));

	// add sub_not, check value are twice as many
	statsfs_source_add_subordinate(src, sub_not);
	assert(get_total_number_values(src) == ARR_SIZE(test_values) * 2);

	assert(get_number_values(sub_not) == ARR_SIZE(test_values));
	assert(get_number_aggregates(sub_not) == 0);

	statsfs_source_destroy(src);
}

static void test_search_value()
{
	struct statsfs_source *src;
	uint64_t ret;
	int n;

	src = statsfs_source_create("parent");

	// add values
	n = statsfs_source_add_values(src, test_values, &cont);
	assert(n == 0);
	n = get_number_values_with_base(src, &cont);
	assert(n == ARR_SIZE(test_values));

	// get u64
	n = statsfs_source_get_value_by_name(src, "u64", &ret);
	assert(ret == def_u64 && n == 0);

	n = statsfs_source_get_value_by_name(src, "s32", &ret);
	assert(((int32_t)ret) == def_val_s32 && n == 0);

	n = statsfs_source_get_value_by_name(src, "bo", &ret);
	assert(((bool)ret) == def_val_bool && n == 0);

	// get a non-added value
	n = statsfs_source_get_value_by_name(src, "does not exist", &ret);
	assert(ret == 0 && n == -ENOENT);

	statsfs_source_destroy(src);
}

static void test_search_value_in_subfolder()
{
	struct statsfs_source *src, *sub;
	uint64_t ret;
	int n;

	src = statsfs_source_create("parent");
	sub = statsfs_source_create("child");

	// src -> sub
	statsfs_source_add_subordinate(src, sub);

	// add values to sub
	n = statsfs_source_add_values(sub, test_values, &cont);
	assert(n == 0);
	n = get_number_values_with_base(sub, &cont);
	assert(n == ARR_SIZE(test_values));

	n = statsfs_source_get_value_by_name(sub, "u64", &ret);
	assert(ret == def_u64 && n == 0);
	n = statsfs_source_get_value_by_name(src, "u64", &ret);
	assert(ret == 0 && n == -ENOENT);

	n = statsfs_source_get_value_by_name(sub, "s32", &ret);
	assert(((int32_t)ret) == def_val_s32 && n == 0);
	n = statsfs_source_get_value_by_name(src, "s32", &ret);
	assert(ret == 0 && n == -ENOENT);

	n = statsfs_source_get_value_by_name(sub, "bo", &ret);
	assert(((bool)ret) == def_val_bool && n == 0);
	n = statsfs_source_get_value_by_name(src, "bo", &ret);
	assert(ret == 0 && n == -ENOENT);

	n = statsfs_source_get_value_by_name(sub, "does not exist", &ret);
	assert(ret == 0 && n == -ENOENT);
	n = statsfs_source_get_value_by_name(src, "does not exist", &ret);
	assert(ret == 0 && n == -ENOENT);

	statsfs_source_destroy(src);
}

static void test_search_value_in_empty_folder()
{
	struct statsfs_source *src;
	uint64_t ret;
	int n;

	src = statsfs_source_create("empty folder");
	assert(get_number_aggregates(src) == 0);
	assert(get_number_subsources(src) == 0);
	assert(get_number_values(src) == 0);

	n = statsfs_source_get_value_by_name(src, "u64", &ret);
	assert(ret == 0 && n == -ENOENT);

	n = statsfs_source_get_value_by_name(src, "s32", &ret);
	assert(ret == 0 && n == -ENOENT);

	n = statsfs_source_get_value_by_name(src, "bo", &ret);
	assert(ret == 0 && n == -ENOENT);

	n = statsfs_source_get_value_by_name(src, "does not exist", &ret);
	assert(ret == 0 && n == -ENOENT);

	statsfs_source_destroy(src);
}

static void test_add_aggregate()
{
	struct statsfs_source *src;
	int n;

	src = statsfs_source_create("parent");

	// add aggr to src, no values
	n = statsfs_source_add_values(src, test_aggr, NULL);
	assert(n == 0);
	n = get_number_values_with_base(src, NULL);
	assert(n == 0);

	// count values
	n = get_number_aggr_with_base(src, NULL);
	assert(n == ARR_SIZE(test_aggr));

	// add same array again, should not be added
	n = statsfs_source_add_values(src, test_aggr, NULL);
	assert(n == -EEXIST);
	n = get_number_aggr_with_base(src, NULL);
	assert(n == ARR_SIZE(test_aggr));

	assert(get_number_values(src) == 0);
	assert(get_number_aggregates(src) == ARR_SIZE(test_aggr));

	statsfs_source_destroy(src);
}

static void test_add_aggregate_in_subfolder()
{
	struct statsfs_source *src, *sub, *sub_not;
	int n;

	src = statsfs_source_create("parent");
	sub = statsfs_source_create("child");
	// src->sub
	statsfs_source_add_subordinate(src, sub);

	// add aggr to sub
	n = statsfs_source_add_values(sub, test_aggr, NULL);
	assert(n == 0);
	n = get_number_aggr_with_base(sub, NULL);
	assert(n == ARR_SIZE(test_aggr));
	assert(get_number_values(src) == 0);
	assert(get_number_aggregates(src) == 0);
	assert(get_total_number_values(src) == 0);

	assert(get_number_values(sub) == 0);
	assert(get_number_aggregates(sub) == ARR_SIZE(test_aggr));

	// not a child
	sub_not = statsfs_source_create("not a child");

	// add aggr to "not a child"
	n = statsfs_source_add_values(sub_not, test_aggr, NULL);
	assert(n == 0);
	n = get_number_aggr_with_base(sub_not, NULL);
	assert(n == ARR_SIZE(test_aggr));
	assert(get_number_values(src) == 0);
	assert(get_number_aggregates(src) == 0);
	assert(get_total_number_values(src) == 0);

	// remove sub
	statsfs_source_remove_subordinate(src, sub);
	assert(get_total_number_values(src) == 0);

	// re-add both
	statsfs_source_add_subordinate(src, sub);
	assert(get_total_number_values(src) == 0);
	statsfs_source_add_subordinate(src, sub_not);
	assert(get_total_number_values(src) == 0);

	assert(get_number_values(sub_not) == 0);
	assert(get_number_aggregates(sub_not) == ARR_SIZE(test_aggr));

	statsfs_source_destroy(src);
}

static void test_search_aggregate()
{
	struct statsfs_source *src;
	uint64_t ret;
	int n;

	src = statsfs_source_create("parent");
	n = statsfs_source_add_values(src, test_aggr, NULL);
	assert(n == 0);
	n = get_number_aggr_with_base(src, NULL);
	assert(n == ARR_SIZE(test_aggr));
	n = get_number_aggr_with_base(src, &cont);
	assert(n == 0);
	n = statsfs_source_get_value_by_name(src, "u64", &ret);
	assert(ret == 0 && n == 0);

	n = statsfs_source_get_value_by_name(src, "s32", &ret);
	assert(ret == INT64_MAX && n == 0);

	n = statsfs_source_get_value_by_name(src, "bo", &ret);
	assert(ret == 0 && n == 0);

	n = statsfs_source_get_value_by_name(src, "does not exist", &ret);
	assert(ret == 0 && n == -ENOENT);
	statsfs_source_destroy(src);
}

static void test_search_aggregate_in_subfolder()
{
	struct statsfs_source *src, *sub;
	uint64_t ret;
	int n;

	src = statsfs_source_create("parent");
	sub = statsfs_source_create("child");

	statsfs_source_add_subordinate(src, sub);

	n = statsfs_source_add_values(sub, test_aggr, NULL);
	assert(n == 0);
	n = get_number_aggr_with_base(sub, NULL);
	assert(n == ARR_SIZE(test_aggr));
	n = get_number_aggr_with_base(sub, &cont);
	assert(n == 0);

	// no u64 in test_aggr
	n = statsfs_source_get_value_by_name(sub, "u64", &ret);
	assert(ret == 0 && n == 0);
	n = statsfs_source_get_value_by_name(src, "u64", &ret);
	assert(ret == 0 && n == -ENOENT);

	n = statsfs_source_get_value_by_name(sub, "s32", &ret);
	assert(ret == INT64_MAX && n == 0);
	n = statsfs_source_get_value_by_name(src, "s32", &ret);
	assert(ret == 0 && n == -ENOENT);

	n = statsfs_source_get_value_by_name(sub, "bo", &ret);
	assert(ret == 0 && n == 0);
	n = statsfs_source_get_value_by_name(src, "bo", &ret);
	assert(ret == 0 && n == -ENOENT);

	n = statsfs_source_get_value_by_name(sub, "does not exist", &ret);
	assert(ret == 0 && n == -ENOENT);
	n = statsfs_source_get_value_by_name(src, "does not exist", &ret);
	assert(ret == 0 && n == -ENOENT);

	statsfs_source_destroy(src);
}

void test_search_same()
{
	struct statsfs_source *src;
	uint64_t ret;
	int n;

	src = statsfs_source_create("parent");
	n = statsfs_source_add_values(src, test_same_name, &cont);
	assert(n == 0);
	n = get_number_values_with_base(src, &cont);
	assert(n == 1);
	n = get_number_aggr_with_base(src, &cont);
	assert(n == 1);

	n = statsfs_source_add_values(src, test_same_name, &cont);
	assert(n == -EEXIST);
	n = get_number_values_with_base(src, &cont);
	assert(n == 1);
	n = get_number_aggr_with_base(src, &cont);
	assert(n == 1);

	// returns first the value
	n = statsfs_source_get_value_by_name(src, "s32", &ret);
	assert(((int32_t)ret) == def_val_s32 && n == 0);

	statsfs_source_destroy(src);
}

static void test_add_mixed()
{
	struct statsfs_source *src;
	int n;

	src = statsfs_source_create("parent");

	n = statsfs_source_add_values(src, test_aggr, NULL);
	assert(n == 0);
	n = get_number_values_with_base(src, NULL);
	assert(n == 0);
	n = statsfs_source_add_values(src, test_values, &cont);
	assert(n == 0);
	n = get_number_aggr_with_base(src, NULL);
	assert(n == ARR_SIZE(test_aggr));

	n = statsfs_source_add_values(src, test_values, &cont);
	assert(n == -EEXIST);
	n = get_number_values_with_base(src, &cont);
	assert(n == ARR_SIZE(test_values));
	n = statsfs_source_add_values(src, test_aggr, NULL);
	assert(n == -EEXIST);
	n = get_number_aggr_with_base(src, NULL);
	assert(n == ARR_SIZE(test_aggr));

	assert(get_number_values(src) == ARR_SIZE(test_values));
	assert(get_number_aggregates(src) == ARR_SIZE(test_aggr));
	statsfs_source_destroy(src);
}

static void test_search_mixed()
{
	struct statsfs_source *src, *sub;
	uint64_t ret;
	int n;

	src = statsfs_source_create("parent");
	sub = statsfs_source_create("child");
	statsfs_source_add_subordinate(src, sub);

	// src has the aggregates, sub the values. Just search
	n = statsfs_source_add_values(sub, test_values, &cont);
	assert(n == 0);
	n = get_number_values_with_base(sub, &cont);
	assert(n == ARR_SIZE(test_values));
	n = statsfs_source_add_values(src, test_aggr, &cont);
	assert(n == 0);
	n = get_number_aggr_with_base(src, &cont);
	assert(n == ARR_SIZE(test_aggr));

	// u64 is sum so again same value
	n = statsfs_source_get_value_by_name(sub, "u64", &ret);
	assert(ret == def_u64 && n == 0);
	n = statsfs_source_get_value_by_name(src, "u64", &ret);
	assert(ret == def_u64 && n == 0);

	// s32 is min so return the value also in the aggregate
	n = statsfs_source_get_value_by_name(sub, "s32", &ret);
	assert(((int32_t)ret) == def_val_s32 && n == 0);
	n = statsfs_source_get_value_by_name(src, "s32", &ret);
	assert(((int32_t)ret) == def_val_s32 && n == 0);

	// bo is max
	n = statsfs_source_get_value_by_name(sub, "bo", &ret);
	assert(ret == def_val_bool && n == 0);
	n = statsfs_source_get_value_by_name(src, "bo", &ret);
	assert(ret == def_val_bool && n == 0);

	n = statsfs_source_get_value_by_name(sub, "does not exist", &ret);
	assert(ret == 0 && n == -ENOENT);
	n = statsfs_source_get_value_by_name(src, "does not exist", &ret);
	assert(ret == 0 && n == -ENOENT);

	statsfs_source_destroy(src);
}

static void test_all_aggregations_agg_val_val()
{
	struct statsfs_source *src, *sub1, *sub2;
	uint64_t ret;
	int n;

	src = statsfs_source_create("parent");
	sub1 = statsfs_source_create("child1");
	sub2 = statsfs_source_create("child2");
	statsfs_source_add_subordinate(src, sub1);
	statsfs_source_add_subordinate(src, sub2);

	n = statsfs_source_add_values(sub1, test_all_aggr, &cont);
	assert(n == 0);
	n = get_number_aggr_with_base(sub1, &cont);
	assert(n == ARR_SIZE(test_all_aggr));
	n = statsfs_source_add_values(sub2, test_all_aggr, &cont2);
	assert(n == 0);
	n = get_number_aggr_with_base(sub2, &cont2);
	assert(n == ARR_SIZE(test_all_aggr));

	// assert(get_number_aggregates(src) == ARR_SIZE(test_all_aggr) * 2);

	n = statsfs_source_add_values(src, test_all_aggr, NULL);
	assert(n == 0);
	n = get_number_aggr_with_base(src, NULL);
	assert(n == ARR_SIZE(test_all_aggr));

	// sum
	n = statsfs_source_get_value_by_name(src, "u64", &ret);
	assert(n == 0 && ret == def_u64 * 2);

	// min
	n = statsfs_source_get_value_by_name(src, "s32", &ret);
	assert(n == 0 && ((int32_t)ret) == def_val_s32);

	// count_0
	n = statsfs_source_get_value_by_name(src, "bo", &ret);
	assert(n == 0 && ret == 1);

	// avg
	n = statsfs_source_get_value_by_name(src, "u8", &ret);
	assert(n == 0 && ret == 191);

	// max
	n = statsfs_source_get_value_by_name(src, "s16", &ret);
	assert(n == 0 && ret == def_val_s16);

	statsfs_source_destroy(src);
}

static void test_all_aggregations_val_agg_val()
{
	struct statsfs_source *src, *sub1, *sub2;
	uint64_t ret;
	int n;
	src = statsfs_source_create("parent");
	sub1 = statsfs_source_create("child1");
	sub2 = statsfs_source_create("child2");
	statsfs_source_add_subordinate(src, sub1);
	statsfs_source_add_subordinate(src, sub2);

	n = statsfs_source_add_values(src, test_all_aggr, &cont);
	assert(n == 0);
	n = get_number_aggr_with_base(src, &cont);
	assert(n == ARR_SIZE(test_all_aggr));
	n = statsfs_source_add_values(sub2, test_all_aggr, &cont2);
	assert(n == 0);
	n = get_number_aggr_with_base(sub2, &cont2);
	assert(n == ARR_SIZE(test_all_aggr));

	n = statsfs_source_add_values(sub1, test_all_aggr, NULL);
	assert(n == 0);
	n = get_number_aggr_with_base(sub1, NULL);
	assert(n == ARR_SIZE(test_all_aggr));

	n = statsfs_source_get_value_by_name(src, "u64", &ret);
	assert(n == 0 && ret == def_u64);
	n = statsfs_source_get_value_by_name(sub1, "u64", &ret);
	assert(n == 0 && ret == 0);
	n = statsfs_source_get_value_by_name(sub2, "u64", &ret);
	assert(n == 0 && ret == def_u64);

	n = statsfs_source_get_value_by_name(src, "s32", &ret);
	assert(n == 0 && ((int32_t)ret) == def_val_s32);
	n = statsfs_source_get_value_by_name(sub1, "s32", &ret);
	assert(n == 0 && ret == INT64_MAX); // MIN
	n = statsfs_source_get_value_by_name(sub2, "s32", &ret);
	assert(n == 0 && ((int32_t)ret) == def_val2_s32);

	n = statsfs_source_get_value_by_name(src, "bo", &ret);
	assert(n == 0 && ret == def_val_bool);
	n = statsfs_source_get_value_by_name(sub1, "bo", &ret);
	assert(n == 0 && ret == 0);
	n = statsfs_source_get_value_by_name(sub2, "bo", &ret);
	assert(n == 0 && ret == def_val2_bool);

	n = statsfs_source_get_value_by_name(src, "u8", &ret);
	assert(n == 0 && ret == def_val_u8);
	n = statsfs_source_get_value_by_name(sub1, "u8", &ret);
	assert(n == 0 && ret == 0);
	n = statsfs_source_get_value_by_name(sub2, "u8", &ret);
	assert(n == 0 && ret == def_val2_u8);

	n = statsfs_source_get_value_by_name(src, "s16", &ret);
	assert(n == 0 && ret == def_val_s16);
	n = statsfs_source_get_value_by_name(sub1, "s16", &ret);
	assert(n == 0 && ret == INT64_MIN); // MAX
	n = statsfs_source_get_value_by_name(sub2, "s16", &ret);
	assert(n == 0 && ret == def_val2_s16);

	statsfs_source_destroy(src);
}

static void test_all_aggregations_agg_val_val_sub()
{
	struct statsfs_source *src, *sub1, *sub11;
	uint64_t ret;
	int n;

	src = statsfs_source_create("parent");
	sub1 = statsfs_source_create("child1");
	sub11 = statsfs_source_create("child11");
	statsfs_source_add_subordinate(src, sub1);
	statsfs_source_add_subordinate(sub1, sub11); // changes here!

	n = statsfs_source_add_values(sub1, test_values, &cont);
	assert(n == 0);
	n = get_number_values_with_base(sub1, &cont);
	assert(n == ARR_SIZE(test_values));
	n = statsfs_source_add_values(sub11, test_values, &cont2);
	assert(n == 0);
	n = get_number_values_with_base(sub11, &cont2);
	assert(n == ARR_SIZE(test_values));

	assert(get_total_number_values(src) == ARR_SIZE(test_values) * 2);

	n = statsfs_source_add_values(sub1, test_all_aggr, &cont);
	assert(n == 0);
	n = get_number_aggr_with_base(sub1, &cont);
	assert(n == ARR_SIZE(test_all_aggr));
	n = statsfs_source_add_values(sub11, test_all_aggr, &cont2);
	assert(n == 0);
	n = get_number_aggr_with_base(sub11, &cont2);
	assert(n == ARR_SIZE(test_all_aggr));

	n = statsfs_source_add_values(src, test_all_aggr, NULL);
	assert(n == 0);
	n = get_number_aggr_with_base(src, NULL);
	assert(n == ARR_SIZE(test_all_aggr));

	// sum
	n = statsfs_source_get_value_by_name(src, "u64", &ret);
	assert(n == 0 && ret == def_u64 * 2);

	// min
	n = statsfs_source_get_value_by_name(src, "s32", &ret);
	assert(n == 0 && ((int32_t)ret) == def_val_s32);

	// count_0
	n = statsfs_source_get_value_by_name(src, "bo", &ret);
	assert(n == 0 && ret == 1);

	// avg
	n = statsfs_source_get_value_by_name(src, "u8", &ret);
	assert(n == 0 && ret == 191);

	// max
	n = statsfs_source_get_value_by_name(src, "s16", &ret);
	assert(n == 0 && ret == def_val_s16);

	statsfs_source_destroy(src);
}

static void test_all_aggregations_agg_no_val_sub()
{
	struct statsfs_source *src, *sub1, *sub11;
	uint64_t ret;
	int n;

	src = statsfs_source_create("parent");
	sub1 = statsfs_source_create("child1");
	sub11 = statsfs_source_create("child11");
	statsfs_source_add_subordinate(src, sub1);
	statsfs_source_add_subordinate(sub1, sub11);

	// it changes from the previous here!
	n = statsfs_source_add_values(sub11, test_all_aggr, &cont2);
	assert(n == 0);
	n = get_number_aggr_with_base(sub11, &cont2);
	assert(n == ARR_SIZE(test_all_aggr));

	assert(get_total_number_values(src) == 0);

	n = statsfs_source_add_values(src, test_all_aggr, NULL);
	assert(n == 0);
	n = get_number_aggr_with_base(src, NULL);
	assert(n == ARR_SIZE(test_all_aggr));

	// sum
	n = statsfs_source_get_value_by_name(src, "u64", &ret);
	assert(n == 0 && ret == def_u64);

	// min
	n = statsfs_source_get_value_by_name(src, "s32", &ret);
	assert(n == 0 && ((int32_t)ret) == def_val2_s32);

	// count_0
	n = statsfs_source_get_value_by_name(src, "bo", &ret);
	assert(n == 0 && ret == 1);

	// avg
	n = statsfs_source_get_value_by_name(src, "u8", &ret);
	assert(n == 0 && ret == def_val2_u8);

	// max
	n = statsfs_source_get_value_by_name(src, "s16", &ret);
	assert(n == 0 && ret == def_val2_s16);

	statsfs_source_destroy(src);
}

int main(int argc, char *argv[])
{
	test_empty_folder();
	test_add_subfolder();
	test_add_value();
	test_add_value_in_subfolder();
	test_search_value();
	test_search_value_in_subfolder();
	test_search_value_in_empty_folder();
	test_add_aggregate();
	test_add_aggregate_in_subfolder();
	test_search_aggregate();
	test_search_aggregate_in_subfolder();
	test_search_same();
	test_add_mixed();
	test_search_mixed();
	test_all_aggregations_agg_val_val();
	test_all_aggregations_val_agg_val();
	test_all_aggregations_agg_val_val_sub();
	test_all_aggregations_agg_no_val_sub();
	printf("Test completed\n");
}