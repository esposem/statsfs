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
    int64_t s64;
    bool bo;
};

struct container {
    struct test_values vals;
};

struct statsfs_value test_values[4] = {
    STATSFS_STAT(vals, u64, .type=STATSFS_U64,
                .aggr_kind = STATSFS_NONE, .mode =0),
    STATSFS_STAT(vals, s64, .type=STATSFS_S64,
                .aggr_kind = STATSFS_NONE, .mode =0),
    STATSFS_STAT(vals, bo, .type=STATSFS_BOOL,
                .aggr_kind = STATSFS_NONE, .mode =0),
    { NULL },
};

struct statsfs_value test_aggr[3] = {
    STATSFS_STAT(vals, s64, .type=STATSFS_S64,
                .aggr_kind = STATSFS_MIN, .mode =0),
    STATSFS_STAT(vals, bo, .type=STATSFS_BOOL,
                .aggr_kind = STATSFS_MAX, .mode =0),
    { NULL },
};

// TODO:
struct statsfs_value test_mixed[6] = {
    STATSFS_STAT(vals, s64, .type=STATSFS_S64,
                .aggr_kind = STATSFS_MIN, .mode =0),
    STATSFS_STAT(vals, bo, .type=STATSFS_BOOL,
                .aggr_kind = STATSFS_MAX, .mode =0),
    STATSFS_STAT(vals, u64, .type=STATSFS_U64,
                .aggr_kind = STATSFS_NONE, .mode =0),
    STATSFS_STAT(vals, s64, .type=STATSFS_S64,
                .aggr_kind = STATSFS_NONE, .mode =0),
    STATSFS_STAT(vals, bo, .type=STATSFS_BOOL,
                .aggr_kind = STATSFS_NONE, .mode =0),
    { NULL },
};

struct container cont = {
    .vals = {
        .u64 = 64,
        .s64 = -64,
        .bo = true,
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
    assert(ret == 64 && n == 0);

    n = statsfs_source_get_value_by_name(src, "s64", &ret);
    assert(((int64_t) ret) == -64 && n == 0);

    n = statsfs_source_get_value_by_name(src, "bo", &ret);
    assert(ret == true && n == 0);

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
    n = statsfs_source_add_values(sub, test_values, &cont);
    assert(n == ARR_SIZE(test_values));

    n = statsfs_source_get_value_by_name(sub, "u64", &ret);
    assert(ret == 64 && n == 0);
    n = statsfs_source_get_value_by_name(src, "u64", &ret);
    assert(ret == 0 && n == -ENOENT);

    n = statsfs_source_get_value_by_name(sub, "s64", &ret);
    assert(((int64_t) ret) == -64 && n == 0);
    n = statsfs_source_get_value_by_name(src, "s64", &ret);
    assert(ret == 0 && n == -ENOENT);

    n = statsfs_source_get_value_by_name(sub, "bo", &ret);
    assert(ret == true && n == 0);
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
    // here same array given, so do not add TODO: intented behavior?
    n = statsfs_source_add_aggregate(src, test_aggr);
    assert(n == ARR_SIZE(test_aggr));

    assert(get_number_values(src) ==  0);
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
    assert(ret == 0 && n == -ENOENT);

    n = statsfs_source_get_value_by_name(src, "s64", &ret);
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
    n = statsfs_source_add_aggregate(sub, test_aggr);
    assert(n == ARR_SIZE(test_aggr));

    // no u64 in test_aggr
    n = statsfs_source_get_value_by_name(sub, "u64", &ret);
    assert(ret == 0 && n == -ENOENT);
    n = statsfs_source_get_value_by_name(src, "u64", &ret);
    assert(ret == 0 && n == -ENOENT);

    n = statsfs_source_get_value_by_name(sub, "s64", &ret);
    assert(ret == INT64_MAX && n == 0); //TODO: same as prev test
    n = statsfs_source_get_value_by_name(src, "s64", &ret);
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
}