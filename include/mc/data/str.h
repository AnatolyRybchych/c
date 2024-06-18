#ifndef MC_DATA_STR_H
#define MC_DATA_STR_H

typedef struct MC_Str MC_Str;

#define MC_STR_LEN(STR) ((STR).end - (STR).beg)
#define MC_STR(BEG, END) ((MC_Str){.beg = BEG, .end = END})
#define MC_STRC(CSTR) MC_STR("" CSTR, CSTR + sizeof(CSTR))

struct MC_Str{
    const char *beg;
    const char *end;
};

#endif // MC_DATA_STR_H
