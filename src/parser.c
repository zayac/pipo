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
#include <stdlib.h>
#include <err.h>

#include "tree.h"
#include "global.h"
#include "parser.h"

static struct token *parser_get_token (struct parser *);
static void parser_unget (struct parser *);

static struct token *parser_get_until_tval (struct parser *, enum token_kind);

/* Safely increment or decrement index of token buffer. Make
   sure that negative index equals to size - idx.  */
static inline size_t
buf_idx_inc (const size_t idx, const ssize_t inc, const size_t size)
{
  ssize_t diff = ((ssize_t) idx + inc) % size;
  return diff < 0 ? (size_t)(size - diff) : (size_t)diff;
}

/* Get one token from the lexer or from the token buffer.
   Token is taken from the buffer if parser_unget was
   called earlier. */
static struct token *
parser_get_lexer_token (struct parser *parser)
{
  struct token *tok;

  if (parser->unget_idx == 0)
    {
      /* Skip comments for the time being. We do not skip
	 the comments at the level of lexer, because we
	 can put them in the output program.  */
      while (true)
	{
	  tok = lexer_get_token (parser->lex);
	  if (token_class (tok) != tok_comments
	      && token_class (tok) != tok_whitespace)
	    break;
	  else
	    token_free (tok);
	}

      /* Keep track of brackets.  */
      if (token_class (tok) == tok_operator)
	switch (token_value (tok))
	  {
	  case tv_lparen:
	    parser->paren_count++;
	    break;
	  case tv_rparen:
	    parser->paren_count--;
	    break;
	  case tv_lbrace:
	    parser->brace_count++;
	    break;
	  case tv_rbrace:
	    parser->brace_count--;
	    break;
	  default:
	    ;
	  }

      /* If TOKEN_BUFFER is full, we free the token pointed by BUF_START
	 and put the new token on its place, changing BUF_START and
	 BUF_END accordingly.  */
      if ((parser->buf_end + 1) % parser->buf_size == parser->buf_start)
	{
	  token_free (parser->token_buffer[parser->buf_start]);
	  parser->buf_start = (parser->buf_start + 1) % parser->buf_size;
	  parser->token_buffer[parser->buf_end] = tok;
	  parser->buf_end = (parser->buf_end + 1) % parser->buf_size;
	}
      else
	{
	  parser->token_buffer[parser->buf_end] = tok;
	  parser->buf_end = (parser->buf_end + 1) % parser->buf_size;
	}
    }
  else
    {
      ssize_t s;

      /* Return a token from the buffer.  */
      assert (parser->unget_idx < parser->buf_size,
	      "parser buffer holds only up to %i values.", parser->buf_size);

      s = parser->buf_end - parser->unget_idx;
      s = s < 0 ? parser->buf_size + s : (unsigned) s;
      parser->unget_idx--;

      tok = parser->token_buffer[s];
    }

  return tok;
}

/* Move the parser one token back. It means that the consequent
   call of parser_get_token would return the token from buffer,
   not from lexer.  */
static void
parser_unget (struct parser *parser)
{
  parser->unget_idx++;
  assert (parser->unget_idx < parser->buf_size,
	  "parser buffer holds only up to %i values.", parser->buf_size);
}

/* Skip tokens until token with value TKIND would be found.  */
static struct token *
parser_get_until_tval (struct parser *parser, enum token_kind tkind)
{
  struct token *tok;

  do
    {
      tok = parser_get_token (parser);
      if (!token_uses_buf (tok)
	  /* FIXME the following condition makes it impossible
	     to skip until some symbol if you are inside the
	     block or brackets. */
	  /* && parser_parens_zero (parser) */
	  && token_value (tok) == tkind)
	return tok;
    }
  while (token_class (tok) != tok_eof);

  return tok;
}

/* XXX For the time being make it macro in order to see
   the __LINE__ expansions of error_loc.  */
#define parser_forward_tval(parser, tkind)  __extension__	  \
({								  \
  struct token * tok = parser_get_token (parser);		  \
  if (token_uses_buf  (tok)  || token_value (tok) != tkind)	  \
    {								  \
      error_loc (token_location (tok), "unexpected token `%s' ",  \
		 token_as_string (tok));			  \
      tok = NULL;						  \
    }								  \
  tok;								  \
})

/* Get token from lexer. In addition to this,
   it does some hex number processing.  */
static struct token *
parser_get_token (struct parser *parser)
{
  struct token *tok = parser_get_lexer_token (parser);

  /* Check and concatenate \left or \right with delimiters, if necessary  */
  if (token_uses_buf (tok)
      && !(strcmp (token_as_string (tok), "\\left")
	   && strcmp (token_as_string (tok), "\\right")))
    {
      struct token *del = parser_get_token (parser);
      char *conc = NULL;
      size_t s = parser->buf_size, e = parser->buf_end;

      if (-1 == asprintf (&conc, "%s%s",
			  token_as_string (tok),
			  token_as_string (del)))
	err (EXIT_FAILURE, "asprintf failed");

      /* Leave one token instead of two  */
      free (tok->value.cval);
      tok->value.cval = conc;
      token_free (parser->token_buffer[buf_idx_inc (e, -1, s)]);
      parser->buf_end = buf_idx_inc (e, -1, s);
    }
  return tok;
}

