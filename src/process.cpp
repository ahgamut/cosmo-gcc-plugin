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

#define FUNC_CALL_AS_TREE(fname) lookup_name(get_identifier((fname)))
#define STRING_AS_TREE(str)      build_string_literal(strlen((str)) + 1, (str))

tree insert_case_count(tree swexpr) {
  int case_count = 0, break_count = 0;
  tree stmt = SWITCH_STMT_BODY(swexpr);
  char dest[64] = {0};
  for (auto i = tsi_start(stmt); !tsi_end_p(i); tsi_next(&i)) {
    if (TREE_CODE(tsi_stmt(i)) == CASE_LABEL_EXPR)
      case_count += 1;
    else if (TREE_CODE(tsi_stmt(i)) == BREAK_STMT)
      break_count += 1;
  }
  snprintf(dest, sizeof(dest), "above switch had %d cases and %d breaks\n",
           case_count, break_count);
  return build_call_expr(FUNC_CALL_AS_TREE("printf"), 1, STRING_AS_TREE(dest));
}

void process_switch(tree *swptr) {
  tree swexpr = *swptr;
  /* swexpr is basically a switch statement as a GCC GENERIC AST */
  printf("we are in the switch statement\n");
  printf("type is\n");
  debug_tree(SWITCH_STMT_TYPE(swexpr));
  printf("condition is\n");
  debug_tree(SWITCH_STMT_COND(swexpr));
  printf("body is\n");
  process_stmt(&SWITCH_STMT_BODY(swexpr));
  printf("scope is\n");
  debug_tree(SWITCH_STMT_SCOPE(swexpr));
  *swptr = insert_case_count(*swptr);
}

void process_stmt(tree *sptr) {
  tree stmt = *sptr;
  tree temp;
  switch (TREE_CODE(stmt)) {
    case BIND_EXPR:
      temp = BIND_EXPR_BODY(stmt);
      process_stmt(&temp);
      break;
    case STATEMENT_LIST:
      for (auto i = tsi_start(stmt); !tsi_end_p(i); tsi_next(&i)) {
        process_stmt(tsi_stmt_ptr(i));
        if (TREE_CODE(tsi_stmt(i)) == SWITCH_STMT) {
          tsi_delink(&i);
        }
      }
      break;
    case SWITCH_STMT:
      process_switch(sptr);
      break;
    default:
      printf("did not process %s\n", get_tree_code_str(stmt));
      break;
  }
}
