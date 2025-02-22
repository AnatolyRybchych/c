#include <mc/data/hmap.h>
#include <mc/data/list.h>
#include <mc/util/error.h>
#include <mc/util/assert.h>

#include <memory.h>

#define INITIAL_CAPACITY 16

struct MC_HMapBucket {
    MC_HMapBucket *next;
    void *value;
    size_t key_size;
    uint8_t key[];
};

struct MC_HMap {
    MC_Alloc *alloc;
    size_t items_count;
    size_t generation;
    size_t buckets_cnt;
    MC_HMapBucket **buckets;
};

extern inline void **mc_hmap_get_s(MC_HMap *hmap, MC_Str key);
extern inline void **mc_hmap_get_c(MC_HMap *hmap, const char *key);
extern inline void *mc_hmap_get_value_s(MC_HMap *hmap, MC_Str key);
extern inline void *mc_hmap_get_value_c(MC_HMap *hmap, const char *key);
extern inline MC_Error mc_hmap_set_s(MC_HMap *hmap, MC_Str key, void *value);
extern inline MC_Error mc_hmap_setc(MC_HMap *hmap, const char *key, void *value);
extern inline MC_Error mc_hmap_dels(MC_HMap *hmap, MC_Str key);
extern inline MC_Error mc_hmap_delc(MC_HMap *hmap, const char *key);

static size_t get_hash(size_t size, const void *data);
static MC_Error alloc_bucket(MC_Alloc *alloc, MC_HMapBucket **bucket, size_t key_size, const void *key, void *value);
static MC_HMapBucket **get_bucket_location(MC_HMap *hmap, size_t key_size, const void *key);
static bool is_bucket_of(MC_HMapBucket *bucket, size_t key_size, const void *key);
static MC_Error hmap_new(MC_Alloc *alloc, MC_HMap **hmap, size_t capacity);
static MC_Error rearrange(MC_HMap *hmap, size_t new_size);

MC_Error mc_hmap_new(MC_Alloc *alloc, MC_HMap **hmap) {
    return hmap_new(alloc, hmap, INITIAL_CAPACITY);
}

void mc_hmap_delete(MC_HMap *hmap) {
    for(size_t i = 0; i != hmap->buckets_cnt; i++) {
        while(!mc_list_empty(hmap->buckets[i])) {
            mc_free(hmap->alloc, mc_list_remove(&hmap->buckets[i]));
        }
    }

    mc_free(hmap->alloc, hmap->buckets);
    mc_free(hmap->alloc, hmap);
}

MC_Error mc_hmap_set(MC_HMap *hmap, size_t key_size, const void *key, void *value) {
    if(hmap->items_count > hmap->buckets_cnt) {
        MC_RETURN_ERROR(rearrange(hmap, hmap->items_count * 2));
    }

    MC_HMapBucket **bucket_location = get_bucket_location(hmap, key_size, key);
    MC_LIST_FOR(MC_HMapBucket, *bucket_location, bucket) {
        if(is_bucket_of(bucket, key_size, key)) {
            bucket->value = value;
            return MCE_OK;
        }
    }

    MC_HMapBucket *new_bucket;
    MC_RETURN_ERROR(alloc_bucket(hmap->alloc, &new_bucket, key_size, key, value));
    mc_list_add(bucket_location, new_bucket);
    hmap->items_count++;
    return MCE_OK;
}

MC_Error mc_hmap_del(MC_HMap *hmap, size_t key_size, const void *key) {
    MC_HMapBucket **bucket_location = get_bucket_location(hmap, key_size, key);
    if(bucket_location == NULL) {
        return MCE_NOT_FOUND;
    }

    mc_free(hmap->alloc, mc_list_remove(bucket_location));
    return MCE_OK;
}

void **mc_hmap_get(MC_HMap *hmap, size_t key_size, const void *key) {
    MC_HMapBucket **bucket_location = get_bucket_location(hmap, key_size, key);

    MC_LIST_FOR(MC_HMapBucket, *bucket_location, bucket) {
        if(is_bucket_of(bucket, key_size, key)) {
            return &bucket->value;
        }
    }

    return NULL;
}

void *mc_hmap_get_value(MC_HMap *hmap, size_t key_size, const void *key) {
    void **value_ptr = mc_hmap_get(hmap, key_size, key);
    return value_ptr ? *value_ptr : NULL;
}

