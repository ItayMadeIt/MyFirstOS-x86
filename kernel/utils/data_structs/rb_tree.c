#include <stdio.h>
#include <utils/data_structs/rb_tree.h>

#define RB_COLOR 0b01
#define RB_FLAGS 0b11

static inline bool is_red(const rb_node_t* node)
{
    if (!node) 
		return false;
    return node->parent_color & RB_COLOR;
}
static inline bool is_black(const rb_node_t* node)
{
	return !is_red(node);
}

static inline void set_red(rb_node_t* node)
{
	node->parent_color |= RB_COLOR;
}
static inline void set_black(rb_node_t* node)
{
	node->parent_color &= ~RB_COLOR;
}
static inline rb_node_t* get_parent(const rb_node_t* node)
{
	usize_ptr parent_addr = node->parent_color & (usize_ptr)(~RB_FLAGS);
	return (rb_node_t*)( parent_addr );
}

static inline void set_parent(rb_node_t* node, const rb_node_t* new_parent)
{
	node->parent_color = (node->parent_color & RB_FLAGS) | (usize_ptr)new_parent;
}

static inline void set_color_to_other(rb_node_t* left, rb_node_t* right)
{
	left->parent_color = (usize_ptr)get_parent(left) | (right->parent_color & RB_COLOR);
}

static inline rb_node_t* get_sibling(const rb_node_t* node)
{
	rb_node_t* parent = get_parent(node);

	if (!parent)
	{
		return NULL;
	}

	return parent->left == node ? parent->right : parent->left;
}

static inline rb_node_t* get_uncle(const rb_node_t* node)
{
	rb_node_t* parent = get_parent(node);
	
	if (!parent)
	{
		return NULL;
	}

	return get_sibling(parent);
}

static void rb_rotate_left(rb_node_t** root, rb_node_t* parent)
{
    rb_node_t* right = parent->right;
    rb_node_t* pivot = right->left;

	// Rotate
	parent->right = pivot;
    if (pivot)
	{
        set_parent(pivot, parent);
	}

	rb_node_t* gparent = get_parent(parent);
	set_parent(right, gparent);

	if (!gparent)
	{
		*root = right;
	}
	else if (gparent->left == parent)
	{
		gparent->left = right;
	}
	else if (gparent->right == parent)
	{
		gparent->right = right;
	}

	right->left = parent;
	set_parent(parent, right);
}


static void rb_rotate_right(rb_node_t** root, rb_node_t* parent)
{
    rb_node_t* left = parent->left;
    rb_node_t* pivot = left->right;

	// Rotate
	parent->left = pivot;
    if (pivot)
	{
        set_parent(pivot, parent);
	}

	rb_node_t* gparent = get_parent(parent);
	set_parent(left, gparent);

	if (!gparent)
	{
		*root = left;
	}
	else if (gparent->left == parent)
	{
		gparent->left = left;
	}
	else if (gparent->right == parent)
	{
		gparent->right = left;
	}

	left->right = parent;
	set_parent(parent, left);
}

static rb_node_t** rb_find_node_link(rb_tree_t* tree, rb_node_t* data, rb_node_t** link_parent) 
{
    rb_node_t** link = &tree->root;
    *link_parent = NULL;

	// Simple BST
    while (*link) 
	{
        *link_parent = *link;

		int result = tree->cmp(*link, data);

		if (result > 0)
		{
            link = &(*link)->left;
		}
		else if (result < 0)
        {
			link = &(*link)->right;
		}
		else
		{
            break;
    	}
	}

    return link;
}


static inline void rb_handle_insert_fixup_rotation(rb_node_t** root, 
		rb_node_t* gparent, rb_node_t* parent, 
		bool parent_left_child, bool node_left_child)
{
	bool is_zigzag = (parent_left_child != node_left_child);

	if (parent_left_child)
	{
		// Ensures it turned into a line
		if (is_zigzag)
		{
			rb_rotate_left(root, parent);
		}

		rb_rotate_right(root, gparent);
	}
	else
	{
		// Ensures it turned into a line
		if (is_zigzag)
		{
			rb_rotate_right(root, parent);
		}

		rb_rotate_left(root, gparent);
	}

	// New top is always parent(gparent)
	rb_node_t* new_top = get_parent(gparent);  
	set_black(new_top);
	set_red(gparent);
}

