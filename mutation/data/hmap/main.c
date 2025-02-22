#include <mc/os/file.h>
#include <mc/util/assert.h>
#include <mc/data/hmap.h>
#include <stdio.h>

#define L(...) (mc_fmt(MC_STDOUT, "%s\n", #__VA_ARGS__), __VA_ARGS__)
#define LR(...) MC_REQUIRE(L(__VA_ARGS__))
#define DELIM() mc_fmt(MC_STDOUT, "================================\n\n");
#define NL() mc_fmt(MC_STDOUT, "\n");

int main(){
    MC_HMap *hmap;
    LR(mc_hmap_new(NULL, &hmap));
    LR(mc_hmap_setc(hmap, "key1", "test1"));
    mc_fmt(MC_STDOUT, "%s: %s\n", "key1", mc_hmap_get_value_c(hmap, "key1"));

    LR(mc_hmap_setc(hmap, "key2", "test2"));
    mc_fmt(MC_STDOUT, "%s: %s\n", "key2", mc_hmap_get_value_c(hmap, "key2"));

    LR(mc_hmap_setc(hmap, "key3", "test3"));
    mc_fmt(MC_STDOUT, "%s: %s\n", "key3", mc_hmap_get_value_c(hmap, "key3"));

    LR(mc_hmap_setc(hmap, "key1", "test4"));
    mc_fmt(MC_STDOUT, "%s: %s\n", "key1", mc_hmap_get_value_c(hmap, "key1"));

    NL();
    DELIM();
    for(MC_HMapIterator iter = mc_hmap_iter(hmap); mc_hmap_iter_next(&iter);) {
        mc_fmt(MC_STDOUT, "%.*s: %s\n", mc_str_len(iter.key), iter.key.beg, (const char*)iter.value);
    }
    DELIM();

    for(int i = 0; i < 1000; i++) {
        char key[128];
        char *value = malloc(sizeof(128));
        sprintf(key, "key%d", i);
        sprintf(value, "value%d", i);

        mc_fmt(MC_STDOUT, "mc_hmap_setc(hmap, \"%s\", \"%s\")\n", key, value);
        mc_hmap_setc(hmap, key, value);
    }

    DELIM();
    for(MC_HMapIterator iter = mc_hmap_iter(hmap); mc_hmap_iter_next(&iter);) {
        mc_fmt(MC_STDOUT, "%.*s: %s\n", mc_str_len(iter.key), iter.key.beg, (const char*)iter.value);
    }
    DELIM();

    for(int i = 0; i < 1000; i++) {
        char key[128];
        sprintf(key, "key%d", i);
        char *value = mc_hmap_get_value_c(hmap, key);

        mc_fmt(MC_STDOUT, "mc_hmap_get_c(hmap, \"%s\") -> \"%s\"\n", key, value);
    }
    DELIM();

    mc_hmap_delete(hmap);
}
