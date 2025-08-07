#include <memory/tree_int.h>

#include <stdio.h>

#define RB_COLOR 0b01
#define RB_FLAGS 0b11

static bool is_red(const node_t* node)
{
    if (!node) return false;
    return node->parent_color & RB_COLOR;
}
static inline bool is_black(const node_t* node)
{
	return !is_red(node);
}

static inline void set_red(node_t* node)
{
	node->parent_color |= RB_COLOR;
}
static inline void set_black(node_t* node)
{
	node->parent_color &= ~RB_COLOR;
}
static node_t* get_parent(const node_t* node)
{
	uint32_t parent_addr = node->parent_color & (uint32_t)(~RB_FLAGS);

	return (node_t*)( parent_addr );
}

static inline void set_parent(node_t* node, const node_t* new_parent)
{
	node->parent_color = (node->parent_color & RB_FLAGS) | (uint32_t)new_parent;
}

static inline void set_color_to_other(node_t* left, node_t* right)
{
	left->parent_color = (uint32_t)get_parent(left) | (right->parent_color & RB_COLOR);
}

static inline void set_color_parent(node_t* node, const uint32_t new_parent_color)
{
	node->parent_color = new_parent_color;
}

static inline node_t* get_sibling(const node_t* node)
{
	node_t* parent = get_parent(node);

	if (!parent)
	{
		return NULL;
	}

	return parent->left == node ? parent->right : parent->left;
}

static inline node_t* get_uncle(const node_t* node)
{
	node_t* parent = get_parent(node);
	
	if (!parent)
	{
		return NULL;
	}

	return get_sibling(parent);
}

static void rb_int_rotate_left(node_t** root, node_t* parent)
{
    node_t* right = parent->right;
    node_t* pivot = right->left;

	// Rotate
	parent->right = pivot;
    if (pivot)
	{
        set_parent(pivot, parent);
	}

	node_t* gparent = get_parent(parent);
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


static void rb_int_rotate_right(node_t** root, node_t* parent)
{
    node_t* left = parent->left;
    node_t* pivot = left->right;

	// Rotate
	parent->left = pivot;
    if (pivot)
	{
        set_parent(pivot, parent);
	}

	node_t* gparent = get_parent(parent);
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

static node_t** find_node_link(node_t** root, int_data_t data, node_t** link_parent) 
{
    node_t** link = root;
    *link_parent = NULL;

	// Simple BST
    while (*link) 
	{
        *link_parent = *link;

        if (data.begin < (*link)->data.begin)
		{
            link = &(*link)->left;
		}
		else if (data.end >= (*link)->data.end)
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


static inline void handle_insert_fixup_rotation(node_t** root, 
		node_t* gparent, node_t* parent, 
		bool is_left_child, bool is_zigzag )
{
	if (is_left_child)
	{
		// Ensures it turned into a line
		if (is_zigzag)
		{
			rb_int_rotate_left(root, parent);
		}
		
		rb_int_rotate_right(root, gparent);
	}
	else
	{
		// Ensures it turned into a line
		if (is_zigzag)
		{
			rb_int_rotate_right(root, parent);
		}

		rb_int_rotate_left(root, gparent);
	}
}

static void rb_int_insert_fixup(node_t** root, node_t* node)
{
	node_t* parent = get_parent(node);

	if (parent == NULL)
	{
		set_black(*root); 
		return;
	}

	if (is_black(parent))
	{
		return;
	}

	node_t* gparent = get_parent(parent);

	// If parent is root
	if (gparent == NULL)
	{
		return;
	}

	node_t* uncle = get_uncle(node);

	if (is_red(uncle))
	{
		set_black(uncle);
		set_black(parent);
		set_red(gparent);

		rb_int_insert_fixup(root, gparent);
	}
	else
	{
		bool parent_left_child = gparent->left == parent;
		bool is_left_child = parent->left == node;

		handle_insert_fixup_rotation(root, gparent, parent, is_left_child, parent_left_child != is_left_child);

		set_black(parent);
		set_red(gparent);
	}

	set_black(*root);
}

node_t* rb_int_insert(node_t** root, node_t* node)
{
	node_t** link = root;
	node_t* parent = NULL;

	link = find_node_link(root, node->data, &parent);

	if (*link != NULL)
	{
		return NULL;
	}

	node->right = NULL;
	node->left = NULL;
	
	set_red(node);

	*link = node;

	rb_int_insert_fixup(root, node);

	return node;
}

static node_t* rb_int_find_successor(node_t* node)
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

static void transplant(node_t** root, node_t* old_node, node_t* new_node) 
{
	node_t* parent = get_parent(old_node);

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

void delete_configure_nuclear_node(node_t** root, node_t* delete_node, bool* delete_was_black, node_t** node_to_fix, node_t** node_to_fix_parent)
{
	node_t* successor_node = rb_int_find_successor(delete_node);
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
		transplant(root, successor_node, successor_node->right);

		successor_node->right = delete_node->right;

		if (successor_node->right)
		{
			set_parent(successor_node->right, successor_node);
		}
	}

	// Remove delete_node and replace it with successor
	transplant(root, delete_node, successor_node);

	set_color_to_other(successor_node, delete_node);

	successor_node->left = delete_node->left;

	if (successor_node->left)
	{
		set_parent(successor_node->left, successor_node);
	}

	// delete.color = successor.color
	set_color_to_other(delete_node, successor_node);
}

static inline void delete_fixup_sibling_red(
	node_t** root, node_t* parent, node_t* sibling, bool is_left_child)
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
		rb_int_rotate_left(root, parent);
	}
	else
	{
		rb_int_rotate_right(root, parent);
	}
}

static inline void delete_fixup_sibling_black_child_red(
	node_t** root, node_t* parent, node_t* sibling, bool is_left_child)
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
        rb_int_rotate_right(root, sibling);
		                
		sibling = parent->right;
    }
    else if (!is_left_child && is_red(sibling->right))
    {
		if (sibling->right)
		{
			set_black(sibling->right);
		}

		set_red(sibling);
        rb_int_rotate_left(root, sibling);

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
		rb_int_rotate_left(root, parent);
    }
    else
    {
        if (sibling->left)
        {    
			set_black(sibling->left);
		}
		rb_int_rotate_right(root, parent);
    }
}

