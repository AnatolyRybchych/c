#include <mc/os/file.h>
#include <mc/util/assert.h>
#include <mc/data/str.h>

#define L(...) (mc_fmt(MC_STDOUT, "%s\n", #__VA_ARGS__), __VA_ARGS__)
#define LR(...) MC_REQUIRE(L(__VA_ARGS__))
#define DELIM() mc_fmt(MC_STDOUT, "================================\n\n");
#define NL() mc_fmt(MC_STDOUT, "\n");

#define DUMP(STR) mc_fmt(MC_STDOUT, #STR ": %.*s\n", (STR).end - (STR).beg, (STR).beg)

int main(){
    MC_Str match, oct1, oct2, oct3, oct4;
    MC_Str untouched = MC_STRC("UNTOUCHED");
    bool matches = L(mc_str_match(MC_STRC("    192.168.1.1   "), "(%d+)%.(%d+)%.(%d+)%.(%d+)",
        &match, &oct1, &oct2, &oct3, &oct4, &untouched));

    mc_fmt(MC_STDOUT, "mc_str_match -> %s\n", matches ? "true" : "false");
    DUMP(match);
    DUMP(oct1);
    DUMP(oct2);
    DUMP(oct3);
    DUMP(oct4);
    DUMP(untouched);
    DELIM();
}
