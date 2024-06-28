#include <mc/data/list.h>

extern inline bool mc_list_empty(const void *list);
extern inline void mc_list_add(void *own_location, void *node);
extern inline void *mc_list_remove(void *node_in_owner);
