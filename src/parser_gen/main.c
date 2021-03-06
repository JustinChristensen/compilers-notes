#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <base/args.h>
#include <base/bitset.h>
#include <base/string.h>
#include <base/macros.h>
#include <gram/analyze.h>
#include <gram/ll.h>
#include <gram/lr.h>
#include <gram/states.h>
#include <gram/parser.h>

#define ONEKB (1 << 10)
#define BUFFER_SIZE (ONEKB * 50)

enum command_key {
    ANALYZE,
    AUTOMATA,
    GEN_PARSER,
    SCAN,
    TABLES
};

enum arg_key {
    PARSER_TYPE,
    LR_TYPE,
    SPEC_FILE,
    TEXT
};

enum parser_type {
    LL,
    SLR,
    // LALR,
    LR1
};

struct args {
    enum command_key cmd;
    enum parser_type type;
    char spec[ONEKB];
    bool text;
    int posc;
    char **pos;
};

bool handle_spec_file(int key, struct args *args) {
    if (key == SPEC_FILE) return strcpy(args->spec, argval()), true;
    return false;
}

bool handle_type(int key, struct args *args) {
    if (key != LR_TYPE && key != PARSER_TYPE) return false;

    if (key == PARSER_TYPE && streq("ll", argval())) {
        args->type = LL;
        return true;
    }

    args->type = SLR;

    if (streq("slr", argval())) {
        args->type = SLR;
        return true;
    } else if (streq("lr1", argval())) {
        args->type = LR1;
        return true;
    }

    return false;
}

void read_args(struct args *args, int cmd, struct args_context *context) {
    int key;

    while ((key = readarg(context)) != END) {
        if (cmd == ANALYZE) {
            if (handle_spec_file(key, args));
            else print_usage_exit(stderr, context);
        } else if (cmd == AUTOMATA) {
            if (handle_spec_file(key, args));
            else if (handle_type(key, args));
            else if (key == TEXT) args->text = true;
            else print_usage_exit(stderr, context);
        } else if (cmd == GEN_PARSER) {
            if (handle_spec_file(key, args));
            else if (handle_type(key, args));
            else print_usage_exit(stderr, context);
        } else if (cmd == TABLES) {
            if (handle_spec_file(key, args));
            else if (handle_type(key, args));
            else print_usage_exit(stderr, context);
        } else print_usage_exit(stderr, context);
    }

    args->cmd = cmd;
    args->pos = argv(context);
    args->posc = argc(context);
}

static size_t slurp_file(int bufsize, char *buf, char *filename) {
    FILE *fi = fopen(filename, "r");

    if (!fi) {
        fprintf(stderr, "failed to open %s\n", filename);
        return -1;
    }

    int nread = fread(buf, sizeof *buf, bufsize, fi);
    buf[nread] = '\0';

    if (ferror(fi)) {
        fprintf(stderr, "failed reading %s\n", filename);
        fclose(fi);
        return -1;
    }

    fclose(fi);

    return nread;
}

bool read_files(struct args const args, void *context, bool (*with_file)(void *context, char *filename, char *contents)) {
    char contents[ONEKB * 200] = "";

    if (args.posc == 0) {
        if (!isatty(STDIN_FILENO)) {
            int nread = fread(contents, sizeof *contents, BUFFER_SIZE, stdin);
            contents[nread] = '\0';
            return (*with_file)(context, "stdin", contents);
        }

        fprintf(stderr, "no input\n");
        return false;
    }

    for (int i = 0; i < args.posc; i++) {
        char *filename = args.pos[i];
        int nread = 0;
        if ((nread = slurp_file(BUFFER_SIZE, contents, filename)) == -1) {
            fprintf(stderr, "reading file %s failed\n", filename);
            return false;
        }

        printf("filename: %s, size: %d\n", filename, nread);
        if (!(*with_file)(context, filename, contents)) return false;
    }

    return true;
}

bool parse_spec(struct args args, bool (*with_spec)(struct args const args, struct gram_parser_spec *spec)) {
    if (!args.spec[0]) {
        fprintf(stderr, "spec file required\n");
        return false;
    }

    char specfile[BUFFER_SIZE] = "";

    int nread = 0;
    if ((nread = slurp_file(BUFFER_SIZE, specfile, args.spec)) == -1) {
        fprintf(stderr, "reading spec file %s failed\n", args.spec);
        return false;
    }

    struct gram_spec_parser parser = { 0 };
    struct gram_parse_error parser_error = { 0 };
    struct gram_parser_spec spec = { 0 };

    if (gram_spec_parser(&parser_error, &parser) && gram_parse(&parser_error, &spec, specfile, args.spec, &parser)) {
        free_gram_spec_parser(&parser);
        bool result = (*with_spec)(args, &spec);
        free_gram_parser_spec(&spec);
        return result;
    }

    print_gram_parse_error(stderr, parser_error);
    free_gram_spec_parser(&parser);
    free_gram_parser_spec(&spec);

    return false;
}

