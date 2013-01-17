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

#ifndef __GLOBAL_H__
#define __GLOBAL_H__

#include <stdarg.h>
#include "tree.h"

/* Structure to store a list of user-defined types
   FIXME it should be a hash-table!  */
extern tree module_list;

/* Trees we are to remove in the end.  */
extern tree delete_list;

extern int error_count;
extern int warning_count;
extern tree global_tree[];

void init_global (void);
void finalize_global (void);
void init_global_tree (void);
void finalize_global_tree (void);

int compare_ints (const void *, const void *);
tree module_exists (tree, const char *);

#endif /* __GLOBAL_H__ */

