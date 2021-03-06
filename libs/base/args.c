#include "base/args.h"
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

struct args_context init_args_context(
    struct cmd *cmd, char *version,
    struct args_handlers *handlers,
    struct cmd **cmd_path,
    int argc, char **argv
) {
    return (struct args_context) {
        .cmd = cmd,
        .version = version,
        .handlers = handlers,
        .cmd_path = cmd_path,
        .argc = argc,
        .argv = argv,
        .flag = -1,
        .optstring = NULL,
        .options = NULL,
        .val_table = NULL
    };
}

void run_args(
    void *out_val,
    void (*read_args) (void *out_val, int cmd, struct args_context *context),
    char *version,
    int argc, char **argv,
    struct args_handlers *handlers,
    struct cmd *cmd
) {
    assert(read_args != NULL);

    if (!handlers) (handlers = &(struct args_handlers) {
        .help_found = help_found,
        .version_found = version_found,
        .missing_arg_found = missing_arg_found,
        .unknown_option_found = unknown_option_found
    });

    struct cmd *cmd_path[32] = { NULL };
    struct args_context context = init_args_context(cmd, version, handlers, cmd_path, argc, argv);

    findcmd(&context);

    char optstring[OPTSTRING_SIZE] = "";
    struct option options[OPTIONS_SIZE];
    struct val_assoc val_table[OPTIONS_SIZE];
    determine_options(&context, optstring, options, val_table);

    (*read_args)(out_val, context.cmd->key, &context);
}

void findcmd(struct args_context *context) {
    struct cmd *cmd = context->cmd;
    struct cmd **cmd_path = context->cmd_path;
    int argc = context->argc;
    char **argv = context->argv;
    char *prog = argv[0];
    struct cmd *sub = cmd->subcmds;

    *cmd_path++ = cmd;

    while (sub && argc > 1 && sub->key != END) {
        if (strcmp(sub->cmd, argv[1]) == 0) {
            cmd = sub;
            *cmd_path++ = cmd;
            sub = cmd->subcmds;
            argv++;
            argc--;
            argv[0] = prog;
        } else {
            sub++;
        }
    }

    context->argc = argc;
    context->argv = argv;
    context->cmd = cmd;
}

struct option arg_to_option(struct arg *arg, int *flag) {
    return (struct option) {
        .name = arg->lname,
        .has_arg = arg->has_val,
        .flag = flag,
        .val = arg->key
    };
}

void determine_options(struct args_context *context, char *optstring, struct option *options, struct val_assoc *val_table) {
    context->optstring = optstring;
    context->options = options;
    context->val_table = val_table;

    struct arg *arg = context->cmd->args;

    // leading ':' suppresses error and causes ':' to be returned
    // when there's a missing option argument, i.e. foo missing in -x foo
    *optstring++ = ':';

    while (arg && arg->key != END) {
        *options++ = arg_to_option(arg, &context->flag);

        if (arg->sname) {
            *optstring++ = arg->sname;

            if (arg->has_val == optional_argument || arg->has_val == required_argument) {
                *optstring++ = ':';
            }

            *val_table++ =  (struct val_assoc) { arg->sname, arg->key };
        }

        arg++;
    }

    *options++ = (struct option) { NULL, 0, NULL, 0 };

    *val_table++ = (struct val_assoc) { '?', UNKNOWN_OPTION };
    *val_table++ = (struct val_assoc) { ':', MISSING_ARG };
    *val_table++ = (struct val_assoc) { END, END };
}

int readarg(struct args_context *context) {
    int key = getopt_long(context->argc, context->argv, context->optstring, context->options, NULL);

    if (key == 0) {
        key = context->flag;
    } else {
        struct val_assoc *val_table = context->val_table;

        while (key != END && val_table->key != END) {
            if (val_table->key == key) {
                key = val_table->val;
                break;
            }

            val_table++;
        }
    }

    struct args_handlers *handlers = context->handlers;

    void (*handler) (struct args_context *) = NULL;

    switch (key) {
        case HELP:
            if (handlers->help_found) handler = *handlers->help_found;
            break;
        case VERSION:
            if (handlers->version_found) handler = handlers->version_found;
            break;
        case MISSING_ARG:
            if (handlers->missing_arg_found) handler = *handlers->missing_arg_found;
            break;
        case UNKNOWN_OPTION:
            if (handlers->unknown_option_found) handler = handlers->unknown_option_found;
            break;
    }

    if (handler) (*handler)(context);

    return key;
}

int numarg(struct args_context *context) {
    char *end = optarg;
    int n = strtol(optarg, &end, 10);

    if (optarg == end)
        print_usage_exit(stderr, context);

    return n;
}

char *argval() {
    return optarg;
}

int argc(struct args_context *context) {
    return context->argc - optind;
}

char **argv(struct args_context *context) {
    return context->argv + optind;
}

void print_usage(FILE *handle, struct args_context *context) {
    struct cmd **cmd_path = context->cmd_path + 1;
    struct cmd *cmd = context->cmd;
    char *prog = context->argv[0];
    struct cmd *sub = cmd->subcmds;

    fprintf(handle, "usage: %s", prog);
    while (*cmd_path != NULL) {
        fprintf(handle, " %s", (*cmd_path)->cmd);
        cmd_path++;
    }
    if (sub) fprintf(handle, " [subcommand]");
    fprintf(handle, " [options]\n\n");
    fprintf(handle, "%s\n\n", cmd->desc);

    // subcommands
    if (sub) {
        fprintf(handle, "subcommands:\n");

        while (sub->key != END) {
            fprintf(handle, SUBCMD_HELP_FMT, sub->cmd, sub->desc);
            fprintf(handle, "\n");
            sub++;
        }
        fprintf(handle, "\n");
    }

    // arguments
    struct arg *arg = cmd->args;
    if (arg) {
        fprintf(handle, "options:\n");
        while (arg->key != END) {
            char flag[128] = "";
            char *f = flag;

            if (arg->sname) {
                *f++ = '-';
                *f++ = arg->sname;
                if (arg->lname) *f++ = ',';
            }

            if (arg->lname) {
                *f++ = '-';
                *f++ = '-';
                strcpy(f, arg->lname);
            }

            fprintf(handle, ARG_HELP_FMT, flag, arg->desc);
            fprintf(handle, "\n");
            arg++;
        }

        fprintf(handle, "\n");
    }

    struct env_var *var = cmd->vars;
    if (var) {
        fprintf(handle, "environment variables:\n");
        while (var->name != NULL) {
            fprintf(handle, VAR_HELP_FMT, var->name, var->desc);
            fprintf(handle, "\n");
            var++;
        }

        fprintf(handle, "\n");
    }
}

void print_usage_exit(FILE *handle, struct args_context *context) {
    print_usage(handle, context);
    exit(EXIT_FAILURE);
}

void print_version(struct args_context *context) {
    printf("%s\n", context->version ? context->version : "(missing)");
}

void help_found(struct args_context *context) {
    print_usage(stdout, context);
    exit(EXIT_SUCCESS);
}

void version_found(struct args_context *context) {
    print_version(context);
    exit(EXIT_SUCCESS);
}

void missing_arg_found(struct args_context *context) {
    fprintf(stderr, "missing arg\n");
    print_usage(stderr, context);
    exit(EXIT_FAILURE);
}

void unknown_option_found(struct args_context *context) {
    fprintf(stderr, "unknown option\n");
    print_usage(stderr, context);
    exit(EXIT_FAILURE);
}


