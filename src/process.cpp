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

struct context {
  subu_list *list;
  subu_list *special;
  tree cur;
  tree prev;
};

tree check_usage(tree *tp, int *check_subtree, void *data) {
  context *ctx = (context *)(data);
  tree t = *tp;
  tree z;
  subu_node *use = NULL;
  location_t loc = EXPR_LOCATION(t);

  if (ctx->list->count == 0) {
    DEBUGF("substitutions complete\n");
    *check_subtree = 0;
    return NULL_TREE;
  }

  if (check_loc_in_bound(ctx->list, loc)) {
    if (get_subu_elem(ctx->list, loc, &use)) {
      /* we know for sure one of our macro substitutions
       * has been executed at the location loc */
      DEBUGF("found mark at %u,%u in a %s\n", LOCATION_LINE(loc),
             LOCATION_COLUMN(loc), get_tree_code_str(t));
      if (TREE_CODE(t) == CASE_LABEL_EXPR /* this is a case */
          && CASE_LOW(t) != NULL_TREE     /* not a default */
          && CASE_HIGH(t) == NULL_TREE    /* not a x..y range */
          && tree_to_shwi(CASE_LOW(t)) == get_value_of_const(use->name)
          /* the case is the one we substituted */) {
        DEBUGF("this IS a case label we substituted\n");
        debug_tree(CASE_LOW(t));
        add_subu_elem(ctx->special, build_subu(use->loc, use->name,
                                               strlen(use->name), SW_CASE));
        remove_subu_elem(ctx->list, use);
      } else if (TREE_CODE(t) == CALL_EXPR) {
      check_call_args:
        call_expr_arg_iterator it;
        tree arg = NULL_TREE;
        int i = 0;
        int rep = -1;
        FOR_EACH_CALL_EXPR_ARG(arg, it, t) {
          DEBUGF("arg %d is %s\n", i, get_tree_code_str(arg));
          if (TREE_CODE(arg) == INTEGER_CST &&
              tree_to_shwi(arg) == get_value_of_const(use->name)) {
            DEBUGF("yup this is the constant we want\n");
            rep = i;
            break;
          }
          i += 1;
        }
        if (rep != -1) {
          DEBUGF("replace here pls\n");
          CALL_EXPR_ARG(t, rep) =
              build1(NOP_EXPR, integer_type_node, VAR_NAME_AS_TREE(use->name));
          // debug_tree(t);
          remove_subu_elem(ctx->list, use);
          use = NULL;
          if (get_subu_elem(ctx->list, loc, &use)) {
            /* there is another argument of this call
             * that also had one of our macro expansions */
            goto check_call_args;
          }
        }
      } else {
      check_expr_args:
        DEBUGF("this contains a substitution and it is... a %s?\n",
               get_tree_code_str(t));
        DEBUGF("how many operands do you have? %d\n", TREE_OPERAND_LENGTH(t));
        tree arg = NULL_TREE;
        int i = 0;
        int n = TREE_OPERAND_LENGTH(t);
        int rep = -1;
        for (i = 0; i < n; ++i) {
          arg = TREE_OPERAND(t, i);
          DEBUGF("arg %d is %s\n", i, get_tree_code_str(arg));
          if (TREE_CODE(arg) == INTEGER_CST &&
              tree_to_shwi(arg) == get_value_of_const(use->name)) {
            DEBUGF("yup this is the constant we want\n");
            rep = i;
            break;
          }
        }
        if (rep != -1) {
          DEBUGF("replace here pls\n");
          TREE_OPERAND(t, rep) =
              build1(NOP_EXPR, integer_type_node, VAR_NAME_AS_TREE(use->name));
          // debug_tree(t);
          remove_subu_elem(ctx->list, use);
          use = NULL;
          if (get_subu_elem(ctx->list, loc, &use)) {
            /* there is another argument of this expression
             * that also had one of our macro expansions */
            goto check_expr_args;
          }
        }
      }
      ctx->cur = t;
      return NULL_TREE;
    }
  }

  ctx->prev = t;
  return NULL_TREE;
}

void process_body(tree *sptr, subu_list *list) {
  tree stmt = *sptr;
  context myctx;
  myctx.list = list;
  myctx.special = init_subu_list();
  myctx.prev = stmt;
  myctx.cur = NULL_TREE;
  walk_tree_without_duplicates(sptr, check_usage, &myctx);

  int errcount = 0;
  /* now at this stage, all uses of our macros have been
   * fixed, EXCEPT the case labels. Let's confirm that: */
  for (auto it = myctx.special->head; it; it = it->next) {
    if (it->tp == UNKNOWN) {
      error_at(it->loc, "invalid substitution of constant\n");
      errcount += 1;
    }
  }
  if (errcount != 0) {
    delete_subu_list(myctx.special);
    return;
  }
  /* now we can do what we want */
  delete_subu_list(myctx.special);
  myctx.special = NULL;
}
