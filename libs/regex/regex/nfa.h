#ifndef REGEX_NFA_H_
#define REGEX_NFA_H_ 1

/**
 * Data Structure Options:
 *
 * edge-labeled graph (adjacency list) where
 *  verticies are numbered states containing a list of at most 2 transitions
 *  edges are transitions, labeled by epsilon or the transition symbol, to another vertex
 *
 * edge-labeled graph (adjacency matrix) where
 *  struct transition graph[MAX_STATES][MAX_STATES]
 *  verticies are numbered states where the number is an index
 *  edges contain the index of the state pointed to
 */

#include <stdbool.h>
#include <base/list.h>
#include "result_types.h"
#include "parser.h"

#define STATE_MAX 10000

struct nfa_error {
    struct parse_error perror;
};

struct nfa_context {
    struct nfa_state *statebuf;
    size_t numstates;
    struct nfa_range *rangebuf;
    size_t numranges;
    struct nfa nfa;
    bool has_error;
    struct nfa_error error;
    bool use_nonrec;
};

// nfa state constructors
struct nfa_state accepting_state();
struct nfa_state epsilon_state(struct nfa_state *next);
struct nfa_state dotall_state(struct nfa_state *next);
struct nfa_state branch_state(struct nfa_state *left, struct nfa_state *right);
struct nfa_state range_state(struct nfa_range range);
struct nfa_state symbol_state(char symbol);

// nfa context
struct nfa_context nfa_context(struct nfa_state *statebuf, bool use_nonrec);
struct nfa_state *setst(struct nfa_context *context, struct nfa_state state);
void point(struct nfa *machine, struct nfa_state **end, struct nfa_state **end1);
void patch(struct nfa machine, struct nfa_state *state);
void smachine(struct nfa_context *context, struct nfa machine);

// nfa machine constructors
struct nfa gmachine(struct nfa_context *context);
union regex_result nfa_to_result(struct nfa_context *context);
struct nfa empty_machine(struct nfa_context *context);
struct nfa symbol_machine(struct nfa_context *context, char symbol);
struct nfa alt_machine(struct nfa_context *context, struct nfa left, struct nfa right);
struct nfa cat_machine(struct nfa first, struct nfa second);
struct nfa closure_machine(struct nfa_context *context, struct nfa inner);

// construct and simulate an nfa
struct nfa_context *nfa_regex(char *regex, struct nfa_context *context);
bool has_nfa_error(struct nfa_context *context);
struct nfa_error nfa_error(struct nfa_context *context);
void print_nfa_error(struct nfa_error error);
void free_nfa_context(struct nfa_context *context);
void eps_closure(struct list *nstates, struct nfa_state *state, bool *already_on);
void move(struct list *nstates, struct list *cstates, char c, bool *already_on);
bool accepts(struct list *cstates, struct nfa_state *accept);
bool nfa_match(char *str, struct nfa_context *context);

// parse actions
bool noop_nfa(union regex_result _, struct parse_context *context);
bool do_empty_nfa(union regex_result _, struct parse_context *context);
bool do_alt_nfa(union regex_result nfa, struct parse_context *context);
bool do_cat_nfa(union regex_result nfa, struct parse_context *context);
bool do_dotall_nfa(union regex_result _, struct parse_context *context);
bool do_symbol_nfa(union regex_result sym, struct parse_context *context);
bool do_star_nfa(union regex_result _, struct parse_context *context);
bool do_plus_nfa(union regex_result _, struct parse_context *context);
bool do_optional_nfa(union regex_result _, struct parse_context *context);

// parse action table
extern bool (*nfa_actions[NUM_ACTIONS])(union regex_result val, struct parse_context *context);

// debugging tools
void print_nfa_states(struct list *cstates);
void print_state(struct nfa_state *state);
void print_state_table(struct nfa_state *start, struct nfa_state *end);

#endif // REGEX_NFA_H_
