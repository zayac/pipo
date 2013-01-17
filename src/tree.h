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

#ifndef __TREE_H__
#define __TREE_H__

#include <stdlib.h>
#include "pipo.h"
#include "utlist.h"

#define DEF_TREE_CODE(code, desc, operands) code,
enum tree_code
{
#include "tree.def"
};
#undef DEF_TREE_CODE

extern unsigned char tree_code_operand[];
#define TREE_CODE_OPERANDS(code) tree_code_operand[(int) (code)]

extern bool tree_code_typed[];
#define TREE_CODE_TYPED(code) tree_code_typed[(int) (code)]
extern const char *tree_code_name[];
#define TREE_CODE_NAME(code) tree_code_name[(int) (code)]

union tree_node;
typedef union tree_node *tree;


/* Basic information each node should have.  */
struct tree_base
{
  struct location loc;
  enum tree_code code;
};

/* Base tree with operands pointer */
struct tree_base_op
{
  struct tree_base base;
  tree operands[];
};

struct tree_list_element
{
  tree entry;
  struct tree_list_element *next, *prev;
};

struct tree_list_node
{
  struct tree_base base;
  struct tree_list_element *list;
};

struct tree_value_node
{
  struct tree_base base;
  char *value;
  int length;
};
#if 0
struct tree_identifier_node
{
  struct tree_base base;
  tree name;
};
#endif
union tree_node
{
  struct tree_base base;
  struct tree_base_op base_op;
//  struct tree_identifier_node identifier_node;
  struct tree_list_node list_node;
  struct tree_value_node value_node;
};

enum tree_global_code
{
  TG_ERROR_MARK,
  TG_UNKNOWN_MARK,
  TG_MAX
};

#define error_mark_node     global_tree[TG_ERROR_MARK]
#define unknown_mark_node   global_tree[TG_UNKNOWN_MARK]

#define TREE_LIST(node) ((node)->list_node.list)
#define TREE_CODE(node) ((enum tree_code) (node)->base.code)
#define TREE_LOCATION(node) ((node)->base.loc)
#define TREE_CODE_SET(node, value) ((node)->base.code = (value))

/* Checks if it is possible to access the operand number IDX
   in the node with the code CODE.  */
static inline bool
tree_operand_in_range (enum tree_code code, const int idx)
{
  return idx >= 0 && idx <= TREE_CODE_OPERANDS (code) - 1;
}

/* Returns the operand of the NODE at index IDX checking
   that index is in the range of the node.  */
static inline tree
get_tree_operand (tree node, int idx)
{
  enum tree_code code = TREE_CODE (node);

  assert (tree_operand_in_range (code, idx),
	  "operand index out of range or no operands in the node");

  if (TREE_CODE_OPERANDS (code) > 0)
    return node->base_op.operands[idx];
  else
    unreachable ("node `%s` doesn't have operands", TREE_CODE_NAME (code));
}

static inline void
set_tree_operand (tree node, int idx, tree value)
{
  enum tree_code code = TREE_CODE (node);
  assert (tree_operand_in_range (code, idx),
	  "operand index out of range or no operands in the node");

  if (TREE_CODE_OPERANDS (code) > 0)
    node->base_op.operands[idx] = value;
  else
    unreachable ("node `%s` does not have operands", TREE_CODE_NAME (code));
}

#define TREE_OPERAND(node, i) get_tree_operand ((node), (i))
#define TREE_OPERAND_SET(node, i, value) set_tree_operand ((node), (i), (value))

//#define TREE_ID_NAME(node) ((node)->identifier_node.name)
#define TREE_VALUE(node) ((node)->value_node.value)
#define TREE_VALUE_LENGTH(node) ((node)->value_node.length)

tree make_tree (enum tree_code);
void free_tree (tree);
void free_atomic_trees (void);
tree make_value_tok (struct token *);
tree make_value_str (const char *);
//tree make_identifier_tok (struct token *);
tree make_tree_list (void);
bool tree_list_append (tree, tree);
tree make_binary_op (enum tree_code, tree, tree);
void free_list (tree);
tree eliminate_list (tree);

#endif /* __TREE_H__  */

