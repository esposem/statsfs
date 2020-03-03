#ifndef _STATSFS_H

#define _STATSFS_H

struct statsfs_value;
struct statsfs_source;

void statsfs_subordinates_debug_list(struct statsfs_source *parent);
void statsfs_values_debug_list(struct statsfs_source *parent);

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



// 1) guarda i values “foglia” (aggiunti con statsfs_source_add_values)
// 2) se non c’è un valore foglia con quel nome, cerca tra i valori da aggregare (aggiunti con
// statsfs_source_add_aggregate). Se c’è lì, itera su tutte le sorgenti subordinate e calcola
// minimo/massimo/media/totale/quello che serve
// 3) se non c’è nemmeno lì, ritorna -ENOENT.
int statsfs_source_get_value(
                struct statsfs_source *source,
                struct statsfs_value *val, void **ret);


// come sopra, ma per nome
// per ora basta una ricerca con for + chiamata a statsfs_source_get_value, poi vediamo
int statsfs_source_get_value_by_name(
                struct statsfs_source *source,
                const char *name, void **val);

/* TODO:
// vedi kref_get e kref_put in include/linux/kref.h
void statsfs_source_get(struct statsfs_source *);
void statsfs_source_put(struct statsfs_source *);

void statsfs_source_register(struct statsfs_source *source);
*/


#endif