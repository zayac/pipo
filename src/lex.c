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
#include <ctype.h>
#include <err.h>

#include "pipo.h"

#define TOKEN_KIND(a, b) b,
#define KEYWORD(a, b) b,
const char *token_kind_name[] = {
#include "token_kind.def"
#include "keywords.def"
};

#undef TOKEN_KIND
#undef KEYWORD

#define TOKEN_CLASS(a, b) b,
const char *token_class_name[] = {
#include "token_class.def"
};

#undef TOKEN_CLASS

/* This is a pointer to the first token from keywords.def  */
const char **keywords = &token_kind_name[(int) tv_function];
size_t keywords_length = tok_kind_length - tv_function;

static bool
lexer_init_file (struct lexer * lex, FILE * f, const char *fname);

/* Binary search function to search string in a char** table.  */
static inline size_t
kw_bsearch (const char *key, const char *table[], size_t len)
{
  size_t l = 0, r = len;

  while (l < r)
    {
      size_t hit = (l + r) / 2;
      int i = strcmp (key, table[hit]);
      /* printf ("%s ? %s, [%i, %i, %i]\n", key, table[hit], l, r, i); */

      if (i == 0)
	return hit;
      else if (i < 0)
	r = hit;
      else
	l = hit + 1;
    }
  return len;
}

/* Initialize lexer LEX with a file name FNAME and
   set initial parameters of the lexer.  */
bool
lexer_init (struct lexer * lex, const char *fname)
{
  FILE *f = fopen (fname, "r");

  if (!f)
    {
      warn ("error opening file `%s'", fname);
      return false;
    }

  return lexer_init_file (lex,  f, fname);
}

/* Initialize lexer LEX with a file FILE, which is open
   by external program and the name FNAME that matches
   FILE.  Set initial parameters of the lexer.  */
static bool
lexer_init_file (struct lexer * lex, FILE * f, const char *fname)
{
  assert (fname != NULL, "lexer initialized with empty filename");
  assert (lex != NULL, "lexer memory is not allocated");
  assert (f != NULL, "invalid file passed to lexer");

  lex->is_eof = false;
  lex->loc = (struct location){1, 0};
  lex->fname = fname;
  lex->file = f;
  lex->error_notifications = false;
  if (!lex->file)
    {
      warn ("error opening file `%s'", fname);
      return false;
    }

  return true;
}

/* Actions before deallocating lexer.  */
bool
lexer_finalize (struct lexer * lex)
{
  fclose (lex->file);
  return true;
}


/* Gets one character from the file, is end of file is
   reached, it will return EOF in all the consequent calls.  */
static inline char
lexer_getch (struct lexer *lex)
{
  int ch;

  if (lex->is_eof)
    return EOF;

  ch = fgetc (lex->file);
  if (ch == EOF)
    {
      lex->is_eof = true;
      return EOF;
    }

  if (ch == '\n')
    {
      lex->loc.line++;
      lex->loc.col = 0;
    }
  else
    lex->loc.col++;
  return (char) ch;
}

/* Put character back on the stream of the lexer.
   Consequent lexer_getch should return exactly this character.  */
static inline void
lexer_ungetch (struct lexer *lex, char ch)
{
  if (ch == '\n')
    lex->loc.line--;
  /* FIXME position should show the last symbol
     of previous line, not -1.  */
  lex->loc.col--;
  ungetc (ch, lex->file);
}

/* Adds the character C to the string *BUFFER that has length *SIZE
   at the position *INDEX. *INDEX is a pointer in the *BUFFER.
   If the *BUFFER is NULL then it is being allocated, if the *INDEX
   points at the end of the *BUFFER the *BUFFER will be reallocated. */
static inline void
buffer_add_char (char **buffer, char **index, size_t * size, char c)
{
  const size_t initial_size = 16;

  if (*buffer == NULL)
    {
      *buffer = (char *) malloc (initial_size * sizeof (char));
      memset(*buffer, 0, initial_size * sizeof (char));
      *index = *buffer;
      *(*index)++ = c;
      *size = initial_size;
      return;
    }

  assert (*index <= *buffer + *size,
	  "index is greater than allocated buffer");

  if (*index == *buffer + *size)
    {
      *buffer = (char *) realloc (*buffer, *size * 2 * sizeof (char));
      *index = *buffer + *size;
      *size *= 2;
    }

  *(*index)++ = c;
}

