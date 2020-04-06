#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <base/debug.h>
#include "gram/spec.h"

#ifdef INVARIANTS
void assert_gram_parsed_spec(struct gram_parser_spec const *spec) {
    assert(spec != NULL);
    assert(spec->type == GM_PARSED_SPEC);
}

void assert_gram_packed_spec(struct gram_parser_spec const *spec) {
    assert(spec != NULL);
    assert(spec->type == GM_PACKED_SPEC);

    assert(spec->symbols != NULL);
    assert(spec->symbols[0].num == 0);
    assert(spec->symbols[0].rules == NULL);

    assert(spec->rules != NULL);
    assert(spec->rules[0] == NULL);
}

void assert_gram_sym_index(unsigned int i, struct gram_stats stats) {
    assert(i >= 1 && i <= stats.symbols);
}
#endif

static struct gram_stats nsymbols(struct gram_stats stats) {
    stats.symbols = stats.terms + stats.nonterms;
    return stats;
}

struct gram_parser_spec gram_parsed_spec(
    struct gram_pattern_def *pdefs, struct gram_rule *rules,
    int start_rule, struct gram_stats stats
) {
    return (struct gram_parser_spec) {
        GM_PARSED_SPEC,
        .pdefs = pdefs, .prules = rules,
        .start_rule = start_rule, .stats = nsymbols(stats)
    };
}

struct gram_parser_spec gram_packed_spec(
    struct regex_pattern *patterns, struct gram_symbol *symbols,
    unsigned int **rules, unsigned int start_rule, struct gram_stats stats
) {
    return (struct gram_parser_spec) {
        GM_PACKED_SPEC,
        .patterns = patterns, .symbols = symbols, .rules = rules,
        .start_rule = start_rule, .stats = nsymbols(stats)
    };
}

#define N_(next) (1 + (next ? next->n : 0))

struct gram_pattern_def *init_gram_pattern_def(
    struct regex_loc loc,
    char *id, char *regex,
    bool tag_only, bool skip,
    struct gram_pattern_def *next
) {
    struct gram_pattern_def *pdef = malloc(sizeof *pdef);

    if (!pdef) return NULL;

    if (id && ((id = strdup(id)) == NULL)) {
        free(pdef);
        return NULL;
    }

    if (regex && ((regex = strdup(regex)) == NULL)) {
        free(pdef);
        free(id);
        return NULL;
    }

    *pdef = (struct gram_pattern_def) {
        loc, id, regex,
        .tag_only = tag_only,
        .skip = skip,
        next, N_(next)
    };

    return pdef;
}

struct gram_rule *init_gram_rule(struct regex_loc loc, char *id, struct gram_alt *alts, struct gram_rule *next) {
    struct gram_rule *rule = malloc(sizeof *rule);

    if (!rule) return NULL;

    if (id && ((id = strdup(id)) == NULL)) {
        free(rule);
        return NULL;
    }

    *rule = (struct gram_rule) { loc, id, alts, next, N_(next) };

    return rule;
}

struct gram_alt *init_gram_alt(struct regex_loc loc, struct gram_rhs *rhses, struct gram_alt *next) {
    struct gram_alt *alt = malloc(sizeof *alt);

    if (!alt) return NULL;

    *alt = (struct gram_alt) { loc, rhses, next, N_(next) };

    return alt;
}

static struct gram_rhs *init_rhs(struct regex_loc loc, enum gram_rhs_type type, char *str, struct gram_rhs *next) {
    struct gram_rhs *rhs = malloc(sizeof *rhs);

    if (!rhs) return NULL;

    if (str && ((str = strdup(str)) == NULL)) {
        free(rhs);
        return NULL;
    }

    *rhs = (struct gram_rhs) { loc, type, .str = str, next, N_(next) };

    return rhs;
}

struct gram_rhs *init_id_gram_rhs(struct regex_loc loc, char *str, struct gram_rhs *next) {
    return init_rhs(loc, GM_ID_RHS, str, next);
}

struct gram_rhs *init_char_gram_rhs(struct regex_loc loc, char *str, struct gram_rhs *next) {
    return init_rhs(loc, GM_CHAR_RHS, str, next);
}

struct gram_rhs *init_string_gram_rhs(struct regex_loc loc, char *str, struct gram_rhs *next) {
    return init_rhs(loc, GM_STRING_RHS, str, next);
}

struct gram_rhs *init_eof_gram_rhs(struct regex_loc loc, struct gram_rhs *next) {
    return init_rhs(loc, GM_EOF_RHS, GM_EOF_TOKEN, next);
}