bool make_ll_parser(
    struct args const args, struct gram_parser_spec *spec,
    bool (*with_parser)(struct args const args, struct ll_parser const *parser)
) {
    struct ll_parser parser = { 0 };
    struct ll_error generr = { 0 };

    if (!gen_ll(&generr, &parser, spec)) {
        print_ll_error(stderr, generr);
        return false;
    }

    bool result = (*with_parser)(args, &parser);
    free_ll_parser(&parser);

    return result;
}

bool make_ll_parser_state(
    struct args const args, struct ll_parser const *parser,
    bool (*with_parser_state)(struct args const args, struct ll_parser_state *parser_state)
) {
    struct ll_parser_state pstate = ll_parser_state(parser);
    bool result = (*with_parser_state)(args, &pstate);
    free_ll_parser_state(&pstate);
    return result;
}

bool parse_file_ll(void *parser_state, char *filename, char *contents) {
    struct ll_error parse_error = { 0 };

    if (ll_parse(&parse_error, contents, filename, parser_state)) {
        printf("parsed %s\n", filename);
        return true;
    }

    print_ll_error(stderr, parse_error);

    return false;
}

bool print_ll_tables(struct args const _, struct ll_parser const *parser) {
    UNUSED(_);
    print_ll_parser(stdout, parser);
    return true;
}

bool make_lr_parser(
    struct args const args, struct gram_parser_spec *spec,
    bool (*with_parser)(struct args const args, struct lr_parser const *parser)
) {
    struct lr_parser parser = { 0 };
    struct lr_error generr = { 0 };

    action_table *table = slr_table;
    // if (args.type == LALR) table = lalr_table;
    if (args.type == LR1) table = lr1_table;

    if (!gen_lr(&generr, &parser, table, spec)) {
        print_lr_error(stderr, generr);
        return false;
    }

    bool result = (*with_parser)(args, &parser);
    free_lr_parser(&parser);

    return result;
}

bool make_lr_parser_state(
    struct args const args, struct lr_parser const *parser,
    bool (*with_parser_state)(struct args const args, struct lr_parser_state *parser_state)
) {
    struct lr_parser_state pstate = lr_parser_state(parser);
    bool result = (*with_parser_state)(args, &pstate);
    free_lr_parser_state(&pstate);
    return result;
}

bool parse_file_lr(void *parser_state, char *filename, char *contents) {
    struct lr_error parse_error = { 0 };

    if (lr_parse(&parse_error, contents, filename, parser_state)) {
        printf("parsed %s\n", filename);
        return true;
    }

    print_lr_error(stderr, parse_error);

    return false;
}

bool print_lr_tables(struct args const _, struct lr_parser const *parser) {
    UNUSED(_);
    print_lr_parser(stdout, parser);
    return true;
}

bool run_ll_parse(struct args const args, struct ll_parser_state *parser_state) {
    return read_files(args, parser_state, parse_file_ll);
}

bool run_ll_parser_state(struct args const args, struct ll_parser const *parser) {
    return make_ll_parser_state(args, parser, run_ll_parse);
}

bool run_ll_tables(struct args const args, struct gram_parser_spec *spec) {
    return make_ll_parser(args, spec, print_ll_tables);
}

bool run_ll_parser(struct args const args, struct gram_parser_spec *spec) {
    return make_ll_parser(args, spec, run_ll_parser_state);
}

bool run_lr_parse(struct args const args, struct lr_parser_state *parser_state) {
    return read_files(args, parser_state, parse_file_lr);
}

bool run_lr_parser_state(struct args const args, struct lr_parser const *parser) {
    return make_lr_parser_state(args, parser, run_lr_parse);
}

bool run_lr_tables(struct args const args, struct gram_parser_spec *spec) {
    return make_lr_parser(args, spec, print_lr_tables);
}

bool run_lr_parser(struct args const args, struct gram_parser_spec *spec) {
    return make_lr_parser(args, spec, run_lr_parser_state);
}