/* Internal function to read until the end of comment.  */
static inline enum token_class
lexer_read_comments (struct lexer *lex, char **buf, size_t * size)
{
  char *index = *buf;

  while (true)
    {
      char c = lexer_getch (lex);

      if (c == EOF)
	break;

      if (c == '\n')
	break;

      buffer_add_char (buf, &index, size, c);
    }

  buffer_add_char (buf, &index, size, '\0');
  return tok_comments;
}

/* Internal function to read until the end of string/char ignoring
escape sequences. */
static inline enum token_class
lexer_read_string (struct lexer *lex, char **buf, size_t * size, char c)
{
  char *index = *buf;
  const char stop = c;

  assert (stop == '"', "inapproriate starting symbol for string or char");

  buffer_add_char (buf, &index, size, stop);

  while (true)
    {
      c = lexer_getch (lex);
      if (c == EOF)
	{
	  if (lex->error_notifications)
	    error_loc (lex->loc,
		       "unexpected end of file in the middle of string");
	  buffer_add_char (buf, &index, size, 0);
	  return tok_unknown;
	}

      buffer_add_char (buf, &index, size, c);
      if (c == '\\')
	{
	  char cc = lexer_getch (lex);
	  if (cc == EOF)
	    {
	      if (lex->error_notifications)
		error_loc (lex->loc,
			   "unexpected end of file in the middle of string");
	      buffer_add_char (buf, &index, size, 0);
	      return tok_unknown;
	    }
	  buffer_add_char (buf, &index, size, cc);
	}
      else if (c == stop)
	break;
    }

  buffer_add_char (buf, &index, size, 0);
  return tok_string;
}

/* Function to read a hex number */
static inline void
lexer_read_octal_number (struct lexer *lex, struct token *tok, char **buf,
		       size_t * size, char c)
{
  char *index = *buf;

  assert (c == '0', "a character must be '0'");
  buffer_add_char (buf, &index, size, c);
  do
    {
      buffer_add_char (buf, &index, size, c);
      c = lexer_getch (lex);
    }
  while (c >= '0' && c < '8');

  lexer_ungetch (lex, c);
  buffer_add_char (buf, &index, size, 0);
  tok->tok_class = tok_octnum;
  tok->uses_buf = true;
}

/* Function to read a hex number */
static inline void
lexer_read_hex_number (struct lexer *lex, struct token *tok, char **buf,
		       size_t * size, char c)
{
  char *index = *buf;

  assert (c == '0', "a character must be '0', '%c' found", c);
  buffer_add_char (buf, &index, size, c);
  c = lexer_getch (lex);
  assert (c == 'x' || c == 'X', "a character must be 'x' or 'X', '%c' found", c);
  do
    {
      buffer_add_char (buf, &index, size, c);
      c = lexer_getch (lex);
    }
  while (isxdigit (c));

  lexer_ungetch (lex, c);
  buffer_add_char (buf, &index, size, 0);
  tok->tok_class = tok_hexnum;
  tok->uses_buf = true;
}

/* Internal function to read until the end of identifier, checking
   if it is a keyword.  */
static inline void
lexer_read_id (struct lexer *lex, struct token *tok,
               char **buf, size_t *size, char c)
{
  char *index = *buf;
  size_t search;

  do 
    {
      buffer_add_char (buf, &index, size, c);
      c = lexer_getch (lex);
    }
  while (isalnum (c) || c == '_');
  lexer_ungetch (lex, c);
  buffer_add_char (buf, &index, size, 0);

  search = kw_bsearch (*buf, keywords, keywords_length);
  if (search != keywords_length)
    {
      if (*buf)
        free (*buf);
      *size = 0;
      *buf = NULL;
      tval_tok_init (tok, tok_keyword, (enum token_kind)(search + tv_function));
      return;
    }
  tok->tok_class = tok_id;
  return;
}

