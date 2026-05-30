#ifndef MC_DATA_HMAP_H
#define MC_DATA_HMAP_H

#include <mc/error.h>
#include <mc/data/alloc.h>
#include <mc/data/str.h>

typedef struct MC_HMap MC_HMap;
typedef struct MC_HMapKV MC_HMapKV;
typedef struct MC_HMapBucket MC_HMapBucket;
typedef struct MC_HMapIterator MC_HMapIterator;

MC_Error mc_hmap_new(MC_Alloc *alloc, MC_HMap **hmap);
void mc_hmap_delete(MC_HMap *hmap);

MC_Error mc_hmap_set(MC_HMap *hmap, MC_Str key, void *value);
MC_Error mc_hmap_del(MC_HMap *hmap, MC_Str key);
MC_HMapBucket *mc_hmap_get_or_new(MC_HMap *hmap, MC_Str key);
MC_HMapBucket *mc_hmap_get(MC_HMap *hmap, MC_Str key);
void *mc_hmap_get_value(MC_HMap *hmap, MC_Str key);
size_t mc_hmap_size(MC_HMap *hmap);

MC_HMapIterator mc_hmap_iter(MC_HMap *hmap);
bool mc_hmap_iter_next(MC_HMapIterator *iter);

struct MC_HMapIterator {
    MC_HMap *hmap;
    MC_HMapBucket **cur_bucket;
    MC_HMapBucket *cur_item;
    const size_t generation;
    MC_Str key;
    void *value;
};

struct MC_HMapBucket {
    MC_HMapBucket *next;
    void *value;
    size_t key_size;
    const char key[];
};

inline void *mc_hmap_get_value_c(MC_HMap *hmap, const char *key) {
    return mc_hmap_get_value(hmap, mc_strc(key));
}

inline MC_Error mc_hmap_setc(MC_HMap *hmap, const char *key, void *value) {
    return mc_hmap_set(hmap, mc_strc(key), value);
}

inline MC_Error mc_hmap_delc(MC_HMap *hmap, const char *key) {
    return mc_hmap_del(hmap, mc_strc(key));
}

#endif // MC_DATA_HMAP_H