void delete_fixup(node_t** root, node_t* node_to_fix, node_t* node_to_fix_parent)
{
	while (node_to_fix != *root && is_black(node_to_fix)) 
	{        
		bool is_left_child = (node_to_fix == node_to_fix_parent->left);
        
		node_t* sibling = is_left_child ? node_to_fix_parent->right : node_to_fix_parent->left;

		// Ensure sibling is black, if red fix it
        if (is_red(sibling)) 
        {
			delete_fixup_sibling_red(root, node_to_fix_parent, sibling, is_left_child);
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
			delete_fixup_sibling_black_child_red(root, node_to_fix_parent, sibling, is_left_child);

            node_to_fix = *root;

			break;
        }
    }

    if (node_to_fix)
	{
        set_black(node_to_fix);
	}
}


void rb_int_configure_delete(
	node_t** root, node_t* delete,
	bool* delete_was_black,
	node_t** node_to_fix, node_t** node_to_fix_parent)
{
	if (delete->left == NULL) 
	{
		*node_to_fix = delete->right;
		*node_to_fix_parent = get_parent(delete);
		*delete_was_black = is_black(delete);
		transplant(root, delete, delete->right);
	}
	else if (delete->right == NULL) 
	{
		*node_to_fix = delete->left;
		*node_to_fix_parent = get_parent(delete);
		*delete_was_black = is_black(delete);
		transplant(root, delete, delete->left);
	}
	else 
	{
		delete_configure_nuclear_node(root, delete, delete_was_black, node_to_fix, node_to_fix_parent);
	}
}


node_t* rb_int_delete(node_t** root, data_t data)
{
	node_t** link = root;
	node_t* parent = NULL;

	link = find_node_link(root, data, &parent);
	
	node_t* delete = *link;

	if (delete == NULL)
	{
		return NULL;
	}

	node_t* node_to_fix = NULL;
	node_t* node_to_fix_parent = NULL;

    bool delete_was_black = is_black(delete);

	// Find the easiest configuration to delete the delete_node
	rb_int_configure_delete(root, delete, &delete_was_black, &node_to_fix, &node_to_fix_parent);

	if (delete_was_black)
	{
		delete_fixup(root, node_to_fix, node_to_fix_parent);
	}

	return delete;
}

node_t* rb_int_search(node_t** root, data_t data)
{
	node_t* parent = NULL;

	node_t** link = find_node_link(root, data, &parent);

	return *link;
}

bool rb_int_overlap(node_t** root, uint32_t addr)
{
	node_t* parent = NULL;

	node_t** link = find_node_link(root, (int_data_t){addr, addr, 0}, &parent);

	return *link != NULL;
}