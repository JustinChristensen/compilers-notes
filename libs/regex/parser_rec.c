#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include "regex/parser.h"
#include "regex/parser_rec.h"
#include "regex/result_types.h"

bool parse_regex(struct parse_context *context) {
    if (parse_expr(context) && expect(context, EOI)) {
        do_action(context, DO_REGEX, NULLRVAL);
        return true;
    }

    return false;
}

bool parse_expr(struct parse_context *context) {
    parse_alt(context);
    return true;
}

bool parse_alt(struct parse_context *context) {
    if (parse_cat(context)) {
        while (true) {
            union rval lval = getval(context);

            // FIXME: should this really be expr?
            if (peek(context, ALT) &&
                expect(context, ALT) &&
                parse_expr(context)) {
                do_action(context, DO_ALT, lval);
                continue;
            }

            break;
        }

        return true;
    }

    return false;
}

bool parse_cat(struct parse_context *context) {
    do_action(context, DO_EMPTY, NULLRVAL);
    union rval empty = getval(context);

    if (parse_factor(context)) {
        while (true) {
            union rval lval = getval(context);

            if (parse_factor(context)) {
                do_action(context, DO_CAT, lval);
                continue;
            }

            break;
        }

        do_action(context, DO_CAT, empty);

        return true;
    }

    return false;
}

bool parse_factor(struct parse_context *context) {
    bool has_head = false;

    // FIXME: this doesn't distinguish between selecting ε for expr and
    // a parse error encountered during expr
    if (peek(context, LPAREN) &&
        expect(context, LPAREN) &&
        parse_expr(context) && expect(context, RPAREN)) {
        do_action(context, DO_SUB, NULLRVAL);
        has_head = true;
    } else if (peek(context, DOTALL)) {
        expect(context, DOTALL);
        do_action(context, DO_DOTALL, NULLRVAL);
        has_head = true;
    } else if (peek(context, SYMBOL)) {
        union rval sym = { .sym = symbol(context) };
        expect(context, SYMBOL);
        do_action(context, DO_SYMBOL, sym);
        has_head = true;
    }

    if (has_head) {
        while (true) {
            if (peek(context, STAR) && expect(context, STAR)) {
                do_action(context, DO_STAR, NULLRVAL);
                continue;
            } else if (peek(context, PLUS) && expect(context, PLUS)) {
                do_action(context, DO_PLUS, NULLRVAL);
                continue;
            } else if (peek(context, OPTIONAL) && expect(context, OPTIONAL)) {
                do_action(context, DO_OPTIONAL, NULLRVAL);
                continue;
            }

            break;
        }

        return true;
    }

    return false;
}
