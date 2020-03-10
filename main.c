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

#define STATSFS_STAT(el, x, ...) { .name= #x, .offset= offsetof(struct container, el.x), ## __VA_ARGS__ }

#define ARR_SIZE(el) (sizeof(el) / sizeof(struct statsfs_value) - 1)

struct test_values {
    uint64_t u64;
    int32_t s32;
    bool bo;
    uint8_t u8;
    int16_t s16;
};

struct container {
    struct test_values vals;
    struct test_values vals2;
};

struct statsfs_value test_values[6] = {
    STATSFS_STAT(vals, u64, .type=STATSFS_U64,
                .aggr_kind = STATSFS_NONE, .mode =0),
    STATSFS_STAT(vals, s32, .type=STATSFS_S32,
                .aggr_kind = STATSFS_NONE, .mode =0),
    STATSFS_STAT(vals, bo, .type=STATSFS_BOOL,
                .aggr_kind = STATSFS_NONE, .mode =0),
    STATSFS_STAT(vals, u8, .type=STATSFS_U8,
                .aggr_kind = STATSFS_NONE, .mode =0),
    STATSFS_STAT(vals, s16, .type=STATSFS_S16,
                .aggr_kind = STATSFS_NONE, .mode =0),
    { NULL },
};

struct statsfs_value test_values2[6] = {
    STATSFS_STAT(vals2, u64, .type=STATSFS_U64,
                .aggr_kind = STATSFS_NONE, .mode =0),
    STATSFS_STAT(vals2, s32, .type=STATSFS_S32,
                .aggr_kind = STATSFS_NONE, .mode =0),
    STATSFS_STAT(vals2, bo, .type=STATSFS_BOOL,
                .aggr_kind = STATSFS_NONE, .mode =0),
    STATSFS_STAT(vals2, u8, .type=STATSFS_U8,
                .aggr_kind = STATSFS_NONE, .mode =0),
    STATSFS_STAT(vals2, s16, .type=STATSFS_S16,
                .aggr_kind = STATSFS_NONE, .mode =0),
    { NULL },
};

struct statsfs_value test_aggr[4] = {
    STATSFS_STAT(vals, s32, .type=STATSFS_S32,
                .aggr_kind = STATSFS_MIN, .mode =0),
    STATSFS_STAT(vals, bo, .type=STATSFS_BOOL,
                .aggr_kind = STATSFS_MAX, .mode =0),
    STATSFS_STAT(vals, u64, .type=STATSFS_U64,
                .aggr_kind = STATSFS_SUM, .mode =0),
    { NULL },
};


struct statsfs_value test_same_name[3] = {
    STATSFS_STAT(vals, s32, .type=STATSFS_S32,
                .aggr_kind = STATSFS_NONE, .mode =0),
    STATSFS_STAT(vals, s32, .type=STATSFS_S32,
                .aggr_kind = STATSFS_MIN, .mode =0),
    { NULL },
};

struct statsfs_value test_all_aggr[6] = {
    STATSFS_STAT(vals, s32, .type=STATSFS_S32,
                .aggr_kind = STATSFS_MIN, .mode =0),
    STATSFS_STAT(vals, bo, .type=STATSFS_BOOL,
                .aggr_kind = STATSFS_COUNT_ZERO, .mode =0),
    STATSFS_STAT(vals, u64, .type=STATSFS_U64,
                .aggr_kind = STATSFS_SUM, .mode =0),
    STATSFS_STAT(vals, u8, .type=STATSFS_U8,
                .aggr_kind = STATSFS_AVG, .mode =0),
    STATSFS_STAT(vals, s16, .type=STATSFS_S16,
                .aggr_kind = STATSFS_MAX, .mode =0),
    { NULL },
};

#define def_u64 64

#define def_val_s32 INT32_MIN
#define def_val_bool true
#define def_val_u8 120
#define def_val_s16 10000

#define def_val2_s32 INT16_MAX
#define def_val2_bool false
#define def_val2_u8 5
#define def_val2_s16 -20000


