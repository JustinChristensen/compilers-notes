#ifndef GRAM_PARSER_H_
#define GRAM_PARSER_H_ 1

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <base/array.h>
#include <base/hash_table.h>
#include <regex/nfa.h>

/*
parser_spec       = pattern_defs_head grammar eof;
pattern_defs_head = pattern_def pattern_defs { pattern_defs_head } | $empty;
pattern_defs      = pattern_def { += pattern_def } pattern_defs | $empty;
pdef_flags        = '@' | '-' | $empty;
pattern_def       = '@' id regex { tag_only_pattern_def }
                  | '-' id regex { skip_pattern_def }
                  | pdef_flags id regex { pattern_def };
grammar           = "---" rules_head | $empty;
rules_head        = rule rules { rules_head } | $empty;
rules             = rule { += rule } rules | $empty;
rule              = id '=' alt alts ';' { rule };
alts              = '|' alt { += alt } alts | $empty;
alt               = rhs rhses { alt };
rhses             = rhs { += rhs } rhses | $empty;
rhs               = id { id_rhs(lexeme) }
                  | char { char_rhs(lexeme) }
                  | string { string_rhs(lexeme) }
                  | "$empty" { empty };
*/

enum gram_symbol_type {
    GM_TERM,
    GM_NONTERM
};

struct gram_symbol {
    enum gram_symbol_type type;
    unsigned int num;
    union {
        unsigned int *rules;    // nonterm derives rules
    };
};

enum gram_parser_symbol {
    GM_ERROR,

    // terminals
    GM_EOF_T,
    GM_TAG_ONLY_T,
    GM_SKIP_T,
    GM_REGEX_T,
    GM_SECTION_T,
    GM_ASSIGN_T,
    GM_ALT_T,
    GM_SEMICOLON_T,
    GM_CHAR_T,
    GM_STRING_T,
    GM_EMPTY_T,
    GM_ID_T,

    // non-terminals
    GM_PARSER_SPEC_NT,
    GM_PATTERN_DEFS_NT,
    GM_PATTERN_DEF_NT,
    GM_GRAMMAR_NT,
    GM_RULES_NT,
    GM_RULE_NT,
    GM_ALTS_NT,
    GM_ALT_NT,
    GM_RHSES_NT,
    GM_RHS_NT
};

enum gram_parse_error_type {
    GM_SYNTAX_ERROR,
    GM_OOM_ERROR,
    GM_SCANNER_ERROR
    // GM_PATTERN_DEFINED,
    // GM_DUPLICATE_PATTERN,
    // GM_NONTERM_DEFINED_AS_TERM,
    // scanner oom, missing tag, tag exists, duplicate pattern
};

struct gram_parse_error {
    enum gram_parse_error_type type;
    union {
        struct {
            enum gram_parser_symbol actual;
            struct regex_loc loc;
            enum gram_parser_symbol *expected;
        };
        struct regex_error scanerr;
    };
};

struct gram_parse_context {
    void *ast;
    struct hash_table *symtab;
    unsigned int terms;
    unsigned int nonterms;
    unsigned int rules;
    void *current_rule;

    struct nfa_context scanner;
    struct nfa_match match;
    enum gram_parser_symbol sym;

    bool has_error;
    struct gram_parse_error error;
};

bool gram_parse_context(struct gram_parse_context *context);
void free_gram_parse_context(struct gram_parse_context *context);
bool gram_parse_has_error(struct gram_parse_context *context);
struct gram_parse_error gram_parser_error(struct gram_parse_context *context);
struct gram_parse_error gram_syntax_error(enum gram_parser_symbol actual, struct regex_loc loc, enum gram_parser_symbol expected);
void print_gram_parse_error(FILE *handle, struct gram_parse_error error);
bool gram_start_scanning(char *input, struct gram_parse_context *context);
enum gram_parser_symbol gram_scan(struct gram_parse_context *context);
enum gram_parser_symbol gram_lookahead(struct gram_parse_context *context);
struct regex_loc gram_location(struct gram_parse_context *context);
bool gram_lexeme(char *lexeme, struct gram_parse_context *context);
bool parse_gram_parser_spec(char *spec, struct gram_parse_context *context);
struct gram_parser_spec *gram_parser_spec(struct gram_parse_context *context);
bool gram_check(struct gram_parse_context *context);
struct regex_pattern *gram_pack_patterns(struct gram_parse_context *context);
struct gram_symbol *gram_pack_symbols(struct gram_parse_context *context);
unsigned int **gram_pack_rules(struct gram_parse_context *context);
void gram_free_rules(unsigned int **rules);
void print_gram_tokens(FILE *handle, char *spec);

#endif // GRAM_PARSER_H_
