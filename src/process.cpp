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

tree get_magic_identifier(char *s) {
  char *result = (char *)xmalloc(strlen("__tmp_ifs_") + strlen(s) + 1);
  strcpy(result, "__tmp_ifs_");
  strcat(result, s);
  tree t = lookup_name(get_identifier(result));
  free(result);
  return t;
}

tree insert_case_count(tree swexpr, subu_list *list) {
  int case_count = 0, break_count = 0;
  tree stmt = SWITCH_STMT_BODY(swexpr);
  tree tmp = NULL;
  char dest[64] = {0};
  subu_node *use = NULL;
  for (auto i = tsi_start(stmt); !tsi_end_p(i); tsi_next(&i)) {
    tmp = tsi_stmt(i);
    if (TREE_CODE(tsi_stmt(i)) == CASE_LABEL_EXPR) {
      case_count += 1;
      // DEBUGF("location of this case is %u\n", EXPR_LOCATION(tmp));
      if (CASE_LOW(tmp) != NULL_TREE && CASE_HIGH(tmp) == NULL_TREE) {
        /* this is a case label */
        if (get_subu_elem(list, EXPR_LOCATION(tmp), &use) &&
            use->in_switch == 1) {
          tree vx = get_magic_identifier(use->name);
          int z = tree_to_shwi(DECL_INITIAL(vx));
          DEBUGF("hello pls %d %x\n", z, z);
          debug_tree(DECL_INITIAL(vx));
          remove_subu_elem(list, use);
          use = NULL;
        }
        DEBUGF("location of the label is %u %u\n", EXPR_LOC_LINE(tmp),
               EXPR_LOC_COL(tmp));
        auto sr = EXPR_LOCATION(tmp);
        debug_tree(CASE_LOW(tmp));
        debug_tree(CASE_LABEL(tmp));
        debug_tree(CASE_CHAIN(tmp));
      }
    } else if (TREE_CODE(tsi_stmt(i)) == BREAK_STMT) {
      break_count += 1;
    } else {
      DEBUGF("%s: inside switch body %u\n", get_tree_code_str(tmp),
             EXPR_LOC_LINE(tmp));
      if (get_subu_elem(list, EXPR_LOCATION(tmp), &use)) {
        tree vx = get_magic_identifier(use->name);
        tree raw = DECL_INITIAL(vx);
        tree actual = lookup_name(get_identifier(use->name));
        int z = tree_to_shwi(DECL_INITIAL(vx));
        /* This BREAKS EVERYTHING,
         * but we get what we want :P */
        /* (*raw) = *actual; */
        auto tmp2 = substitute_in_expr(tmp, DECL_INITIAL(vx), actual);
        debug_tree(actual);
        DEBUGF("\nREPLACE hello pls %d\n", EXPR_LOCATION(tmp), use->loc, z);
        debug_tree(tmp2);
        remove_subu_elem(list, use);
        use = NULL;
      }
    }
  }
  snprintf(dest, sizeof(dest), "above switch had %d cases and %d breaks\n",
           case_count, break_count);
  return build_call_expr(FUNC_CALL_AS_TREE("printf"), 1, STRING_AS_TREE(dest));
}

void process_switch(tree *swptr, subu_list *list) {
  tree swexpr = *swptr;
  /* swexpr is basically a switch statement as a GCC GENERIC AST */
  printf("we are in the switch statement\n");
  printf("type is\n");
  debug_tree(SWITCH_STMT_TYPE(swexpr));
  printf("condition is\n");
  debug_tree(SWITCH_STMT_COND(swexpr));
  printf("body is\n");
  process_stmt(&SWITCH_STMT_BODY(swexpr), list);
  printf("scope is\n");
  debug_tree(SWITCH_STMT_SCOPE(swexpr));
  *swptr = insert_case_count(*swptr, list);
}

void process_stmt(tree *sptr, subu_list *list) {
  tree stmt = *sptr;
  source_range rng = EXPR_LOCATION_RANGE(stmt);
  tree temp;
  if (list->count == 0) {
    DEBUGF("no substitutions made in %s\n", IDENTIFIER_NAME(stmt));
    return;
  }
  switch (TREE_CODE(stmt)) {
    case BIND_EXPR:
      temp = BIND_EXPR_BODY(stmt);
      process_stmt(&temp, list);
      break;
    case STATEMENT_LIST:
      for (auto i = tsi_start(stmt); !tsi_end_p(i); tsi_next(&i)) {
        process_stmt(tsi_stmt_ptr(i), list);
        if (TREE_CODE(tsi_stmt(i)) == SWITCH_STMT) {
          tsi_delink(&i);
        }
      }
      break;
    case SWITCH_STMT:
      process_switch(sptr, list);
      break;
    default:
      printf("did not process %s\n", get_tree_code_str(stmt));
      break;
  }
}