bool automata(struct args const args, struct gram_parser_spec *spec) {
    struct gram_symbol_analysis san = { 0 };

    if (!gram_analyze_symbols(&san, spec))
        return false;

    enum lr_item_type type = GM_LR0_ITEMS;
    if (args.type == LR1) type = GM_LR1_ITEMS;
    // else if (args.type == LALR) type = GM_LALR_ITEMS;

    bool result = false;

    unsigned nstates = 0;
    struct lr_state *states = NULL;
    if ((states = discover_lr_states(&nstates, type, &san, spec))) {
        if (args.text) {
            print_lr_states(stdout, nstates, states, spec);
            result = true;
        } else {
            result = print_lr_states_dot(stdout, nstates, states, spec);
        }
    }

    free_lr_states(nstates, states);
    free_gram_symbol_analysis(&san);

    return result;
}

bool analyze(struct args const args, struct gram_parser_spec *spec) {
    UNUSED(args);
    print_gram_stats(stdout, spec->stats);
    print_gram_parser_spec(stdout, spec);

    struct gram_symbol_analysis san = { 0 };
    if (!gram_analyze_symbols(&san, spec))
        return false;

    print_gram_symbol_analysis(stdout, &san);

    struct gram_rule_analysis ran = { 0 };
    if (!gram_analyze_rules(&ran, &san, spec))
        return free_gram_symbol_analysis(&san), false;

    print_gram_rule_analysis(stdout, &ran);
    free_gram_rule_analysis(&ran);

    struct gram_analysis gan = { 0 };
    if (!gram_analyze(&gan, &san, spec))
        return free_gram_symbol_analysis(&san), false;

    print_gram_analysis(stdout, &gan, spec);

    free_gram_analysis(&gan);
    free_gram_symbol_analysis(&san);

    return true;
}

bool scan(void *_1, char *filename, char *contents) {
    UNUSED(_1);
    print_gram_tokens(stdout, contents, filename);
    return true;
}

bool tables(struct args const args) {
    bool result = true;
    if (args.type == LL) result = parse_spec(args, run_ll_tables);
    else result = parse_spec(args, run_lr_tables);
    return result;
}

bool gen_parser(struct args const args) {
    bool result = true;
    if (args.type == LL) result = parse_spec(args, run_ll_parser);
    else result = parse_spec(args, run_lr_parser);
    return result;
}

int main(int argc, char *argv[]) {
    struct args args = {
        .cmd = GEN_PARSER,
        .type = LL,
        .spec = ""
    };

    struct arg parser_type_arg = { PARSER_TYPE, "type", 0, required_argument, "Parser type: ll, slr, lr1" };
    struct arg lr_type_arg = { LR_TYPE, "type", 0, required_argument, "Parser type: slr, lr1" };
    struct arg spec_file_arg = { SPEC_FILE, "spec", 0, required_argument, "Spec file" };
    struct arg text_arg = { TEXT, "text", 0, no_argument, "Print as text" };

    run_args(&args, ARG_FN read_args, "1.0.0", argc, argv, NULL, CMD {
        GEN_PARSER,
        NULL,
        ARGS { spec_file_arg, parser_type_arg, help_and_version_args, END_ARGS },
        NULL,
        CMDS {
            {
                ANALYZE, "analyze",
                ARGS { spec_file_arg, help_and_version_args, END_ARGS },
                NULL,
                NULL,
                "Analyze spec files"
            },
            {
                AUTOMATA, "automata",
                ARGS { spec_file_arg, lr_type_arg, text_arg, help_and_version_args, END_ARGS },
                NULL,
                NULL,
                "Print the LR automaton in dot format"
            },
            {
                SCAN, "scan",
                ARGS { help_and_version_args, END_ARGS },
                NULL,
                NULL,
                "Scan spec file"
            },
            {
                TABLES, "tables",
                ARGS { spec_file_arg, parser_type_arg, help_and_version_args, END_ARGS },
                NULL,
                NULL,
                "Print parser tables as CSV"
            },
            END_CMDS
        },
        "Generate a parser"
    });

    bool result = true;

    if (args.cmd == ANALYZE)         result = parse_spec(args, analyze);
    else if (args.cmd == AUTOMATA)   result = parse_spec(args, automata);
    else if (args.cmd == GEN_PARSER) result = gen_parser(args);
    else if (args.cmd == SCAN)       result = read_files(args, NULL, scan);
    else if (args.cmd == TABLES)     result = tables(args);

    return result ? EXIT_SUCCESS : EXIT_FAILURE;
}
