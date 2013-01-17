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
#include <err.h>

#include "pipo.h"
#include "tree.h"
#include "global.h"
#include "codegen.h"

int
codegen_atomic_value (FILE* f, tree t)
{
  struct tree_list_element *el;
  assert (TREE_CODE (t) == LIST, "list expected");
  DL_FOREACH (TREE_LIST (t), el)
    {
      fprintf (f, "%s", TREE_VALUE (el->entry));
      if (el->next != NULL)
	fprintf (f, ", ");
    }
  return 0;
}

int
codegen (char *file)
{
  struct tree_list_element *tl, *tll, *tlll;
  int function_error = 0;
  FILE* f;
  const char* extension = ".py";
  char* filename = NULL;
  if (-1 == asprintf (&filename, "%s%s", file, extension))
    err (EXIT_FAILURE, "asprintf failed");
  
  /* File to write files to.  */
  if ((f = fopen (filename, "w")) == NULL)
    {
      fprintf (stderr, "Can't open file `%s' for writing", filename);
      return 1;
    }

  fprintf (f, "import unittest\n");
  fprintf (f, "from ctypes import cdll\n");
  DL_FOREACH (TREE_LIST (module_list), tl)
    fprintf (f, "import %s\n", TREE_VALUE (TREE_OPERAND (tl->entry, 0)));

  DL_FOREACH (TREE_LIST (module_list), tl)
    {
      fprintf (f, "class Test_%s(unittest.TestCase):\n"
		  "\tdef setUp(self):\n"
		  "\t\tself.lib = cdll.LoadLibrary('./lib%s.so')\n",
		  TREE_VALUE (TREE_OPERAND (tl->entry, 0)),
		  TREE_VALUE (TREE_OPERAND (tl->entry, 0)));
      DL_FOREACH (TREE_LIST (TREE_OPERAND (tl->entry, 1)), tll)
	{
	  fprintf (f, "\tdef test_%s(self):\n",
		      TREE_VALUE (TREE_OPERAND (tll->entry, 0)));
	  DL_FOREACH (TREE_LIST (TREE_OPERAND (tll->entry, 1)), tlll)
	    {
	      fprintf (f, "\t\tself.assertEqual(self.lib.%s(",
			  TREE_VALUE (TREE_OPERAND (tll->entry, 0)));
	      codegen_atomic_value (f, tlll->entry);
	      fprintf (f, "), %s.%s(",
			  TREE_VALUE (TREE_OPERAND (tl->entry, 0)),
			  TREE_VALUE (TREE_OPERAND (tll->entry, 0)));
	      codegen_atomic_value (f, tlll->entry);	
	      fprintf (f, "))\n");
	    }
	}
    }

  fprintf (f, "if __name__ == '__main__':\n");
  DL_FOREACH (TREE_LIST (module_list), tl)
    fprintf (f, "\tsuite = unittest.TestLoader().loadTestsFromTestCase"
		"(Test_%s)\n" 
		"\tunittest.TextTestRunner(verbosity=2).run(suite)\n",
		TREE_VALUE (TREE_OPERAND (tl->entry, 0)));
  
  printf ("note: finished generating python code  [ok].\n");
  return function_error;
}