/* Initialize the parser, allocate memory for token_buffer.  */
bool
parser_init (struct parser * parser, struct lexer * lex)
{
  parser->lex = lex;
  parser->buf_size = 16;
  parser->buf_start = 0;
  parser->buf_end = 0;
  parser->buf_empty = true;
  parser->token_buffer =
    (struct token * *) malloc (parser->buf_size * sizeof (struct token *));
  parser->unget_idx = 0;
  return true;
}

/* Clear the memory allocated for internal structure.
   NOTE: PARSER is not freed.  */
bool
parser_finalize (struct parser * parser)
{
  assert (parser, "attempt to free empty parser");

  if (parser->buf_size != 0)
    {
      while (parser->buf_start % parser->buf_size !=
	     parser->buf_end % parser->buf_size)
	{
	  token_free (parser->token_buffer[parser->buf_start]);
	  parser->buf_start = (parser->buf_start + 1) % parser->buf_size;
	}

      if (parser->token_buffer)
	{
	  free (parser->token_buffer);
	}

      lexer_finalize (parser->lex);
    }
  return true;
}

static tree
handle_list (struct parser * parser, tree (*handler) (struct parser *),
	     enum token_kind delim)
{
  tree ret;
  tree t;
  t = handler (parser);
  if (t == error_mark_node)
    return t;

  if (!token_is_operator (parser_get_token (parser), delim))
    {
      ret = make_tree_list ();
      tree_list_append (ret, t);
    }
  else
    {
      tree list = make_tree_list ();
      tree_list_append (list, t);
      while (true)
	{
	  t = handler (parser);

	  if (t != NULL && t != error_mark_node)
	    tree_list_append (list, t);
	  if (!token_is_operator (parser_get_token (parser), delim))
	    {
	      parser_unget (parser);
	      return list;
	    }
	}
      ret = list;
    }
  parser_unget (parser);

  return ret;
}

tree
handle_value (struct parser *parser)
{
  struct token *tok;
  tok = parser_get_token (parser);
  return make_value_tok (tok);
}

tree
handle_args (struct parser *parser)
{
  tree t;

  if (!parser_forward_tval (parser, tv_lparen))
    goto error;

  t = handle_list (parser, handle_value, tv_comma);

  if (!parser_forward_tval (parser, tv_rparen))
    return error_mark_node;

  return t;
error:
  parser_get_until_tval (parser, tv_rparen);
  return error_mark_node;

}

tree
handle_cases (struct parser *parser)
{
  struct token *tok;
  tree function, t;

  if (!parser_forward_tval (parser, tv_function))
    goto error;
  tok = parser_get_token (parser);
  function = make_tree (FUNCTION);
  TREE_OPERAND_SET (function, 0, make_value_tok (tok));

  if (!parser_forward_tval (parser, tv_lbrace))
    goto error;

  t = handle_list (parser, handle_args, tv_comma);

  TREE_OPERAND_SET (function, 1, t);

  if (!parser_forward_tval (parser, tv_rbrace))
    return error_mark_node;

  return function;
error:
  parser_get_until_tval (parser, tv_rbrace);
  return error_mark_node;
}

tree
handle_module (struct parser *parser)
{
  struct token *tok;
  tree module, t;

  if (!parser_forward_tval (parser, tv_validate))
    goto error;
  tok = parser_get_token (parser);
  module = make_tree (MODULE);
  TREE_OPERAND_SET (module, 0, make_value_tok (tok));

  if (!parser_forward_tval (parser, tv_lbrace))
    goto error;

  TREE_OPERAND_SET (module, 1, make_tree_list ());
  while (token_is_keyword (tok = parser_get_token (parser), tv_function))
    {
      parser_unget (parser);
      t = handle_cases (parser);
      tree_list_append (TREE_OPERAND (module, 1), t);
    }
  parser_unget (parser);

  if (!parser_forward_tval (parser, tv_rbrace))
    return error_mark_node;

  return module;
error:
  parser_get_until_tval (parser, tv_rbrace);
  return error_mark_node;
}

/* Top level function to parse the file.  */
int
parse (struct parser *parser)
{
  struct token *tok;
  error_count = warning_count = 0;
  while (token_class (tok = parser_get_token (parser)) != tok_eof)
    {
      parser_unget (parser);

      /* Enable lexer error handling inside modules.  */
      parser->lex->error_notifications = true;
      tree t = handle_module (parser);
      if (t != NULL && t != error_mark_node)
	{
	  if (!module_exists (module_list,
	    TREE_VALUE (TREE_OPERAND (t, 0))))
	    tree_list_append (module_list, t);
	  else
	    {
	      error_loc (TREE_LOCATION (t),
		    "function `%s' is defined already",
		    TREE_VALUE (TREE_OPERAND (t, 0)));
	    }
	}
      parser->lex->error_notifications = false;
    }

  printf ("note: finished parsing.\n");
  if (error_count != 0)
    {
      printf ("note: %i errors found.\n", error_count);
      return -3;
    }

  return 0;
}
