#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>
#include "statsfs.h"
#include "list.h"

struct kvm_vcpu_stat1 {
	uint64_t sum_aggr_u64;
    uint32_t min_aggr_u32;
    uint16_t simple_1_u32;
    uint8_t max_agg_u8;
    uint16_t avg_agg_u16;
    char *simple_2;
};

struct kvm_vcpu_stat2 {
	uint64_t sum_aggr_u64;
    uint32_t min_aggr_u32;
    uint8_t max_agg_u8;
    uint16_t avg_agg_u16;
    uint16_t simple_2;
};

struct kvm_vcpu_stat3 {
	uint64_t sum_aggr_u64;
    uint32_t min_aggr_u32;
    uint8_t max_agg_u8;
    uint16_t avg_agg_u16;
    uint16_t simple_1_u16;
    uint8_t simple_1_u8;
    bool simple_1_bool;
    char *simple_1_str;
};

struct kvm {
    // complicated things
    struct kvm_vcpu_stat1 stat1;
    struct kvm_vcpu_stat2 stat2;
    struct kvm_vcpu_stat3 stat3;
    // more complicated things
};

#define STATSFS_STAT1(x, ...) { .name= #x, .offset= offsetof(struct kvm, stat1.x), ## __VA_ARGS__ }
#define STATSFS_STAT2(x, ...) { .name= #x, .offset= offsetof(struct kvm, stat2.x), ## __VA_ARGS__ }
#define STATSFS_STAT3(x, ...) { .name= #x, .offset= offsetof(struct kvm, stat3.x), ## __VA_ARGS__ }
#define STATSFS_STAT_AGGR(x, ...) { .name= #x, .offset= 0, ## __VA_ARGS__ }


// 3 aggr, 2 val
struct statsfs_value values1[6] = {
    STATSFS_STAT_AGGR(sum_aggr_u64, .type= U64, .aggr_kind= SUM, .mode = 0),
    STATSFS_STAT_AGGR(min_aggr_u32, .type= U32, .aggr_kind= MIN, .mode = 0),
    STATSFS_STAT_AGGR(bool_aggr_sum, .type=BOOL, .aggr_kind= SUM, .mode = 0),
    STATSFS_STAT1(simple_1_u32, .type=U32),
    STATSFS_STAT1(max_agg_u8, .type=U8),
    {NULL}
};

// 5 val, 3 aggr
struct statsfs_value values2[9] = {
    STATSFS_STAT_AGGR(max_agg_u8, .type=U8, .aggr_kind= MAX, .mode = 0),
    STATSFS_STAT_AGGR(avg_agg_u16, .type=U16, .aggr_kind= AVG, .mode = 0),
    STATSFS_STAT2(simple_2, .type=U16),
    STATSFS_STAT_AGGR(bool_aggr_avg, .type=BOOL, .aggr_kind= AVG, .mode = 0),
    STATSFS_STAT2(min_aggr_u32, .type= U32),
    STATSFS_STAT2(sum_aggr_u64, .type=U64),
    STATSFS_STAT2(max_agg_u8, .type=U8),
    STATSFS_STAT2(avg_agg_u16, .type=U16),
    {NULL}
};

// 5 val
struct statsfs_value values3[6] = {
    STATSFS_STAT3(simple_1_u16, .type=U16),
    STATSFS_STAT3(simple_1_u8, .type=U8),
    STATSFS_STAT3(simple_1_bool, .type=BOOL),
    STATSFS_STAT3(sum_aggr_u64, .type=U64),
    STATSFS_STAT3(max_agg_u8, .type=U8),
    {NULL}
};

// 4 val, 1 aggr
struct statsfs_value values4[6] = {
    STATSFS_STAT1(sum_aggr_u64, .type=U64),
    STATSFS_STAT_AGGR(bool_aggr_sum, .type=BOOL, .aggr_kind= SUM, .mode = 0),
    STATSFS_STAT1(min_aggr_u32, .type= U32),
    STATSFS_STAT1(max_agg_u8, .type=U8), // This is a dup!
    STATSFS_STAT1(avg_agg_u16, .type=U16),
    {NULL}
};

// 2 val
struct statsfs_value values5[3] = {
    STATSFS_STAT3(min_aggr_u32, .type= U32),
    STATSFS_STAT3(avg_agg_u16, .type=U16),
    {NULL}
};

