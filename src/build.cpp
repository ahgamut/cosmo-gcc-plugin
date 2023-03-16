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
#include "ifswitch.h"

/* construct AST by function calls LOL */
#define BUILD_STRING_AS_TREE(str)     build_string_literal(strlen((str)) + 1, (str))

tree build_modded_switch_stmt(tree swexpr, subu_list *list) {
  int case_count = 0, glitch_count = 0, break_count = 0;
  tree stmt = SWITCH_STMT_BODY(swexpr);
  tree t = NULL;
  subu_node *use = NULL;
  char dest[64] = {0};
  for (auto i = tsi_start(stmt); !tsi_end_p(i); tsi_next(&i)) {
    t = tsi_stmt(i);
    if (TREE_CODE(tsi_stmt(i)) == CASE_LABEL_EXPR) {
      case_count += 1;
      // DEBUGF("location of this case is %u\n", EXPR_LOCATION(tmp));

      if (get_subu_elem(list, EXPR_LOCATION(t),
                        &use)          /* on a line we substituted */
          && CASE_LOW(t) != NULL_TREE  /* not a x..y range */
          && CASE_HIGH(t) == NULL_TREE /* not a default */
          && tree_to_shwi(CASE_LOW(t)) == get_value_of_const(use->name)
          /* the case is the one we substituted */) {
        glitch_count += 1;
        remove_subu_elem(list, use);
      }
    } else if (TREE_CODE(tsi_stmt(i)) == BREAK_STMT) {
      break_count += 1;
    }
  }
  snprintf(dest, sizeof(dest),
           "above switch had %d cases, %d cases replaced and %d breaks\n",
           case_count, glitch_count, break_count);
  return build_call_expr(VAR_NAME_AS_TREE("printf"), 1, BUILD_STRING_AS_TREE(dest));
}