/* Internal function to read until the end of number.  */
static inline enum token_class
lexer_read_number (struct lexer *lex, char **buf, size_t * size, char c)
{
  bool isreal = false;
  bool saw_dot = false;
  bool saw_exp = false;
  char *index = *buf;

  buffer_add_char (buf, &index, size, c);

  if (c == '.')
    {
      isreal = true;
      saw_dot = true;

      c = lexer_getch (lex);

      if (!isdigit (c))
	{
	  if (lex->error_notifications)
	    error_loc (lex->loc, "digit expected, '%c' found instead", c);
	  lexer_ungetch (lex, c);
	  goto return_unknown;
	}
      else
	buffer_add_char (buf, &index, size, c);
    }

  while (true)
    {
      c = lexer_getch (lex);
      if (c == EOF)
	{
	  if (lex->error_notifications)
	    error_loc (lex->loc, "unexpected end of file");
	  goto return_unknown;
	}
      else if (c == 'e' || c == 'E')
	{
	  if (saw_exp)
	    {
	      if (lex->error_notifications)
		error_loc (lex->loc, "exponent is specified more than once");
	      goto return_unknown;
	    }
	  isreal = true;

	  buffer_add_char (buf, &index, size, c);
	  c = lexer_getch (lex);

	  if (c == '+' || c == '-')
	    {
	      buffer_add_char (buf, &index, size, c);
	      c = lexer_getch (lex);
	    }

	  if (!isdigit (c))
	    {
	      if (lex->error_notifications)
		error_loc (lex->loc, "digit expected after exponent sign");
	      goto return_unknown;
	    }
	  else
	    buffer_add_char (buf, &index, size, c);

	  while (isdigit (c = lexer_getch (lex)))
	    buffer_add_char (buf, &index, size, c);

	  break;
	}
      else if (c == '.')
	{
	  if (saw_dot)
	    {
	      if (lex->error_notifications)
		error_loc (lex->loc, "more than one dot in the number ");
	      goto return_unknown;
	    }
	  saw_dot = true;
	  isreal = true;
	}
      else if (!isdigit (c))
	break;

      buffer_add_char (buf, &index, size, c);
    }
  lexer_ungetch (lex, c);
  buffer_add_char (buf, &index, size, 0);

  if (isreal)
    return tok_realnum;
  else
    return tok_intnum;

return_unknown:
  buffer_add_char (buf, &index, size, 0);
  return tok_unknown;
}

/* Reads the stream from lexer and returns dynamically allocated token
   of the appropriate type.  */
struct token *
lexer_get_token (struct lexer *lex)
{
  char c;
  struct location loc;
  struct token *tok = (struct token *) malloc (sizeof (struct token));
  tok->uses_buf = false;
  size_t buf_size = 16;
  char *buf = NULL;

  c = lexer_getch (lex);
  loc = lex->loc;
  if (isspace (c))
    {
      while (EOF != (c = lexer_getch (lex)) && isspace (c));
      loc = lex->loc;
    }

  if (c == EOF)
    {
      tval_tok_init (tok, tok_eof, tv_eof);
      goto return_token;
    }

  if (c == '#')
    {
      tok->tok_class = lexer_read_comments (lex, &buf, &buf_size);
      goto return_token;
    }

  if (c == '"')
    {
      tok->tok_class = lexer_read_string (lex, &buf, &buf_size, c);
      goto return_token;
    }

  if (isalpha (c))
    {
      lexer_read_id (lex, tok, &buf, &buf_size, c);
      goto return_token;
    }

  if (c == '.')
    {
      tok->tok_class = lexer_read_number (lex, &buf, &buf_size, c);
      goto return_token;
    }

  if (isdigit (c))
    {
      if (c == '0')
	{
	  int c1 = c;
	  c = lexer_getch (lex);
	  lexer_ungetch (lex, c);
	  if  (c == 'x' || c == 'X')
	    lexer_read_hex_number (lex, tok, &buf, &buf_size, c1);
	  else if (isdigit (c))
	    {
	      if (!(c >= '0' && c <= '7'))
		error_loc (lex->loc, "%c found in the octal number", c);
	      else
		lexer_read_octal_number (lex, tok, &buf, &buf_size, c1);
	    }
	  else
	    tok->tok_class = lexer_read_number (lex, &buf, &buf_size, c1);
	}
      else
	tok->tok_class = lexer_read_number (lex, &buf, &buf_size, c);
      goto return_token;
    }

  switch (c)
    {
    case ',':
      tval_tok_init (tok, tok_operator, tv_comma);
      goto return_token;
    case '(':
      tval_tok_init (tok, tok_operator, tv_lparen);
      goto return_token;
    case ')':
      tval_tok_init (tok, tok_operator, tv_rparen);
      goto return_token;
    case '{':
      tval_tok_init (tok, tok_operator, tv_lbrace);
      goto return_token;
    case '}':
      tval_tok_init (tok, tok_operator, tv_rbrace);
      goto return_token;
    default:
      ;
    }

  /* if nothing was found, we construct an unknown token.  */
  assert (buf == NULL, "buf was used, but token_class is missing");
  buf = (char *) malloc (2 * sizeof (char));
  buf[0] = c;
  buf[1] = 0;
  tok->tok_class = tok_unknown;

return_token:
  /* All tokens are valid except `tok_class_length'.  */
  assert (tok->tok_class <= tok_unknown, "token type was not provided");

  if (buf != NULL)
    {
      tok->uses_buf = true;
      tok->value.cval = buf;
    }

  tok->loc = loc;
  return tok;
}