int main(int argc, char *argv[]){
    struct kvm k = {
        .stat1 = {
            .sum_aggr_u64 = 23,
            .min_aggr_u32 = 3200,
            .simple_1_u32 = 32,
            .max_agg_u8 = 255,
            .avg_agg_u16 = 1600,
        },
        .stat2 = {
            // .sum_aggr_u64 = 43,
            .min_aggr_u32 = 0,
            .max_agg_u8 = 80,
            .avg_agg_u16 = 16,
            .simple_2 = 4096,
        },
        .stat3 = {
            .sum_aggr_u64 = 5554,
            .min_aggr_u32 = 10,
            .max_agg_u8 = 1,
            .avg_agg_u16 = 610,
            .simple_1_u16 = 65535,
            .simple_1_u8 = 200,
            .simple_1_bool = true,
        },
    };

    /*
    Directory structure:

    kvm_main (val values2)
        kvm_1 (agg values1 val values5)
            kvm_1_1
            kvm_1_2 (agg values2 agg values4)

        kvm_2
            kvm_2_1
                kvm_2_1_1
                    kvm_2_1_1_1 (agg values3 val values4)

            kvm_2_2 (agg values2 val values3)

            kvm_2_3 (val values3 val values1)
    */
    struct statsfs_source *kvm_main= statsfs_source_create("kvm_main");

    struct statsfs_source *kvm_1= statsfs_source_create("kvm_1");
    statsfs_source_add_subordinate(kvm_main, kvm_1);

    struct statsfs_source *kvm_2= statsfs_source_create("kvm_2");
    statsfs_source_add_subordinate(kvm_main, kvm_2);

    struct statsfs_source *kvm_1_1= statsfs_source_create("kvm_1_1");
    statsfs_source_add_subordinate(kvm_1, kvm_1_1);
    struct statsfs_source *kvm_1_2= statsfs_source_create("kvm_1_2");
    statsfs_source_add_subordinate(kvm_1, kvm_1_2);

    struct statsfs_source *kvm_2_1= statsfs_source_create("kvm_2_1");
    statsfs_source_add_subordinate(kvm_2, kvm_2_1);
    struct statsfs_source *kvm_2_2= statsfs_source_create("kvm_2_2");
    statsfs_source_add_subordinate(kvm_2, kvm_2_2);
    struct statsfs_source *kvm_2_3= statsfs_source_create("kvm_2_3");
    statsfs_source_add_subordinate(kvm_2, kvm_2_3);

    struct statsfs_source *kvm_2_1_1= statsfs_source_create("kvm_2_1_1");
    statsfs_source_add_subordinate(kvm_2_1, kvm_2_1_1);

    struct statsfs_source *kvm_2_1_1_1= statsfs_source_create("kvm_2_1_1_1");
    statsfs_source_add_subordinate(kvm_2_1_1, kvm_2_1_1_1);

    // statsfs_source_destroy

    int ret = 0;
    ret = statsfs_source_add_values(kvm_main, values2, &k.stat1);
    assert(ret == 5);

    ret = statsfs_source_add_values(kvm_1, values5, &k.stat3);
    assert(ret == 2);
    ret = statsfs_source_add_aggregate(kvm_1, values1);
    assert(ret == 3);

    ret = statsfs_source_add_aggregate(kvm_1_2, values2);
    assert(ret == 3);
    ret = statsfs_source_add_aggregate(kvm_1_2, values4);
    assert(ret == 1);

    ret = statsfs_source_add_aggregate(kvm_2_1_1_1, values3);
    assert(ret == 0);
    ret = statsfs_source_add_values(kvm_2_1_1_1, values4, &k.stat1);
    assert(ret == 4);

    ret = statsfs_source_add_aggregate(kvm_2_2, values2);
    assert(ret == 3);
    ret = statsfs_source_add_values(kvm_2_2, values3, &k.stat3);
    assert(ret == 5);

    ret = statsfs_source_add_values(kvm_2_3, values3, &k.stat3);
    assert(ret == 5);
    ret = statsfs_source_add_values(kvm_2_3, values1, &k.stat1);
    assert(ret == 2);

    uint64_t res;
    statsfs_source_get_value_by_name(kvm_main, "sum_aggr_u64", &res);
    printf("Res %ld\n", res);
    // assert(23 + 43 + 5554 == res);
    // statsfs_source_get_value_by_val

    // find then remove and then refind
    // statsfs_source_remove_subordinate

}