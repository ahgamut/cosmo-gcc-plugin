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

tree check_usage(tree *tp, int *check_subtree, void *data) {
  SubContext *ctx = (SubContext *)(data);
  tree t = *tp;
  tree z;
  subu_node *use = NULL;
  location_t loc = EXPR_LOCATION(t);
  source_range rng;
  rng = EXPR_LOCATION_RANGE(t);

  if (ctx->active == 0 || ctx->mods->count == 0) {
    /* DEBUGF("substitutions complete\n"); */
    *check_subtree = 0;
    return NULL_TREE;
  }

  if (ctx->prev && TREE_CODE(*(ctx->prev)) == DECL_EXPR &&
      (LOCATION_BEFORE2(ctx->mods->start, loc) ||
       LOCATION_BEFORE2(ctx->mods->start, rng.m_start))) {
    // debug_tree(t);
    DEBUGF("did we miss a decl?\n");
    if (loc != rng.m_start) loc = rng.m_start;
    build_modded_declaration(ctx->prev, ctx, loc);
    ctx->prev = NULL;
    check_context_clear(ctx, loc);
  }

  if (TREE_CODE(t) == SWITCH_STMT) {
    rng = get_switch_bounds(t);
    if (valid_subu_bounds(ctx->mods, rng.m_start, rng.m_finish) &&
        count_mods_in_switch(t, ctx->mods) > 0) {
      /* this is one of the switch statements
       * where we modified a case label */
      DEBUGF("modding the switch \n");
      *tp = build_modded_switch_stmt(t, ctx);
      DEBUGF("we modded it??\n");
      walk_tree_without_duplicates(tp, check_usage, ctx);
      /* due to the above call, I don't need to check
       * any subtrees from this current location */
      *check_subtree = 0;
      ctx->switchcount += 1;
      return NULL_TREE;
    }
  }

  if (TREE_CODE(t) == FOR_STMT || TREE_CODE(t) == WHILE_STMT ||
      TREE_CODE(t) == IF_STMT || TREE_CODE(t) == DO_STMT ||
      TREE_CODE(t) == USING_STMT || TREE_CODE(t) == CLEANUP_STMT ||
      TREE_CODE(t) == TRY_BLOCK || TREE_CODE(t) == HANDLER) {
    /* there's nothing to check in these statements, the walk will
     * anyway go through their subtrees, which will have the
     * expressions we need to mod */
    return NULL_TREE;
  }

  if (TREE_CODE(t) == DECL_EXPR) {
    DEBUGF("hi decl_expr for %s at (%u,%u)-(%u,%u)\n",
           IDENTIFIER_NAME(DECL_EXPR_DECL(t)), LOCATION_LINE(rng.m_start),
           LOCATION_COLUMN(rng.m_start), LOCATION_LINE(rng.m_finish),
           LOCATION_COLUMN(rng.m_finish));
    // debug_tree(DECL_EXPR_DECL(t));
    if ((RECORD_TYPE == TREE_CODE(TREE_TYPE(DECL_EXPR_DECL(t))) ||
         ARRAY_TYPE == TREE_CODE(TREE_TYPE(DECL_EXPR_DECL(t)))) &&
        DECL_INITIAL(DECL_EXPR_DECL(t)) != NULL_TREE) {
      // debug_tree(DECL_INITIAL(DECL_EXPR_DECL(t)));
      ctx->prev = tp;
      return NULL_TREE;
    }

    if (INTEGRAL_TYPE_P(TREE_TYPE(DECL_EXPR_DECL(t)))) {
      /* suppose we have recorded a macro use
       * related to this declaration, but then
       * somehow, we did not actually fix the
       * substituted value in the below code.
       * something's fishy: maybe there is a
       * multi-line comment between the name
       * of the variable and the actual value.
       * we'll check again before the next
       * element in the AST, just in case */
      DEBUGF("just in case\n");
      ctx->prev = tp;
    }
  }

  if (valid_subu_bounds(ctx->mods, rng.m_start, rng.m_finish) ||
      check_loc_in_bound(ctx->mods, loc)) {
    if (get_subu_elem(ctx->mods, loc, &use) ||
        get_subu_elem2(ctx->mods, rng, &use)) {
      /* one of our macro substitutions
       * has been executed either at the location loc,
       * or within the range rng */
      DEBUGF("found mark at %u,%u in a %s\n", LOCATION_LINE(loc),
             LOCATION_COLUMN(loc), get_tree_code_str(t));
      if (TREE_CODE(t) == DECL_EXPR) {
        if (build_modded_int_declaration(tp, ctx, use)) {
          use = NULL;
          ctx->initcount += 1;
          DEBUGF("fixed\n");
          ctx->prev = NULL;
        }
      } else if (TREE_CODE(t) == CALL_EXPR) {
      check_call_args:
        call_expr_arg_iterator it;
        tree arg = NULL_TREE;
        int i = 0;
        int rep = -1;
        FOR_EACH_CALL_EXPR_ARG(arg, it, t) {
          DEBUGF("arg %d is %s\n", i, get_tree_code_str(arg));
          // debug_tree(arg);
          if ((TREE_CODE(arg) == INTEGER_CST &&
               check_magic_equal(arg, use->name)) ||
              (TREE_CODE(arg) == NOP_EXPR &&
               TREE_OPERAND(arg, 0) == get_ifsw_identifier(use->name))) {
            DEBUGF("yup this is the constant we want\n");
            rep = i;
            break;
          }
          i += 1;
        }
        if (rep != -1) {
          CALL_EXPR_ARG(t, rep) =
              build1(NOP_EXPR, integer_type_node, VAR_NAME_AS_TREE(use->name));
          // debug_tree(t);
          remove_subu_elem(ctx->mods, use);
          use = NULL;
          ctx->subcount += 1;
          if (get_subu_elem(ctx->mods, loc, &use) ||
              get_subu_elem2(ctx->mods, rng, &use)) {
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
          // debug_tree(arg);
          if ((TREE_CODE(arg) == INTEGER_CST &&
               check_magic_equal(arg, use->name)) ||
              (TREE_CODE(arg) == NOP_EXPR &&
               TREE_OPERAND(arg, 0) == get_ifsw_identifier(use->name))) {
            DEBUGF("yup this is the constant we want\n");
            rep = i;
            break;
          }
        }
        if (rep != -1 && TREE_CODE(t) != CASE_LABEL_EXPR) {
          TREE_OPERAND(t, rep) =
              build1(NOP_EXPR, integer_type_node, VAR_NAME_AS_TREE(use->name));
          // debug_tree(t);
          remove_subu_elem(ctx->mods, use);
          use = NULL;
          ctx->subcount += 1;
          if (get_subu_elem(ctx->mods, loc, &use) ||
              get_subu_elem2(ctx->mods, rng, &use)) {
            /* there is another argument of this expression
             * that also had one of our macro expansions */
            goto check_expr_args;
          }
        }
      }
      return NULL_TREE;
    }
    DEBUGF("got in but no use %s at %u-%u\n", get_tree_code_str(t),
           LOCATION_LINE(rng.m_start), LOCATION_LINE(rng.m_finish));

    // debug_tree(t);
  }
  return NULL_TREE;
}

void process_body(tree *sptr, SubContext *ctx) {
  walk_tree_without_duplicates(sptr, check_usage, ctx);
  /* now at this stage, all uses of our macros have been
   * fixed, INCLUDING case labels. Let's confirm that: */
  check_context_clear(ctx, MAX_LOCATION_T);
}
