#ifndef GRAM_PACK_H_
#define GRAM_PACK_H_ 1

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <regex/nfa.h>
#include "gram/ast.h"
#include "gram/parser.h"

struct gram_packed_spec {
    struct regex_pattern *patterns;
    struct gram_symbol *symbols;
    int **rules;
};

#define GM_END_PATTERN  { 0 }
#define GM_START_SYMBOL { 0 }
#define GM_END_SYMBOL   { 0 }
#define GM_START_RULE   NULL
#define GM_END_RULE     NULL

#ifndef NDEBUG
void assert_gram_packed_spec(struct gram_packed_spec const *spec);
#endif

struct gram_packed_spec *gram_pack(struct gram_parser_spec *spec, struct hash_table *symtab, struct gram_stats stats);
void print_gram_packed_spec(FILE *handle, struct gram_packed_spec *spec);
void free_gram_packed_spec(struct gram_packed_spec *spec);

#endif // GRAM_PACK_H_