static void rb_insert_fixup(rb_node_t** root, rb_node_t* node)
{
	rb_node_t* parent = get_parent(node);

	if (parent == NULL)
	{
		set_black(*root); 
		return;
	}

	if (is_black(parent))
	{
		return;
	}

	rb_node_t* gparent = get_parent(parent);

	// If parent is root
	if (gparent == NULL)
	{
		return;
	}

	rb_node_t* uncle = get_uncle(node);

	if (is_red(uncle))
	{
		set_black(uncle);
		set_black(parent);
		set_red(gparent);

		rb_insert_fixup(root, gparent);
	}
	else
	{
		bool parent_left_child = (gparent->left == parent);
		bool node_left_child   = (parent->left == node);

		rb_handle_insert_fixup_rotation(
			root, 
			gparent, parent, 
			parent_left_child, node_left_child
		);
	}

	set_black(*root);
}

static rb_node_t* rb_find_successor(rb_node_t* node)
{
	node = node->right;
	if (!node)
	{
		return NULL;
	}
	while (node->left)
	{
		node = node->left;
	}

	return node;
}

static void rb_transplant(rb_node_t** root, rb_node_t* old_node, rb_node_t* new_node) 
{
	rb_node_t* parent = get_parent(old_node);

	if (parent == NULL) 
	{
        *root = new_node;
    }
	else if (old_node == parent->left) 
	{
        parent->left = new_node;
    }
	else 
	{
        parent->right = new_node;
    }

    if (new_node) 
	{
        set_parent(new_node, parent);
    }
}

static void rb_delete_configure_nuclear_node(rb_node_t** root, rb_node_t* delete_node, bool* delete_was_black, rb_node_t** node_to_fix, rb_node_t** node_to_fix_parent)
{
	rb_node_t* successor_node = rb_find_successor(delete_node);
	*delete_was_black = is_black(successor_node);
	
	*node_to_fix = successor_node->right;

	//         delete               delete/successor
	//         /    \                  /     \
	//        A    successor  ==>     A      right
	//               /  \                     / \
	//             NIL  right               ... ...
	if (get_parent(successor_node) == delete_node)
	{
		*node_to_fix_parent = successor_node;
	}
	//         delete             delete/succesor
	//              \                 \
	//             node               node
	//              /       ===>      / 
	//          successor           right
 	//           /      \            / \
	//         NIL   right        ... ...
	else
	{
		*node_to_fix_parent = get_parent(successor_node);

		// Remove successor_node 
		rb_transplant(root, successor_node, successor_node->right);

		successor_node->right = delete_node->right;

		if (successor_node->right)
		{
			set_parent(successor_node->right, successor_node);
		}
	}

	// Remove delete_node and replace it with successor
	rb_transplant(root, delete_node, successor_node);

	set_color_to_other(successor_node, delete_node);

	successor_node->left = delete_node->left;

	if (successor_node->left)
	{
		set_parent(successor_node->left, successor_node);
	}

	// delete.color = successor.color
	set_color_to_other(delete_node, successor_node);
}

static inline void rb_delete_fixup_sibling_red(
	rb_node_t** root, rb_node_t* parent, rb_node_t* sibling, bool is_left_child)
{
	/* 
 	    Uppercase = Black, Lowercase = Red
	
	    P/p - parent
	    R/r - replacement
        S/s - sibling
	    A/a, B/b, C/c, D/d, E/e - random children

	  	   P                S
	     /   \            /   \
	    R     s   ===>   p     B  
	         / \        / \
	        A   B      R   A
	*/
	
	set_black(sibling);
	set_red(parent);

	if (is_left_child)
	{
		rb_rotate_left(root, parent);
	}
	else
	{
		rb_rotate_right(root, parent);
	}
}

static inline void rb_delete_fixup_sibling_black_child_red(
	rb_node_t** root, rb_node_t* parent, rb_node_t* sibling, bool is_left_child)
{
	// Assumptions:
	// - sibling is black
	// - sibling has a red child (1 or 2)
	// - the replacement(R/r) is black 

	// Case 3: ZigZag case
	/* 
 	    Uppercase = Black, Lowercase = Red
	
	    P.p - parent
	    R.r - replacement
        S.s - sibling
	    A.a, B.b, C.c, D.d, E.e - random children

	  	  P.p              P.p      
	     /   \            /   \    
	    R      S   ===>  R      A     
	         /   \            /   \  
	        a    B.b        C.c    s 
	       / \                    / \
         C.c D.d                 D.d B.b

		After this, A will be turned into the new sibling
	*/
    if (is_left_child && is_red(sibling->left))
    {
		if (sibling->left)
		{
			set_black(sibling->left);
		}

		set_red(sibling);
        rb_rotate_right(root, sibling);
		                
		sibling = parent->right;
    }
    else if (!is_left_child && is_red(sibling->right))
    {
		if (sibling->right)
		{
			set_black(sibling->right);
		}

		set_red(sibling);
        rb_rotate_left(root, sibling);

		sibling = parent->left;
    }

	set_black(parent);

	// Case 4: Line Case:
	// Assumptions:
	// - sibling->left == red if is_right_child 
	//  OR
	//   sibling->right == red if is_left_child
	// 
	// - sibling == black
	// - parent == black

	/*
 	    Uppercase = Black, Lowercase = Red
	
	    P.p - parent
	    R.r - replacement
        S.s - sibling
	    A.a, B.b, C.c, D.d, E.e - random children

             P                       S
            / \                   /    \
           R   S                 P      A(changed)
              / \     ====>     / \    /  \
            C.c  a             R  C.c D.d B.b
                / \
              D.d B.b

		- To ensure it all works, C is still under S and P that have the same colors just in a different order
		- R has now an additional parent(grandparent) that is black, the node S, which solves the double black
		- And A changing to be black makes it so D.d and B.b will act like there were in total still 2 black parents
			Instead of a grandparent and a grand-grandparent, it turned to be a parent and a grandaprent
	*/ 

    if (is_left_child)
    {
        if (sibling->right)
		{
            set_black(sibling->right);
		}
		rb_rotate_left(root, parent);
    }
    else
    {
        if (sibling->left)
        {    
			set_black(sibling->left);
		}
		rb_rotate_right(root, parent);
    }
}

