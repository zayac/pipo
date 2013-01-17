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

#ifndef __PARSER_H__
#define __PARSER_H__

#include "pipo.h"

struct parser
{
  struct lexer *lex;

  /* Buffer and lengths associated with buffer.
     Buffer holds up-to BUF_SIZE tokens, which means
     that it is possible to look BUF_SIZE tokens
     forward.  */
  struct token **token_buffer;
  size_t buf_size;
  size_t buf_start, buf_end, unget_idx;
  bool buf_empty;

  /* Count of opened parens, square brackets and
     figure brackets. Used when we skip the tokens
     skip is finished when all the three counters
     are zeroes.  */
  int paren_count;
  int square_count;
  int brace_count;
};


__BEGIN_DECLS
#define TOKEN_CLASS(a, b) \
static inline bool \
token_is_ ## a (struct token *  tok, enum token_kind tkind) \
{ \
  return token_class (tok) == tok_ ## a && token_value (tok) == tkind; \
}
#include "token_class.def"
#undef TOKEN_CLASS
static inline bool
token_is_number (struct token *tok)
{
  return (token_class (tok) == tok_realnum
	  || token_class (tok) == tok_intnum
	  || token_class (tok) == tok_octnum
	  || token_class (tok) == tok_hexnum);
}

int parse (struct parser *);
bool parser_init (struct parser *, struct lexer *);
bool parser_finalize (struct parser *);

#define PARSER_MATCH_EXPR_ALLOWED(parser) ((parser)->match_expr_allowed)
__END_DECLS
#endif /* __PARSER_H__  */