struct gram_rhs *init_empty_gram_rhs(struct regex_loc loc, struct gram_rhs *next) {
    return init_rhs(loc, GM_EMPTY_RHS, NULL, next);
}

void free_gram_pattern_def(struct gram_pattern_def *pdef) {
    if (!pdef) return;

    for (struct gram_pattern_def *next; pdef; pdef = next) {
        next = pdef->next;
        free(pdef->id);
        free(pdef->regex);
        free(pdef);
    }
}

void free_gram_rule(struct gram_rule *rule) {
    if (!rule) return;

    for (struct gram_rule *next; rule; rule = next) {
        next = rule->next;
        free(rule->id);
        free_gram_alt(rule->alts);
        free(rule);
    }
}

void free_gram_alt(struct gram_alt *alt) {
    if (!alt) return;

    for (struct gram_alt *next; alt; alt = next) {
        next = alt->next;
        free_gram_rhs(alt->rhses);
        free(alt);
    }
}

void free_gram_rhs(struct gram_rhs *rhs) {
    if (!rhs) return;

    for (struct gram_rhs *next; rhs; rhs = next) {
        next = rhs->next;
        switch (rhs->type) {
            case GM_ID_RHS:
            case GM_CHAR_RHS:
            case GM_STRING_RHS:
            case GM_EOF_RHS:
                free(rhs->str);
                break;
            case GM_EMPTY_RHS:
                break;
        }
        free(rhs);
    }
}

static void free_parsed_spec(struct gram_parser_spec *spec) {
    free_gram_pattern_def(spec->pdefs);
    spec->pdefs = NULL;
    free_gram_rule(spec->prules);
    spec->prules = NULL;
}

static void free_patterns(struct regex_pattern *patterns) {
    if (!patterns) return;
    struct regex_pattern *p = patterns;
    while (p->sym) {
        free(p->tag);
        free(p->pattern);
        p++;
    }
    free(patterns);
}

static void free_symbols(struct gram_symbol *symbols) {
    if (!symbols) return;

    struct gram_symbol *sym = &symbols[1];
    while (sym->num) {
        if (sym->rules) free(sym->rules);
        sym++;
    }

    free(symbols);
}

static void free_rules(unsigned int **rules) {
    if (!rules) return;
    unsigned int **r = &rules[1];
    while (*r) free(*r), r++;
    free(rules);
}

static void free_packed_spec(struct gram_parser_spec *spec) {
    free_patterns(spec->patterns);
    free_symbols(spec->symbols);
    free_rules(spec->rules);
}

void free_gram_parser_spec(struct gram_parser_spec *spec) {
    if (!spec) return;

    switch (spec->type) {
        case GM_PARSED_SPEC:
        case GM_CHECKED_SPEC:
            free_parsed_spec(spec);
            break;
        case GM_PACKED_SPEC:
            free_packed_spec(spec);
            break;
    }
}

#define PATTERN_DEF_FMT "%s %s\n"
static void print_gram_pattern_def(FILE *handle, struct gram_pattern_def *def) {
    for (; def; def = def->next) {
        if (def->tag_only) fprintf(handle, "@");
        if (def->skip) fprintf(handle, "-");
        fprintf(handle, PATTERN_DEF_FMT, def->id, def->regex);
    }
}

#define RHS_FMT "%s"
#define EMPTY_RHS_FMT "$empty"
#define EOF_RHS_FMT "$eof"
#define RHS_SEP_FMT " "
static void _print_gram_rhs(FILE *handle, struct gram_rhs *rhs) {
    switch (rhs->type) {
        case GM_ID_RHS:
        case GM_CHAR_RHS:
        case GM_STRING_RHS:
            fprintf(handle, RHS_FMT, rhs->str);
            break;
        case GM_EOF_RHS:
            fprintf(handle, EOF_RHS_FMT);
            break;
        case GM_EMPTY_RHS:
            fprintf(handle, EMPTY_RHS_FMT);
            break;
    }
}

static void print_gram_rhs(FILE *handle, struct gram_rhs *rhs) {
    if (rhs) {
        _print_gram_rhs(handle, rhs);
        for (rhs = rhs->next; rhs; rhs = rhs->next) {
            fprintf(handle, RHS_SEP_FMT);
            _print_gram_rhs(handle, rhs);
        }
    }
}

#define ALT_FMT " | "
static void print_gram_alt(FILE *handle, struct gram_alt *alt) {
    if (alt) {
        print_gram_rhs(handle, alt->rhses);

        for (alt = alt->next; alt; alt = alt->next) {
            fprintf(handle, ALT_FMT);
            print_gram_rhs(handle, alt->rhses);
        }
    }
}

