#include <stdlib.h>
#include <assert.h>
#include "gram/parser.h"
#include "gram/ast.h"

#define ACTION (void (*)(union gram_result, void *))
void (*gram_ast_actions[])(union gram_result result, void *context) = {
    [AI(DO_PARSER_SPEC)] = ACTION do_parser_spec,

    [AI(DO_PATTERN_DEF)] = ACTION do_pattern_def,
    [AI(DO_APPEND_PATTERN_DEF)] = ACTION do_append,
    [AI(DO_PATTERN_DEFS_HEAD)] = ACTION do_head,

    [AI(DO_RULE)] = ACTION do_rule,
    [AI(DO_APPEND_RULE)] = ACTION do_append,
    [AI(DO_RULES_HEAD)] = ACTION do_head,

    [AI(DO_ALT)] = ACTION do_alt,
    [AI(DO_APPEND_ALT)] = ACTION do_append,
    [AI(DO_ALTS_HEAD)] = ACTION do_head,

    [AI(DO_ID_RHS)] = ACTION do_id_rhs,
    [AI(DO_LIT_RHS)] = ACTION do_lit_rhs,
    [AI(DO_EMPTY_RHS)] = ACTION do_empty_rhs,
    [AI(DO_APPEND_RHS)] = ACTION do_append,
    [AI(DO_RHSES_HEAD)] = ACTION do_head
};
#undef ACTION

static void *gast(struct gram_ast_context *context) { return context->ast; }
static void sast(void *ast, struct gram_ast_context *context)  { context->ast = ast;  }

union gram_result ast_to_result(struct gram_ast_context *context) {
    return (union gram_result) { .ast = gast(context) };
}

struct parser_spec *get_parser_spec(struct gram_ast_context *context) {
    return gast(context);
}

struct parser_spec parser_spec(struct pattern_def *pattern_defs, struct rule *rules) {
    return (struct parser_spec) { pattern_defs, rules };
}

struct pattern_def pattern_def(char *id, char *regex, struct pattern_def *next) {
    return (struct pattern_def) { id, regex, next };
}

struct rule rule(char *id, struct alt *alts, struct rule *next) {
    return (struct rule) { id, alts, next };
}

struct alt alt(struct rhs *rhses, struct alt *next) {
    return (struct alt) { rhses, next };
}

static struct rhs rhs(enum rhs_type type, struct rhs *next, char *sym) {
    return (struct rhs) { type, next, sym };
}

struct parser_spec *init_parser_spec(struct pattern_def *defs, struct rule *rules) {
    struct parser_spec *n = malloc(sizeof *n);
    assert(n != NULL);
    *n = parser_spec(defs, rules);
    return n;
}

struct pattern_def *init_pattern_def(char *id, char *regex, struct pattern_def *next) {
    struct pattern_def *n = malloc(sizeof *n);
    assert(n != NULL);
    *n = pattern_def(id, regex, next);
    return n;
}

struct rule *init_rule(char *id, struct alt *alts, struct rule *next) {
    struct rule *n = malloc(sizeof *n);
    assert(n != NULL);
    *n = rule(id, alts, next);
    return n;
}

struct alt *init_alt(struct rhs *rhses, struct alt *next) {
    struct alt *n = malloc(sizeof *n);
    assert(n != NULL);
    *n = alt(rhses, next);
    return n;
}

static struct rhs *init_rhs(enum rhs_type type, char *sym, struct rhs *next) {
    struct rhs *n = malloc(sizeof *n);
    assert(n != NULL);
    *n = rhs(type, sym, next);
    return n;
}

struct rhs *init_id_rhs(char *sym, struct rhs *next) {
    return init_rhs(ID_RHS, sym, next);
}

struct rhs *init_lit_rhs(char *sym, struct rhs *next) {
    return init_rhs(LIT_RHS, sym, next);
}

struct rhs *init_empty_rhs(struct rhs *next) {
    return init_rhs(EMPTY_RHS, NULL, next);
}

void free_parser_spec(struct parser_spec *spec) {
    free_pattern_def(spec->pattern_defs);
    spec->pattern_defs = NULL;
    free_rule(spec->rules);
    spec->rules = NULL;
    free(spec);
}

void free_pattern_def(struct pattern_def *def) {
    for (struct pattern_def *next; def; def = next)
        next = def->next, free(def);
}

void free_rule(struct rule *rule) {
    for (struct rule *next; rule; rule = next) {
        next = rule->next;
        free_alt(rule->alts);
        rule->alts = NULL;
        free(rule);
    }
}

void free_alt(struct alt *alt) {
    for (struct alt *next; alt; alt = next) {
        next = alt->next;
        free_rhs(alt->rhses);
        alt->rhses = NULL;
        free(alt);
    }
}

void free_rhs(struct rhs *rhs) {
    for (struct rhs *next; rhs; rhs = next)
        next = rhs->next, free(rhs);
}

void do_parser_spec(union gram_result result, struct gram_ast_context *context) {
    sast(init_parser_spec(result.ast, gast(context)), context);
}

void do_pattern_def(union gram_result result, struct gram_ast_context *context) {
    sast(init_pattern_def(result.pdef.id, result.pdef.regex, NULL), context);
}

void do_rule(union gram_result result, struct gram_ast_context *context) {
    sast(init_rule(result.id, gast(context), NULL), context);
}

void do_alt(union gram_result _, struct gram_ast_context *context) {
    sast(init_alt(gast(context), NULL), context);
}

void do_id_rhs(union gram_result result, struct gram_ast_context *context) {
    sast(init_id_rhs(result.id, NULL), context);
}

void do_lit_rhs(union gram_result result, struct gram_ast_context *context) {
    sast(init_lit_rhs(result.lit, NULL), context);
}

void do_empty_rhs(union gram_result _, struct gram_ast_context *context) {
    sast(init_empty_rhs(), context);
}

void do_head(union gram_result result, struct gram_ast_context *context) {
    sast(result.ast, context);
}

void do_append(union gram_result result, struct gram_ast_context *context) {
    result.ast->next = gast(context);
}

