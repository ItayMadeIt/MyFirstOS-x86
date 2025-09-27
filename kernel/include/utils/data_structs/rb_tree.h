#ifndef __TREE_INT_H__
#define __TREE_INT_H__

#include <core/defs.h>


typedef struct __attribute__((aligned(sizeof(uintptr_t)))) rb_node {

	uintptr_t parent_color;
	struct rb_node* left;
	struct rb_node* right;

} rb_node_t;

typedef int  (*rb_cmp_t    )(const rb_node_t* a, const rb_node_t* b);
typedef void (*rb_augment_t)(rb_node_t* var);
typedef void (*rb_free_t)  (rb_node_t* var);

typedef struct rb_tree 
{
	rb_node_t* root;
	rb_cmp_t cmp;
	rb_augment_t augment;
} rb_tree_t;

void rb_init_tree(rb_tree_t *tree, rb_cmp_t cmp, rb_augment_t augment);

static inline bool rb_empty(const rb_tree_t *tree) { return tree->root == NULL; }

rb_node_t* rb_insert     (rb_tree_t* tree, rb_node_t* node);
rb_node_t* rb_remove_key (rb_tree_t* tree, rb_node_t* key);
rb_node_t* rb_remove_node(rb_tree_t* tree, rb_node_t* node);
rb_node_t* rb_search     (rb_tree_t* tree, rb_node_t* key);

void rb_free_tree(rb_tree_t* tree, rb_free_t free_callback);

rb_node_t* rb_min (rb_tree_t* tree);
rb_node_t* rb_max (rb_tree_t* tree);
rb_node_t* rb_prev(rb_node_t* node);
rb_node_t* rb_next(rb_node_t* node);

#define rb_for_each(tree, node) for (node = rb_min(tree); node; node = rb_next(node))

#endif // __TREE_INT_H__