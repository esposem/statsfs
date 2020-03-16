#ifndef _TEST_HELPERS_H
#define _TEST_HELPERS_H

int source_has_subsource(struct statsfs_source *src,
			 struct statsfs_source *sub);

int get_number_subsources(struct statsfs_source *src);
int get_number_values(struct statsfs_source *src);
int get_total_number_values(struct statsfs_source *src);
int get_number_aggregates(struct statsfs_source *src);
int get_number_values_with_base(struct statsfs_source *src, void *addr);
int get_number_aggr_with_base(struct statsfs_source *src, void *addr);

#endif
