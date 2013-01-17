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
#include <stdarg.h>
#include <string.h>
#include "pipo.h"
#include "tree.h"
#include "global.h"

/* Variable that is going to be increased every
   time when an error is happening.  */
int error_count = 0;

/* Variable that is going to be increased every
   time when an error is happening.  */
int warning_count = 0;

/* A global list to store modules.  */
tree module_list = NULL;

/* Trees that are to be deleted.  */
tree delete_list = NULL;

/* Allocate all the global structures that are going to be used
   during the compilation.  */
void
init_global ()
{
  module_list = make_tree_list ();
  delete_list = make_tree_list ();
  error_count = 0;
  warning_count = 0;
}

void
finalize_global ()
{
  free_tree (delete_list);
  free_tree (module_list);
}

void
init_global_tree ()
{
  global_tree[TG_ERROR_MARK] = (tree) malloc (sizeof (struct tree_base));
  TREE_CODE_SET (global_tree[TG_ERROR_MARK], ERROR_MARK);
}

void
finalize_global_tree ()
{
  int i;
  for (i = 0; i < TG_MAX; i++)
    if (global_tree[i] == error_mark_node)
      free (global_tree[i]);
    else
      free_tree (global_tree[i]);
}

int
compare_ints (const void *a, const void *b)
{
  const int *ia = (const int *) a;
  const int *ib = (const int *) b;
  return (*ia > *ib) - (*ia < *ib);
}

tree
module_exists (tree list, const char *str)
{
  struct tree_list_element *tl;

  DL_FOREACH (TREE_LIST (list), tl)
    {
      if (strcmp (TREE_VALUE (((TREE_OPERAND (tl->entry, 0)))),
		str) == 0)
	return tl->entry;
    }

  return NULL;
}