/* If the value of the token needs a character buffer or it is
   stored as an enum token_kind variable.  */
inline bool
token_uses_buf (struct token * tok)
{
  return tok->uses_buf;
}

/* String representation of the token TOK.  */
const char *
token_as_string (struct token *tok)
{

  if (token_uses_buf (tok))
    return tok->value.cval;
  else
    return token_kind_name[(int) tok->value.tval];
}


/* Prints the token.  */
void
token_print (struct token *tok)
{
  const char *tokval = token_as_string (tok);

  (void) fprintf (stdout, "%d:%d %s ", (int) tok->loc.line,
		  (int) tok->loc.col, token_class_name[(int) tok->tok_class]);

  if (tok->tok_class != tok_unknown)
    (void) fprintf (stdout, "['%s']\n", tokval);
  else
    (void) fprintf (stdout, "['%s'] !unknown\n", tokval);

  fflush (stdout);
}

/* Copy token. Also copies string if necessary.
   Memory allocation is done too.
 */
struct token *
token_copy (struct token *tok)
{
  struct token *ret;
  if (tok == NULL)
    return NULL;

  ret = (struct token *) malloc (sizeof (struct token));
  ret->loc = tok->loc;
  ret->tok_class = tok->tok_class;
  ret->uses_buf = token_uses_buf (tok);
  if (token_uses_buf (tok))
    ret->value.cval = strdup (tok->value.cval);
  else
    ret->value.tval = tok->value.tval;
  return ret;
}

/* Compare two tokens
   It doesn't take into consideration token locations
 */
int
token_compare (struct token *first, struct token *second)
{
  if (first == second)
    return 0;

  /* Compare token classes  */
  if (first->tok_class < second->tok_class)
    return -1;
  else if (first->tok_class > second->tok_class)
    return 1;

  /* Compare by buffer usage  */
  if (token_uses_buf (first) != token_uses_buf (second))
    {
      if (!token_uses_buf (first))
	return -1;
      else
	return 1;
    }

  if (token_uses_buf (first))
    return strcmp (first->value.cval, second->value.cval);
  else
    {
      if (first->value.tval < second->value.tval)
	return -1;
      else if (first->value.tval > second->value.tval)
	return 1;
      else
	return 0;
    }
  return 0;
}

/* Deallocates the memory that token occupies.  */
void
token_free (struct token *tok)
{
  assert (tok, "attempt to free NULL token");

  if (token_uses_buf (tok) && tok->value.cval)
    free (tok->value.cval);
  free (tok);
  tok = NULL;
}


/* Main function if you want to test lexer part only.  */
#ifdef LEXER_BINARY
int error_count = 0;
int warning_count = 0;

int
main (int argc, char *argv[])
{
  struct lexer *lex = (struct lexer *) malloc (sizeof (struct lexer));
  struct token *tok = NULL;

  if (argc <= 1)
    {
      fprintf (stderr, "No input file\n");
      goto cleanup;
    }

  if (!lexer_init (lex, argv[1]))
    goto cleanup;

  while ((tok = lexer_get_token (lex))->tok_class != tok_eof)
    {
      token_print (tok);
      token_free (tok);
    }

  token_free (tok);
  lexer_finalize (lex);

cleanup:
  if (lex)
    free (lex);

  return 0;
}
#endif