struct container cont = {
    .vals = {
        .u64 = def_u64,
        .s32 = def_val_s32,
        .bo = def_val_bool,
        .u8 = def_val_u8,
        .s16 = def_val_s16,
    },
    .vals2 = {
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
    n = statsfs_source_add_values(src, test_values, &cont);
    assert(n == ARR_SIZE(test_values));

    n = statsfs_source_add_values(src, test_values, &cont);
    assert(n == 0);
    // here same array given, so don't add TODO: intented behavior?
    assert(get_number_values(src) ==  ARR_SIZE(test_values));


    n = statsfs_source_add_aggregate(src, test_values);
    assert(n == 0);
    assert(get_number_values(src) ==  ARR_SIZE(test_values));
    assert(get_number_aggregates(src) == 0);
    statsfs_source_destroy(src);
}

static void test_add_value_in_subfolder()
{
    struct statsfs_source *src, *sub, *sub_not;
    int n;

    src = statsfs_source_create("parent");
    sub = statsfs_source_create("child");
    statsfs_source_add_subordinate(src, sub);

    n = statsfs_source_add_values(sub, test_values, &cont);
    assert(n == ARR_SIZE(test_values));
    assert(get_number_values(src) ==  0);
    assert(get_number_aggregates(src) == 0);
    assert(get_total_number_values(src) == ARR_SIZE(test_values));

    assert(get_number_values(sub) ==  ARR_SIZE(test_values));
    assert(get_number_aggregates(sub) == 0);

    sub_not = statsfs_source_create("not a child");

    n = statsfs_source_add_values(sub_not, test_values, &cont);
    assert(n == ARR_SIZE(test_values));
    assert(get_number_values(src) ==  0);
    assert(get_number_aggregates(src) == 0);
    assert(get_total_number_values(src) == ARR_SIZE(test_values));

    statsfs_source_remove_subordinate(src, sub);
    assert(get_total_number_values(src) == 0);

    statsfs_source_add_subordinate(src, sub);
    assert(get_total_number_values(src) == ARR_SIZE(test_values));
    statsfs_source_add_subordinate(src, sub_not);
    assert(get_total_number_values(src) == ARR_SIZE(test_values) * 2);

    assert(get_number_values(sub_not) ==  ARR_SIZE(test_values));
    assert(get_number_aggregates(sub_not) == 0);

    statsfs_source_destroy(src);
}

static void test_search_value()
{
    struct statsfs_source *src;
    uint64_t ret;
    int n;

    src = statsfs_source_create("parent");
    n = statsfs_source_add_values(src, test_values, &cont);
    assert(n == ARR_SIZE(test_values));
    n = statsfs_source_get_value_by_name(src, "u64", &ret);
    assert(ret == def_u64 && n == 0);

    n = statsfs_source_get_value_by_name(src, "s32", &ret);
    assert(((int32_t) ret) == def_val_s32 && n == 0);

    n = statsfs_source_get_value_by_name(src, "bo", &ret);
    assert(((bool) ret) == def_val_bool && n == 0);

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
    statsfs_source_add_subordinate(src, sub);
    n = statsfs_source_add_values(sub, test_values, &cont);
    assert(n == ARR_SIZE(test_values));

    n = statsfs_source_get_value_by_name(sub, "u64", &ret);
    assert(ret == def_u64 && n == 0);
    n = statsfs_source_get_value_by_name(src, "u64", &ret);
    assert(ret == 0 && n == -ENOENT);

    n = statsfs_source_get_value_by_name(sub, "s32", &ret);
    assert(((int32_t) ret) == def_val_s32 && n == 0);
    n = statsfs_source_get_value_by_name(src, "s32", &ret);
    assert(ret == 0 && n == -ENOENT);

    n = statsfs_source_get_value_by_name(sub, "bo", &ret);
    assert(((bool) ret) == def_val_bool && n == 0);
    n = statsfs_source_get_value_by_name(src, "bo", &ret);
    assert(ret == 0 && n == -ENOENT);

    n = statsfs_source_get_value_by_name(sub, "does not exist", &ret);
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
    n = statsfs_source_add_values(src, test_aggr, &cont);
    assert(n == 0);
    n = statsfs_source_add_aggregate(src, test_aggr);
    assert(n == ARR_SIZE(test_aggr));
    n = statsfs_source_add_aggregate(src, test_aggr);
    assert(n == ARR_SIZE(test_aggr));

    assert(get_number_values(src) ==  0);
    // TODO: intended behavior?
    assert(get_number_aggregates(src) == ARR_SIZE(test_aggr) * 2);
    statsfs_source_destroy(src);
}

static void test_add_aggregate_in_subfolder()
{
    struct statsfs_source *src, *sub, *sub_not;
    int n;

    src = statsfs_source_create("parent");
    sub = statsfs_source_create("child");
    statsfs_source_add_subordinate(src, sub);

    n = statsfs_source_add_aggregate(sub, test_aggr);
    assert(n == ARR_SIZE(test_aggr));
    assert(get_number_values(src) ==  0);
    assert(get_number_aggregates(src) == 0);
    assert(get_total_number_values(src) == 0);

    assert(get_number_values(sub) ==  0);
    assert(get_number_aggregates(sub) == ARR_SIZE(test_aggr));

    sub_not = statsfs_source_create("not a child");

    n = statsfs_source_add_aggregate(sub_not, test_aggr);
    assert(n == ARR_SIZE(test_aggr));
    assert(get_number_values(src) ==  0);
    assert(get_number_aggregates(src) == 0);
    assert(get_total_number_values(src) == 0);

    statsfs_source_remove_subordinate(src, sub);
    assert(get_total_number_values(src) == 0);

    statsfs_source_add_subordinate(src, sub);
    assert(get_total_number_values(src) == 0);
    statsfs_source_add_subordinate(src, sub_not);
    assert(get_total_number_values(src) == 0);

    assert(get_number_values(sub_not) ==  0);
    assert(get_number_aggregates(sub_not) == ARR_SIZE(test_aggr));

    statsfs_source_destroy(src);
}


static void test_search_aggregate()
{
    struct statsfs_source *src;
    uint64_t ret;
    int n;

    src = statsfs_source_create("parent");
    n = statsfs_source_add_aggregate(src, test_aggr);
    assert(n == ARR_SIZE(test_aggr));
    n = statsfs_source_get_value_by_name(src, "u64", &ret);
    assert(ret == 0 && n == 0);

    n = statsfs_source_get_value_by_name(src, "s32", &ret);
    // it's a min so nothing found, return INT_MAX TODO: intended behav?
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
    n = statsfs_source_add_aggregate(sub, test_aggr);
    assert(n == ARR_SIZE(test_aggr));

    // no u64 in test_aggr
    n = statsfs_source_get_value_by_name(sub, "u64", &ret);
    assert(ret == 0 && n == 0);
    n = statsfs_source_get_value_by_name(src, "u64", &ret);
    assert(ret == 0 && n == -ENOENT);

    n = statsfs_source_get_value_by_name(sub, "s32", &ret);
    assert(ret == INT64_MAX && n == 0); //TODO: same as prev test
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
    assert(n == 1);

    n = statsfs_source_add_aggregate(src, test_same_name);
    assert(n == 1);

    // returns first the value
    n = statsfs_source_get_value_by_name(src, "s32", &ret);
    assert(((int32_t) ret) == def_val_s32 && n == 0);

    statsfs_source_destroy(src);
}


static void test_add_mixed()
{
    struct statsfs_source *src;
    int n;

    src = statsfs_source_create("parent");
    n = statsfs_source_add_values(src, test_aggr, &cont);
    assert(n == 0);
    n = statsfs_source_add_aggregate(src, test_values);
    assert(n == 0);

    n = statsfs_source_add_values(src, test_values, &cont);
    assert(n == ARR_SIZE(test_values));
    n = statsfs_source_add_aggregate(src, test_aggr);
    assert(n == ARR_SIZE(test_aggr));

    assert(get_number_values(src) ==  ARR_SIZE(test_values));
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
    assert(n == ARR_SIZE(test_values));
    n = statsfs_source_add_aggregate(src, test_aggr);
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

static void test_all_aggregations()
{
    struct statsfs_source *src, *sub1, *sub2;
    uint64_t ret;
    int n;

    src = statsfs_source_create("parent");
    sub1 = statsfs_source_create("child1");
    sub2 = statsfs_source_create("child2");
    statsfs_source_add_subordinate(src, sub1);
    statsfs_source_add_subordinate(src, sub2);

    n = statsfs_source_add_values(sub1, test_values, &cont);
    assert(n == ARR_SIZE(test_values));
    n = statsfs_source_add_values(sub2, test_values2, &cont);
    assert(n == ARR_SIZE(test_values));

    assert(get_total_number_values(src) == ARR_SIZE(test_values)*2);

    n = statsfs_source_add_aggregate(src, test_all_aggr);
    assert(n == ARR_SIZE(test_all_aggr));

    // char *values[] = {"u64", "s32", "bo", "u8", "s16"};
    // uint64_t expected_res[] = {
    //     def_u64*2, def_val_s32, 1, 63, def_val_s16
    // };

    // sum
    n = statsfs_source_get_value_by_name(src, "u64", &ret);
    assert(n == 0 && ret == def_u64*2);

    // min
    n = statsfs_source_get_value_by_name(src, "s32", &ret);
                // printf("%d %ld %d\n", n, ret, INT32_MIN);
    assert(n == 0 && ((int32_t) ret) == def_val_s32);

    // count_0
    n = statsfs_source_get_value_by_name(src, "bo", &ret);
            // printf("%d %ld\n", n, ret);
    assert(n == 0 && ret == 1);

    // avg
    n = statsfs_source_get_value_by_name(src, "u8", &ret);
            // printf("%d %ld\n", n, ret);
    assert(n == 0 && ret == 62);

    // max
    n = statsfs_source_get_value_by_name(src, "s16", &ret);
    assert(n == 0 && ret == def_val_s16);

    statsfs_source_destroy(src);

}

// todo test all aggregation operations on:
// - source (agg) sub1 (val) sub2 (val)
// - source (val) sub1 (agg) sub2 (val)
// - source (agg) sub1 (val) sub11 (val)
// - source (agg) sub1 () sub11 (val)
int main(int argc, char *argv[]) {
    test_empty_folder();
    test_add_subfolder();
    test_add_value();
    test_add_value_in_subfolder();
    test_search_value();
    test_search_value_in_subfolder();
    test_add_aggregate();
    test_add_aggregate_in_subfolder();
    test_search_aggregate();
    test_search_aggregate_in_subfolder();
    test_search_same();
    test_add_mixed();
    test_search_mixed();
    test_all_aggregations();
    printf("Test completed\n");
}