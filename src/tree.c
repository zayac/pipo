/* Copyright (c) 2013 Pavel Zaichenkov <zaichenkov@gmail.com>

   Permission to use, copy, modify, and distribute this software for any
   purpose with or without fee is hereby granted, provided that the above
   copyright notice and this permission notice appear in all copies.

   THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
   WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
   MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
   ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
   WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
   ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
   OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.  */

#include <stdio.h>

#include "tree.h"
#include "global.h"

#undef DEF_TREE_CODE

#define DEF_TREE_CODE(code, desc, operands) operands,
unsigned char tree_code_operand[] = {
#include "tree.def"
};

#undef DEF_TREE_CODE

#define DEF_TREE_CODE(code, desc, operands) desc,
const char *tree_code_name[] = {
#include "tree.def"
};

#undef DEF_TREE_CODE

/* Table to store tree nodes that should be allocated only once.  */
tree global_tree[TG_MAX];

/* When freeing a tree node atomic objects like
   identifiers and string constants can have multiple links.
   That is why when freeing an atomic object we change the code
   of the tree to EMPTY_MARK and save this tree int the
   atomic_trees.  */
static tree *atomic_trees = NULL;
static size_t atomic_trees_size = 0;
static size_t atomic_trees_idx = 0;

static size_t
get_tree_size (enum tree_code code)
{
  size_t size, ops;
  ops = TREE_CODE_OPERANDS (code) * sizeof (tree);
  size =  ops ? sizeof (struct tree_base_op)
	      : sizeof (struct tree_base);

  switch (code)
    {
//      case IDENTIFIER:
//	return ops + sizeof (struct tree_identifier_node);
      case LIST:
	return ops + sizeof (struct tree_list_node);
      case ERROR_MARK:
	return 0;
      case VALUE:
	return ops + sizeof (struct tree_value_node);
      default:
	return size + ops;
    }
}

tree
make_tree (enum tree_code code)
{
  size_t size = get_tree_size (code);

  if (code == ERROR_MARK)
    warning ("attempt to allocate ERRO_MARK_NODE; pointer returned");

  tree ret = (tree) malloc (size);
  memset (ret, 0, size);
  TREE_CODE_SET (ret, code);
  return ret;
}


static inline void
atomic_trees_add (tree t)
{
  assert (TREE_CODE (t) == EMPTY_MARK,
	  "only EMPTY_MARK nodes can be added to atomic_tres");

  if (atomic_trees_size == 0)
    {
      const size_t initial_size = 32;

      atomic_trees = (tree *) malloc (initial_size * sizeof (tree));
      atomic_trees_size = initial_size;
    }

  if (atomic_trees_idx == atomic_trees_size)
    {
      atomic_trees =
	(tree *) realloc (atomic_trees,
			  2 * atomic_trees_size * sizeof (tree));
      atomic_trees_size *= 2;
    }

  /* Most likely we don't need to search anything.  */

  {/* For testing purposes only.  */
    size_t i;
    for (i = 0; i < atomic_trees_idx; i++)
      if (atomic_trees[i] == t)
	unreachable ("double insert of node in atomic_trees");
  }

  atomic_trees[atomic_trees_idx++] = t;
  //printf ("-- atomix_idx = %i\n", (int)atomic_trees_idx);
}


void
free_atomic_trees ()
{
  size_t i;

  for (i = 0; i < atomic_trees_idx; i++)
    {
      assert (TREE_CODE (atomic_trees[i]) == EMPTY_MARK, 0);
      free (atomic_trees[i]);
    }
  if (atomic_trees)
    free (atomic_trees);

  atomic_trees = NULL;
  atomic_trees_size = 0;
  atomic_trees_idx = 0;
}


void
free_tree (tree node)
{
  int i;
  enum tree_code code;
  if (node == NULL
      || node == error_mark_node || TREE_CODE (node) == EMPTY_MARK)
    return;

  code = TREE_CODE (node);

  switch (code)
    {
      case LIST:
	{
	  struct tree_list_element *el = NULL, *tmp = NULL;
	  DL_FOREACH_SAFE (TREE_LIST (node), el, tmp)
	  {
	    DL_DELETE (TREE_LIST (node), el);
	    free_tree (el->entry);
	    free (el);
	  }
	}
	break;
      case VALUE:
	{
	  free (TREE_VALUE (node));
	}
	break;
      case FUNCTION:
	{

	}
	break;
      case MODULE:
	{

	}
	break;
      default:
	unreachable (0);
      break;
    }

  for (i = 0; i < TREE_CODE_OPERANDS (code); i++)
    {
      free_tree (TREE_OPERAND (node, i));
      TREE_OPERAND_SET (node, i, NULL);
    }

  TREE_CODE_SET (node, EMPTY_MARK);
  atomic_trees_add (node);
}

tree
make_value_str (const char *value)
{
  tree t;
  assert (value != NULL, 0);
  t = make_tree (VALUE);
  TREE_VALUE (t) = strdup (value);
  TREE_VALUE_LENGTH (t) = strlen (value);
  return t;
}

tree
make_value_tok (struct token * tok)
{
  tree t;
  const char *str;
  str = token_as_string (tok);
  t = make_value_str (str);
  TREE_LOCATION (t) = token_location (tok);
  return t;
}
#if 0
tree
make_identifier_tok (struct token * tok)
{
  tree t;

  t = make_tree (IDENTIFIER);
  TREE_ID_NAME (t) = make_value_tok (tok);
  TREE_LOCATION (t) = token_location (tok);
  return t;
}
#endif
tree
make_tree_list ()
{
  tree t = make_tree (LIST);
  TREE_LIST (t) = NULL;
  return t;
}

bool
tree_list_append (tree list, tree elem)
{
  struct tree_list_element *el;
  assert (TREE_CODE (list) == LIST, "appending element of type `%s'",
	  TREE_CODE_NAME (TREE_CODE (list)));

  el =
    (struct tree_list_element *) malloc (sizeof (struct tree_list_element));

  assert (el != NULL, "Can't allocate enough memory for new element `%s'",
	  TREE_CODE_NAME (TREE_CODE (list)));
  el->entry = elem;

  DL_APPEND (TREE_LIST (list), el);
  return true;
}

tree
make_binary_op (enum tree_code code, tree lhs, tree rhs)
{
  tree t;

  t = make_tree (code);
  TREE_OPERAND_SET (t, 0, lhs);
  TREE_OPERAND_SET (t, 1, rhs);

  if (lhs != NULL)
    TREE_LOCATION (t) = TREE_LOCATION (lhs);
  return t;
}

void
free_list (tree lst)
{
  struct tree_list_element *ptr;
  struct tree_list_element *tmpptr;

  if (lst == NULL)
    return;

  ptr = TREE_LIST (lst);
  while (ptr != NULL)
    {
      tmpptr = ptr->next;
      if (ptr)
	free (ptr);
      ptr = tmpptr;
    }
  free (lst);
}

tree
eliminate_list (tree expr)
{
  tree tmp = expr;
  assert (TREE_CODE (expr) == LIST, "list tree expected");
  expr = TREE_LIST (expr)->entry;
  free_list (tmp);
  return expr;
}