static void rb_delete_fixup(rb_node_t** root, rb_node_t* node_to_fix, rb_node_t* node_to_fix_parent)
{
	while (node_to_fix != *root && is_black(node_to_fix)) 
	{        
		bool is_left_child = (node_to_fix == node_to_fix_parent->left);
        
		rb_node_t* sibling = is_left_child ? node_to_fix_parent->right : node_to_fix_parent->left;

		// Ensure sibling is black, if red fix it
        if (is_red(sibling)) 
        {
			rb_delete_fixup_sibling_red(root, node_to_fix_parent, sibling, is_left_child);
			sibling = is_left_child ? 
					node_to_fix_parent->right : node_to_fix_parent->left;
        }

		// Case 2: Sibling is black with two black children — recolor sibling and move up the fixup
		//          Double black cant be fixed locally, push the probleem upward by recoloring
        if (is_black(sibling->left) && is_black(sibling->right)) 
        {
            set_red(sibling);

            node_to_fix = node_to_fix_parent;
			node_to_fix_parent = get_parent(node_to_fix_parent);
        } 

		// Case 3 & 4: Sibling is black with a red child - there is a way to rotate 
		//              the parent and act upon it to solve the double black, one of the red children, can be used
        else 
        {
			rb_delete_fixup_sibling_black_child_red(root, node_to_fix_parent, sibling, is_left_child);

            node_to_fix = *root;

			break;
        }
    }

    if (node_to_fix)
	{
        set_black(node_to_fix);
	}
}


static void rb_configure_delete(
	rb_node_t** root, rb_node_t* delete,
	bool* delete_was_black,
	rb_node_t** node_to_fix, rb_node_t** node_to_fix_parent)
{
	if (delete->left == NULL) 
	{
		*node_to_fix = delete->right;
		*node_to_fix_parent = get_parent(delete);
		*delete_was_black = is_black(delete);
		rb_transplant(root, delete, delete->right);
	}
	else if (delete->right == NULL) 
	{
		*node_to_fix = delete->left;
		*node_to_fix_parent = get_parent(delete);
		*delete_was_black = is_black(delete);
		rb_transplant(root, delete, delete->left);
	}
	else 
	{
		rb_delete_configure_nuclear_node(root, delete, delete_was_black, node_to_fix, node_to_fix_parent);
	}
}

static void rb_augment_from(rb_tree_t* tree, rb_node_t* node)
{
	while (node)
	{
		tree->augment(node);

		if (tree->root == node)
		{
			break;
		}

		node = get_parent(node);
	}
}

static void dummy_no_agument(rb_node_t* _) {(void)_;}

void rb_init_tree(rb_tree_t *tree, rb_cmp_t cmp, rb_augment_t augment)
{
	tree->root = NULL;
	tree->cmp = cmp;
	tree->augment = augment ? augment : dummy_no_agument;
}

rb_node_t *rb_insert(rb_tree_t *tree, rb_node_t *node) 
{
	rb_node_t** link = &tree->root;
	rb_node_t* parent = NULL;

	link = rb_find_node_link(tree, node, &parent);

	if (*link != NULL)
	{
		return NULL;
	}

	node->right = NULL;
	node->left = NULL;
	
	set_red(node);

	*link = node;

	set_parent(node, parent);

	rb_insert_fixup(&tree->root, node);

	rb_augment_from(tree, *link);

	return node;
}

