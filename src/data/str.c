#include <mc/data/str.h>
#include <mc/util/assert.h>
#include <mc/util/memory.h>
#include <threads.h>

#include <string.h>

#define ITER_WELL_KNOW_ATOMS(CB, ...) \
    CB(space,       's', MC_STRC("[ \r\n\t]"),   ##__VA_ARGS__) \
    CB(alpha,       'a', MC_STRC("[a-zA-Z]"),    ##__VA_ARGS__) \
    CB(letter,      'l', MC_STRC("[a-z]"),       ##__VA_ARGS__) \
    CB(word,        'w', MC_STRC("[a-zA-Z]"),       ##__VA_ARGS__) \
    CB(capital,     'u', MC_STRC("[A-Z]"),       ##__VA_ARGS__) \
    CB(digit,       'd', MC_STRC("[0-9]"),       ##__VA_ARGS__) \
    CB(hex,         'x', MC_STRC("[0-9a-fA-F]"), ##__VA_ARGS__) \
    CB(printable,   'g', MC_STRC("[\x21-\x7E]"), ##__VA_ARGS__) \
    CB(wildcard,    '.', MC_STRC("[\x00-\xff]"), ##__VA_ARGS__) \
    // CB(punct,       'p', MC_STRC("[]"),          ##__VA_ARGS__)
    // CB(control,     'c', MC_STRC("[]"),          ##__VA_ARGS__)

typedef uint8_t AtomType;
enum AtomType{
    ATOM_RANGE,
    ATOM_SEQUENCE,
};

typedef struct Atom Atom;
struct Atom {
    AtomType type;
    union{
        uint8_t range[32];
        MC_Str sequence;
    };
};

typedef struct Modifier Modifier;
struct Modifier {
    uint32_t min;
    uint32_t max;
    bool lazy;
};

typedef uint8_t MatchStatus;
enum MatchStatus{
    MATCH,
    NOT_MATCH,
    INVALID_EXPESSION,
};

extern inline MC_Str mc_strc(const char *str);
extern inline size_t mc_str_len(MC_Str str);
extern inline bool mc_str_starts_with(MC_Str str, MC_Str substr);
extern inline bool mc_str_ends_with(MC_Str str, MC_Str substr);
extern inline bool mc_str_empty(MC_Str str);
extern inline MC_Str mc_str_ltrim(MC_Str str);
extern inline MC_Str mc_str_rtrim(MC_Str str);
extern inline MC_Str mc_str_trim(MC_Str str);
extern inline MC_Str mc_str_toull(MC_Str str, uint64_t *val);
extern inline MC_Str mc_str_hex_toull(MC_Str str, uint64_t *val);
extern inline bool mc_str_equ(MC_Str str, MC_Str other);
extern inline MC_Str mc_str_substr(MC_Str str, MC_Str substr);
extern inline MC_Str mc_str_pop_front(MC_Str *str, size_t cnt);
extern inline MC_Str mc_str_pop_back(MC_Str *str, size_t cnt);

static bool get_well_known_atom(char esc, Atom *atom);
static void char_set_add_range(uint8_t set[8], uint8_t beg, uint8_t end);
static const char *read_range(MC_Str str, Atom *atom);
static const char *read_atom(MC_Str str, Atom *atom);
static const char *read_modifier(MC_Str str, Modifier *mod);
static const char *match_atom(const Atom *atom, MC_Str str);
static const char *match_atom_reverse(const Atom *atom, MC_Str str);
static MatchStatus matchv_from_beg(MC_Str str, MC_Str fmt, MC_Str *whole, va_list args);

static bool get_well_known_atom(char esc, Atom *atom) {
    #define DEFINE_ATOM_CACHE(NAME, ...) \
        static thread_local Atom _atom_##NAME, *atom_##NAME;
    ITER_WELL_KNOW_ATOMS(DEFINE_ATOM_CACHE)

    switch (esc){
    #define GET_ATOM_CACHED(NAME, ESC, RANGE, RESULT)   \
        case ESC:                                       \
            if(!atom_##NAME) {                          \
                atom_##NAME = &_atom_##NAME;            \
                read_range(RANGE, atom_##NAME);         \
            }                                           \
            (RESULT) = *atom_##NAME;                    \
            break;
        ITER_WELL_KNOW_ATOMS(GET_ATOM_CACHED, *atom);
    default:
        return false;
    }
    
    return true;
}

static void char_set_add_range(uint8_t set[8], uint8_t beg, uint8_t end) {
    for(int c = beg; c <= end; c++) {
        unsigned byte = c >> 3;
        unsigned bit = c & 0x7;
        set[byte] |= 1 << bit;
    }
}

static const char *read_range(MC_Str str, Atom *atom) {
    if(mc_str_empty(str) || *str.beg != '[') {
        return NULL;
    }

    MC_Str closing_bracket = mc_str_substr(str, MC_STRC("]"));
    if(mc_str_empty(closing_bracket)) {
        return NULL;
    }

    MC_Str range_expr = MC_STR(str.beg + 1, closing_bracket.beg);
    if(closing_bracket.end != str.end && *closing_bracket.end == ']') { // "...]]"
        range_expr.end = closing_bracket.end;
    }
    else if(closing_bracket.end + 1 != str.end 
    && closing_bracket.end[0] == '-'
    && closing_bracket.end[1] == ']'){ // "...]-]"
        range_expr.end = closing_bracket.end + 1;
    }

    bool inverse = false;
    if(range_expr.beg[0] == '^' && mc_str_len(range_expr) != 1) {
        inverse = true;
        range_expr.beg++;
    }

    *atom = (Atom) { .type = ATOM_RANGE };

    while (!mc_str_empty(range_expr)) {
        MC_Str lhs = mc_str_pop_front(&range_expr, 1);
        MC_Str rhs = mc_str_pop_front(&range_expr, 1);
        if(mc_str_empty(rhs)) {
            char_set_add_range(atom->range, *lhs.beg, *lhs.beg);
            break;
        }

        if(mc_str_equ(rhs, MC_STRC("-"))) {
            rhs = mc_str_pop_front(&range_expr, 1);
            if(mc_str_empty(rhs)) {
                char_set_add_range(atom->range, *lhs.beg, *lhs.beg);
                char_set_add_range(atom->range, '-', '-');
                break;
            }

            char_set_add_range(atom->range, *lhs.beg, *rhs.beg);
        }
        else {
            char_set_add_range(atom->range, *lhs.beg, *lhs.beg);
            char_set_add_range(atom->range, *rhs.beg, *rhs.beg);
        }
    }
    
    if(inverse) {
        for(int byte = 0; byte != sizeof atom->range; byte++) {
            atom->range[byte] = ~atom->range[byte];
        }
    }

    return range_expr.end + 1;
}

static const char *read_atom(MC_Str str, Atom *atom) {
    if(*str.beg == '[') {
        const char *end = read_range(str, atom);
        if(end) {
            return end;
        }
    }

    if(*str.beg == '%') {
        if(mc_str_len(str) == 1) {
            *atom = (Atom){.type = ATOM_SEQUENCE, .sequence = str };
            return str.end;
        }

        switch (*++str.beg){
            case 's': case 'a': case 'l':
            case 'u': case 'd': case 'x':
            case 'g': case 'p': case 'c':
            case 'w':
                MC_ASSERT_BUG(get_well_known_atom(*str.beg, atom) && "UNDEFINED WELL-KNOWN ATOM");
                return str.beg + 1;
            default:
                *atom = (Atom){.type = ATOM_SEQUENCE, .sequence = MC_STR(str.beg, str.beg + 1)};
                return str.beg + 1;
        }
    }

    if(*str.beg == '.') {
        MC_ASSERT_BUG(get_well_known_atom('.', atom) && "UNDEFINED WELL-KNOWN ATOM");
        return str.beg + 1;
    }

    *atom = (Atom){.type = ATOM_SEQUENCE, .sequence = MC_STR(str.beg, str.beg + 1) };
    return str.beg + 1;

    return NULL;
}

static const char *read_modifier(MC_Str str, Modifier *mod) {
    if(mc_str_empty(str)) {
        *mod = (Modifier){ .min = 1, .max = 1 };
        return str.beg;
    }

    switch (*str.beg){
    case '?':
        *mod = (Modifier){ .min = 0, .max = 1 };
        return str.beg + 1;
    case '+':
        *mod = (Modifier){ .min = 1, .max = 0xFFFFFFFF };
        return str.beg + 1;
    case '-':
        *mod = (Modifier){ .min = 0, .max = 0xFFFFFFFF, .lazy = true };
        return str.beg + 1;
    case '*':
        *mod = (Modifier){ .min = 0, .max = 0xFFFFFFFF };
        return str.beg + 1;
    }

    *mod = (Modifier){ .min = 1, .max = 1 };
    return str.beg;
}

static const char *match_atom(const Atom *atom, MC_Str str) {
    if(atom->type == ATOM_RANGE) {
        if(mc_str_empty(str)) {
            return NULL;
        }

        unsigned byte = *str.beg >> 3;
        unsigned bit = *str.beg & 0x7;
        return atom->range[byte] & (1 << bit)
            ? str.beg + 1
            : NULL;
    }

    if(atom->type == ATOM_SEQUENCE) {
        if(mc_str_starts_with(str, atom->sequence)) {
            return str.beg + mc_str_len(atom->sequence);
        }

        return NULL;
    }

    MC_ASSERT_BUG("NOT IMPLEMENTED" && 0);
    return NULL;
}

static const char *match_atom_reverse(const Atom *atom, MC_Str str) {
    if(atom->type == ATOM_RANGE) {
        if(mc_str_empty(str)) {
            return NULL;
        }

        unsigned byte = str.end[-1] >> 3;
        unsigned bit = str.end[-1] & 0x7;
        return atom->range[byte] & (1 << bit)
            ? str.end - 1
            : NULL;
    }

    if(atom->type == ATOM_SEQUENCE) {
        if(mc_str_ends_with(str, atom->sequence)) {
            return str.end - mc_str_len(atom->sequence);
        }

        return NULL;
    }

    MC_ASSERT_BUG("NOT IMPLEMENTED" && 0);
    return NULL;
}

static MatchStatus matchv_from_beg(MC_Str str, MC_Str fmt, MC_Str *whole, va_list args) {
    while (true) {
        if(mc_str_empty(fmt)) {
            whole->end = str.beg;
            return MATCH;
        }

        if(*fmt.beg == '(') {
            MC_Str close_paren = mc_str_substr(fmt, MC_STRC(")"));
            if(mc_str_empty(close_paren)) {
                return INVALID_EXPESSION;
            }

            MC_Str *group = va_arg(args, MC_Str*);
            group->beg = str.beg;
            MatchStatus status = matchv_from_beg(str, MC_STR(fmt.beg + 1, close_paren.beg), group, args);
            if(status == MATCH) {
                return matchv_from_beg(MC_STR(group->end, str.end), MC_STR(close_paren.end, fmt.end), whole, args);
            }

            return status;
        }

        Atom atom;
        Modifier mod;
        fmt.beg = read_atom(fmt, &atom);
        if(fmt.beg == NULL) {
            return INVALID_EXPESSION;
        }

        fmt.beg = read_modifier(fmt, &mod);

        for(unsigned i = 0; i < mod.min; i++) {
            str.beg = match_atom(&atom, str);
            if(str.beg == NULL) {
                return NOT_MATCH;
            }
        }

        if(mod.lazy) {
            for(unsigned i = mod.min; i < mod.max; i++) {
                MatchStatus status = matchv_from_beg(str, fmt, whole, args);
                if(status != NOT_MATCH) return status;

                str.beg = match_atom(&atom, str);
                if(str.beg == NULL) {
                    return NOT_MATCH;
                }
            }
        }
        else {
            const char *backtrack_first = str.beg;
            const char *backtrack_last = str.beg;
            for(unsigned i = mod.min; i < mod.max; i++) {
                const char *next_entry = match_atom(&atom, MC_STR(backtrack_last, str.end));
                if(!next_entry || next_entry == backtrack_last) {
                    break;
                }

                backtrack_last = next_entry;
            }

            for(const char *backtrack = backtrack_last; backtrack >= backtrack_first;) {
                MatchStatus status = matchv_from_beg(MC_STR(backtrack , str.end), fmt, whole, args);
                if(status != NOT_MATCH) return status;

                backtrack = match_atom_reverse(&atom, MC_STR(str.beg, backtrack));
            }
        }
    }
}

bool mc_str_matchv(MC_Str str, const char *fmt, MC_Str *whole, va_list args) {
    if (*fmt == 0) {
        *whole = MC_STR(str.beg, str.beg);
        return true;
    }

    MC_Str fmt_str = MC_STR(fmt, fmt + strlen(fmt));

    if(*fmt == '^') {
        fmt_str.beg++;
        whole->beg = str.beg;
        return matchv_from_beg(str, fmt_str, whole, args) == MATCH;
    }

    for(const char *cur = str.beg; cur != str.end; cur++) {
        va_list cp;
        va_copy(cp, args);
        whole->beg = cur;
        MatchStatus matches = matchv_from_beg(MC_STR(cur, str.end), fmt_str, whole, cp);
        va_end(cp);

        if(matches == MATCH) {
            return true;
        }
        else if(matches == INVALID_EXPESSION) {
            return false;
        }
    }

    return false;
}

bool mc_str_match(MC_Str str, const char *fmt, MC_Str *whole, ...) {
    va_list args;
    va_start(args, whole);
    bool match = mc_str_matchv(str, fmt, whole, args);
    va_end(args);
    return match;
}

