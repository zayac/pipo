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

#include "pipo.h"
#include "config.h"
#include "global.h"
#include "parser.h"
#include "codegen.h"

#include <stdlib.h>
#include <getopt.h>

static char *progname;

#ifndef LEXER_BINARY
int
main (int argc, char *argv[])
{
  int ret = 0;
  char *src_name = NULL;

  struct lexer *lex = (struct lexer *) malloc (sizeof (struct lexer));
  struct parser *parser = (struct parser *) malloc (sizeof (struct parser));

  init_global ();
  init_global_tree ();

  progname = strrchr (argv[0], '/');
  if (NULL == progname)
    progname = argv[0];
  else
    progname++;

  argv += 1;
  /* FIXME: What if we have multiple files?  */
  if (NULL == *argv)
    {
      fprintf (stderr, "%s:error: filename argument required\n", progname);
      ret = -1;
      goto cleanup;
    }

  /* Initialize the lexer.  */
  if (!lexer_init (lex, *argv))
    {
      fprintf (stderr, "%s cannot create a lexer for file `%s'\n", progname,
	       *argv);
      ret = -2;
      goto cleanup;
    }
  else
    {
      /* Discard extension from file to compile.  */
      char* start = strrchr (*argv, '/');
      char* ext = strrchr (*argv, '.');
      int size = 0;
      
      if (start == NULL)
	start = *argv;
      else
	start += 1;
      if (ext == NULL)
	size = strlen(start);
      else
	size = ext - start;
      src_name = strndup (start, size);
    }


  /* Initialize the parser.  */
  parser_init (parser, lex);

  if ((ret += parse (parser)) == 0)
    ret += codegen (src_name);

  printf ("note: finished compiling.\n");

  free (src_name);
cleanup:
  parser_finalize (parser);
  finalize_global_tree ();
  finalize_global ();

  /* That should be called at the very end.  */
  free_atomic_trees ();

  if (parser)
    free (parser);
  if (lex)
    free (lex);

  return ret == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
#endif
