#ifndef TEST_BASE_BTREE_DOT_H__
#define TEST_BASE_BTREE_DOT_H_ 1

#include <cgraph.h>
#include <base/rbtree.h>

Agnode_t *node_to_agnode(Agraph_t *graph, struct rb_node *node, void (*key_to_str)(char *out, void const *key));
void btree_to_graph(struct rb_node *node, void (*key_to_str)(char *out, void const *key));
void graph_int_tree();

#endif // TEST_BASE_BTREE_DOT_H_
