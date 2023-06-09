/*- mode:c++;indent-tabs-mode:nil;c-basic-offset:2;tab-width:8;coding:utf-8 -*-│
│vi: set net ft=c++ ts=2 sts=2 sw=2 fenc=utf-8                              :vi│
╞══════════════════════════════════════════════════════════════════════════════╡
│ Copyright © 2022, Gautham Venkatasubramanian                                 │
│                                                                              │
│ Permission to use, copy, modify, and/or distribute this software for         │
│ any purpose with or without fee is hereby granted, provided that the         │
│ above copyright notice and this permission notice appear in all copies.      │
│                                                                              │
│ THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL                │
│ WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED                │
│ WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE             │
│ AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL         │
│ DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR        │
│ PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER               │
│ TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR             │
│ PERFORMANCE OF THIS SOFTWARE.                                                │
╚─────────────────────────────────────────────────────────────────────────────*/
#include <utils.h>

const char *get_tree_code_str(tree expr) {
#define END_OF_BASE_TREE_CODES
#define DEFTREECODE(a, b, c, d) \
  case a:                       \
    return b;
  switch (TREE_CODE(expr)) {
#include "all-tree.def"
    default:
      return "<unknown>";
  }
#undef DEFTREECODE
#undef END_OF_BASE_TREE_CODES
}

tree get_ifsw_identifier(char *s) {
  char *result = (char *)xmalloc(strlen("__tmpcosmo_") + strlen(s) + 1);
  strcpy(result, "__tmpcosmo_");
  strcat(result, s);
  tree t = lookup_name(get_identifier(result));
  free(result);
  return t;
}

int get_value_of_const(char *name) {
  tree vx = get_ifsw_identifier(name);
  int z = tree_to_shwi(DECL_INITIAL(vx));
  return z;
}

int check_magic_equal(tree value, char *varname) {
  tree vx = get_ifsw_identifier(varname);
  return tree_int_cst_equal(value, DECL_INITIAL(vx));
}
