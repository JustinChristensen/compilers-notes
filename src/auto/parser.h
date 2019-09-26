#ifndef AUTO_PARSER_H_
#define AUTO_PARSER_H_ 1

#include <stdbool.h>
#include "ast.h"

/**
 * Grammar:
 *
 * regex: exprs eof
 * exprs: ε { empty } exprs_tail
 * exprs_tail: expr exprs_tail
 *           | ε
 * expr: ( exprs ) { sub } { cat }
 *     | a { sym } { cat }
 *     | * { star }
 *     | + exprs { alt }
 *
 * TODO: extensions
 * zero or one: ?
 * one or more: +
 * character classes: [a-zA-Z]
 */

#define EXPR_MAX 5000
#define OPERATOR_OFFSET (-256)

enum token_type {
    SYMBOL = -1,
    ALT = ('|' + OPERATOR_OFFSET),
    STAR = ('*' + OPERATOR_OFFSET),
    RPAREN = (')' + OPERATOR_OFFSET),
    LPAREN = ('(' + OPERATOR_OFFSET)
};

struct scan_context {
    char *input;
    int input_col;
    int token;
    int token_col;
};

struct parse_error {
    int token_col;
    int expected;
    int actual;
};

struct parse_context {
    struct expr *exprbuf;
    struct scan_context scan_context;
    int lookahead;
    int lookahead_col;
    bool has_error;
    struct expr *expr;
    struct parse_error error;
};

struct scan_context scan_context(char *input);
struct scan_context scan(struct scan_context context);
int token(struct scan_context context);
int token_col(struct scan_context context);
struct parse_context parse_context(char *input, struct expr *exprbuf);
bool peek(struct parse_context *context, int expected, int (*is) (int c));
bool expect(struct parse_context *context, int expected, int (*is) (int c));
int is_symbol(int c);
int lookahead(struct parse_context *context);
bool parse_regex(struct parse_context *context);
bool parse_exprs(struct parse_context *context);
bool parse_expr(struct parse_context *context, struct expr *lexpr);
void sexpr(struct parse_context *context, struct expr expr);
struct expr *gexpr(struct parse_context *context);
bool has_error(struct parse_context *context);
struct parse_error gerror(struct parse_context *context);
char *lexeme_for(char *symbuf, int token);
void print_error(struct parse_error error);

#endif // AUTO_PARSER_H_