#define RULE_FMT "%s = "
#define RULE_END_FMT ";\n"
static void print_gram_rule(FILE *handle, struct gram_rule *rule) {
    for (; rule; rule = rule->next) {
        fprintf(handle, RULE_FMT, rule->id);
        print_gram_alt(handle, rule->alts);
        fprintf(handle, RULE_END_FMT);
    }
}

static void print_parsed_spec(FILE *handle, struct gram_parser_spec const *spec) {
    invariants(assert_gram_parsed_spec, spec);

    print_gram_pattern_def(handle, spec->pdefs);
    if (spec->prules) {
        fprintf(handle, "---\n");
        print_gram_rule(handle, spec->prules);
    }
}

#define PATTERNS_TITLE_FMT "patterns:\n"
#define PATTERNS_HEADER_FMT "  %4s  %s\n"
#define SYMBOLS_TITLE_FMT "symbols:\n"
#define SYMBOLS_HEADER_FMT "  %4s  %-7s  %-8s  %s\n"
#define START_RULE_FMT "start rule: %d\n"
#define RULES_TITLE_FMT "rules:\n"
#define RULES_HEADER_FMT "  %4s  %s\n"
static void print_packed_spec(FILE *handle, struct gram_parser_spec const *spec) {
    invariants(assert_gram_packed_spec, spec);

    // print packed patterns
    struct regex_pattern *pat = spec->patterns;
    fprintf(handle, PATTERNS_TITLE_FMT);
    if (pat->sym) {
        fprintf(handle, PATTERNS_HEADER_FMT, "num", "pattern");
        while (pat->sym) {
            fprintf(handle, "  %4d  %s\n", pat->sym, pat->pattern);
            pat++;
        }
    }
    fprintf(handle, "\n");

    // print packed symbols
    struct gram_symbol *sym = &spec->symbols[1];
    fprintf(handle, SYMBOLS_TITLE_FMT);
    fprintf(handle, SYMBOLS_HEADER_FMT, "num", "type", "nullable", "rules");
    fprintf(handle, "  %4d  ---\n", 0);
    while (sym->num) {
        char *type = "nonterm";
        if (sym->num == GM_EOF_NUM + 1) type = "eof";
        else if (sym->type == GM_TERM)  type = "term";

        fprintf(handle, "  %4d  %-7s", sym->num, type);

        if (sym->type == GM_NONTERM) {
            fprintf(handle, "  %-8s", sym->nullable ? "yes" : "no");
            unsigned int *r = sym->rules;
            fprintf(handle, "  %d", *r++);
            while (*r)
                fprintf(handle, ", %d", *r), r++;
        }

        fprintf(handle, "\n");
        sym++;
    }
    fprintf(handle, "\n");

    fprintf(handle, START_RULE_FMT, spec->start_rule);

    // print packed rules
    unsigned int **rule = &spec->rules[1];
    fprintf(handle, RULES_TITLE_FMT);
    fprintf(handle, RULES_HEADER_FMT, "rule", "symbols");
    fprintf(handle, "  %4d  %s\n", 0, "---");
    int r = 1;
    while (*rule) {
        unsigned int *s = *rule;

        fprintf(handle, "  %4d", r);

        if (*s) {
            fprintf(handle, "  %d", *s++);
            while (*s)
                fprintf(handle, ", %d", *s), s++;
        }

        fprintf(handle, "\n");

        rule++;
        r++;
    }
    fprintf(handle, "\n");
}

void print_gram_parser_spec(FILE *handle, struct gram_parser_spec const *spec) {
    if (!spec) return;

    switch (spec->type) {
        case GM_PARSED_SPEC:
        case GM_CHECKED_SPEC:
            print_parsed_spec(handle, spec);
            break;
        case GM_PACKED_SPEC:
            print_packed_spec(handle, spec);
            break;
    }
}

#define STATS_TITLE_FMT "stats:\n"
#define STATS_FMT "  patterns: %d\n  terms: %d\n  nonterms: %d\n  symbols: %d\n  rules: %d\n"
void print_gram_stats(FILE *handle, struct gram_parser_spec const *spec) {
    struct gram_stats stats = spec->stats;
    fprintf(handle, STATS_TITLE_FMT);
    fprintf(handle, STATS_FMT, stats.patterns, stats.terms, stats.nonterms,
        stats.symbols, stats.rules);
    fprintf(handle, "\n");
}