size_t mc_hmap_size(MC_HMap *hmap) {
    return hmap->items_count;
}

MC_HMapIterator mc_hmap_iter(MC_HMap *hmap) {
    return (MC_HMapIterator) {
        .hmap = hmap,
        .cur_bucket = hmap->buckets,
        .cur_item = *hmap->buckets,
        .generation = hmap->generation,
    };
}

bool mc_hmap_iter_next(MC_HMapIterator *iter) {
    if(iter->generation != iter->hmap->generation) {
        return false;
    }

    while(iter->cur_bucket < iter->hmap->buckets + iter->hmap->buckets_cnt || iter->cur_item) {
        while(iter->cur_item){
            MC_HMapBucket *bucket = iter->cur_item;
            iter->key = MC_STR((const char*)bucket->key, (const char*)bucket->key + bucket->key_size);
            iter->value = bucket->value;
            iter->cur_item = iter->cur_item->next;
            return true;
        }

        if(++iter->cur_bucket < iter->hmap->buckets + iter->hmap->buckets_cnt) {
            iter->cur_item = *iter->cur_bucket;
        }
    }

    return false;
}

static size_t get_hash(size_t size, const void *data) {
    const uint8_t *bytes = (const uint8_t *)data;
    size_t hash = 0xcbf29ce484222325ULL;
    size_t prime = 0x100000001b3ULL;

    for (size_t i = 0; i < size; i++) {
        hash ^= bytes[i];
        hash *= prime;
    }

    hash ^= hash >> 33;
    hash *= 0xff51afd7ed558ccdULL;
    hash ^= hash >> 33;
    hash *= 0xc4ceb9fe1a85ec53ULL;
    hash ^= hash >> 33;

    return hash;
}

static MC_Error alloc_bucket(MC_Alloc *alloc, MC_HMapBucket **bucket, size_t key_size, const void *key, void *value) {
    MC_HMapBucket *new;
    MC_RETURN_ERROR(mc_alloc(alloc, sizeof *new + key_size, (void**)&new));
    *new = (MC_HMapBucket) {
        .key_size = key_size,
        .value = value
    };

    memcpy(new->key, key, key_size);
    *bucket = new;
    return MCE_OK;
}

static MC_HMapBucket **get_bucket_location(MC_HMap *hmap, size_t key_size, const void *key) {
    size_t hash = get_hash(key_size, key);
    size_t idx = hash % hmap->buckets_cnt;

    return &hmap->buckets[idx];
}

static bool is_bucket_of(MC_HMapBucket *bucket, size_t key_size, const void *key) {
    return key_size == bucket->key_size
        && memcmp(key, bucket->key, key_size) == 0;
}

static MC_Error hmap_new(MC_Alloc *alloc, MC_HMap **hmap, size_t capacity) {
    *hmap = NULL;

    MC_HMap *new;
    MC_HMapBucket **buckets;
    MC_RETURN_ERROR(mc_alloc_all(alloc,
        &new, sizeof *new,
        &buckets, sizeof(MC_HMapBucket*[capacity]),
        NULL));

    *new = (MC_HMap) {
        .alloc = alloc,
        .buckets_cnt = capacity,
        .buckets = buckets,
    };

    *hmap = new;
    return MCE_OK;
}

static MC_Error rearrange(MC_HMap *hmap, size_t new_size) {
    MC_HMapBucket **new_buckets;
    MC_RETURN_ERROR(mc_alloc(hmap->alloc, sizeof(MC_HMapBucket*[new_size]), (void**)&new_buckets));
    memset(new_buckets, 0, sizeof(MC_HMapBucket*[new_size]));

    for(MC_HMapBucket **b = hmap->buckets; b != hmap->buckets + hmap->buckets_cnt; b++) {
        for(MC_HMapBucket *bucket; (bucket = mc_list_remove(b));) {
            size_t hash = get_hash(bucket->key_size, bucket->key);
            size_t new_idx = hash % new_size;
            mc_list_add(&new_buckets[new_idx], bucket);
        }
    }

    mc_free(hmap->alloc, hmap->buckets);
    hmap->generation++;
    hmap->buckets = new_buckets;
    hmap->buckets_cnt = new_size;

    return MCE_OK;
}