rb_node_t* rb_remove_key(rb_tree_t* tree, rb_node_t* key)
{
	rb_node_t** link = &tree->root;
	rb_node_t* parent = NULL;

	link = rb_find_node_link(tree, key, &parent);
	
	rb_node_t* delete = *link;

	if (delete == NULL)
	{
		return NULL;
	}

	rb_node_t* node_to_fix = NULL;
	rb_node_t* node_to_fix_parent = NULL;

    bool delete_was_black = is_black(delete);

	// Find the easiest configuration to delete the delete_node
	rb_configure_delete(&tree->root, delete, &delete_was_black, &node_to_fix, &node_to_fix_parent);

	if (delete_was_black)
	{
		rb_delete_fixup(&tree->root, node_to_fix, node_to_fix_parent);
	}
	
	if (node_to_fix)
	{
		rb_augment_from(tree, node_to_fix);
	}
	else if (node_to_fix_parent)
	{	
		rb_augment_from(tree, node_to_fix_parent);
	}

	return delete;
}

rb_node_t *rb_remove_node(rb_tree_t *tree, rb_node_t *delete) 
{ 
	if (delete == NULL)
	{
		return NULL;
	}

	rb_node_t* node_to_fix = NULL;
	rb_node_t* node_to_fix_parent = NULL;

    bool delete_was_black = is_black(delete);

	// Find the easiest configuration to delete the delete_node
	rb_configure_delete(&tree->root, delete, &delete_was_black, &node_to_fix, &node_to_fix_parent);

	if (delete_was_black)
	{
		rb_delete_fixup(&tree->root, node_to_fix, node_to_fix_parent);
	}

	if (node_to_fix)
	{
		rb_augment_from(tree, node_to_fix);
	}
	else if (node_to_fix_parent)
	{	
		rb_augment_from(tree, node_to_fix_parent);
	}

	return delete;
}

rb_node_t *rb_search(rb_tree_t *tree, rb_node_t *key) 
{
	rb_node_t *parent = NULL;

	rb_node_t **link = rb_find_node_link(tree, key, &parent);

	return *link;
}

static inline rb_node_t* rb_get_min(rb_node_t* node)
{
	if (!node)
	{
		return NULL;
	}

	while (node->left)
	{
		node = node->left;
	}
	return node;
}
static inline rb_node_t* rb_get_max(rb_node_t* node)
{
	if (!node)
	{
		return NULL;
	}

	while (node->right)
	{
		node = node->right;
	}
	return node;
}

rb_node_t* rb_min(rb_tree_t * tree)
{
	rb_node_t* node = tree->root;
	return rb_get_min(node);
}

rb_node_t* rb_max(rb_tree_t * tree)
{
	rb_node_t* node = tree->root;
	return rb_get_max(node);
}

rb_node_t *rb_prev(rb_node_t *node)
{
	if (node->left)
	{
		return rb_get_max(node->left);
	}

	while (true)
	{
		rb_node_t* parent = get_parent(node);
        if (!parent)
		{
            return NULL;
		}

		// is right child
		if (parent->right == node)
		{
			return parent;
		}

		node = parent;
	}
}


rb_node_t *rb_next(rb_node_t *node)
{
	if (node->right)
	{
		return rb_get_min(node->right);
	}

	while (true)
	{
		rb_node_t* parent = get_parent(node);
        if (!parent)
		{
            return NULL;
		}

		// is left child
		if (parent->left == node)
		{
			return parent;
		}

		node = parent;
	}
}

void rb_free_tree(rb_tree_t *tree, rb_free_t free_callback)
{
    rb_node_t *node = tree->root;
    rb_node_t *last = NULL;

    while (node) 
	{
		// Left as possible
        if (node->left && last != node->left && last != node->right) 
		{
            node = node->left;
        }
        // If can't, go as right as possible
        else if (node->right && last != node->right) 
		{
            node = node->right;
        }
        // Both children done, free this node
        else 
		{
            rb_node_t *parent = get_parent(node);
        
		    free_callback(node);
        
		    last = node;
            node = parent;
        }
    }

    tree->root = NULL;
}


rb_node_t* rb_lower_bound(rb_tree_t* tree, rb_node_t* key)
{
    rb_node_t* node = tree->root;
    rb_node_t* result = NULL;

    while (node) 
	{
		if (tree->cmp(key, node) <= 0) 
		{
            result = node;
            node = node->left;
        } 
		else 
		{
            node = node->right;
        }
    }
    return result;
}


rb_node_t* rb_upper_bound(rb_tree_t* tree, rb_node_t* key)
{
    rb_node_t* node = tree->root;
    rb_node_t* result = NULL;

    while (node) 
	{
		if (tree->cmp(key, node) >= 0) 
		{
            result = node;
            node = node->right;
        } 
		else 
		{
            node = node->left;
        }
    }
    return result;
}