#ifndef __TREE_INT_H__
#define __TREE_INT_H__

#include <core/defs.h>

typedef struct int_data
{
	uintptr_t begin;
	uintptr_t end;

	uint8_t permissions;
	uint8_t type;

} int_data_t;

typedef int_data_t data_t;

typedef struct node_int {

	uintptr_t parent_color;
	struct node_int* left;
	struct node_int* right;

	int_data_t data;

} node_int_t __attribute__((aligned(sizeof(uintptr_t))));

typedef node_int_t node_t;

node_t* rb_int_insert(node_int_t** root, node_int_t* node);
node_t* rb_int_delete(node_int_t** root, int_data_t data);
node_t* rb_int_search(node_int_t** root, int_data_t data);
bool rb_int_overlap(node_int_t** root, uintptr_t addr);
 
 #endif // __TREE_INT_H__