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
#include <portcosmo.h>

int check_expr(tree *tp, SubContext *ctx, location_t loc, source_range rng) {
  tree t = *tp;
  subu_node *use = NULL;
  tree replacement = NULL_TREE;
  int found = 0;

  if (TREE_CODE(t) == DECL_EXPR) {
    get_subu_elem(ctx->mods, loc, &use) || get_subu_elem2(ctx->mods, rng, &use);
    if (build_modded_int_declaration(tp, ctx, use)) {
      use = NULL;
      ctx->initcount += 1;
      DEBUGF("fixed\n");
      ctx->prev = NULL;
      found = 1;
    }
  } else if (TREE_CODE(t) == BIND_EXPR) {
    // debug_tree(t);
    auto body = BIND_EXPR_BODY(t);
    if (EXPR_P(body) && check_expr(&body, ctx, loc, rng)) {
      found += 1;
    } else if (TREE_CODE(body) == STATEMENT_LIST) {
      for (auto it = tsi_start(body); !tsi_end_p(it); tsi_next(&it)) {
        auto tp2 = tsi_stmt_ptr(it);
        if (EXPR_P(*tp2) && check_expr(tp2, ctx, loc, rng)) {
          found += 1;
        }
      }
    }
    // debug_tree(t);
  } else if (TREE_CODE(t) == CALL_EXPR) {
    call_expr_arg_iterator it;
    tree arg = NULL_TREE;
    while (get_subu_elem(ctx->mods, loc, &use) ||
           get_subu_elem2(ctx->mods, rng, &use)) {
      int i = 0;
      int rep = -1;
      FOR_EACH_CALL_EXPR_ARG(arg, it, t) {
        DEBUGF("arg %d is %s (%s)\n", i, get_tree_code_str(arg), use->name);
        // debug_tree(arg);
        if (arg_should_be_modded(arg, use, &replacement)) {
          DEBUGF("yup this is the constant we want\n");
          CALL_EXPR_ARG(t, i) = replacement;
          remove_subu_elem(ctx->mods, use);
          use = NULL;
          ctx->subcount += 1;
          rep = i;
          found += 1;
          replacement = NULL_TREE;
          break;
        } else if (EXPR_P(arg) && check_expr(&arg, ctx, loc, rng)) {
          use = NULL;
          found += 1;
          rep = i;
          break;
        }
        i += 1;
      }
      if (rep == -1) break;
    }
    /* TODO: did we substitute something we weren't
     * supposed to substitute? check via hash-set? */
  } else {
    DEBUGF("this contains a substitution and it is... a %s?\n",
           get_tree_code_str(t));
    DEBUGF("how many operands do you have? %d\n", TREE_OPERAND_LENGTH(t));
    // debug_tree(t);
    tree arg = NULL_TREE;
    int n = TREE_OPERAND_LENGTH(t);
    while (get_subu_elem(ctx->mods, loc, &use) ||
           get_subu_elem2(ctx->mods, rng, &use)) {
      int i = 0;
      int rep = -1;
      for (i = 0; i < n; ++i) {
        arg = TREE_OPERAND(t, i);
        if (!arg || arg == NULL_TREE) continue;
        DEBUGF("arg %d is %s (%s)\n", i, get_tree_code_str(arg), use->name);
        // debug_tree(arg);
        if (arg_should_be_modded(arg, use, &replacement)) {
          DEBUGF("yup this is the constant we want\n");
          rep = i;
          TREE_OPERAND(t, i) = replacement;
          remove_subu_elem(ctx->mods, use);
          use = NULL;
          ctx->subcount += 1;
          found += 1;
          replacement = NULL_TREE;
          break;
        } else if (EXPR_P(arg) && check_expr(&arg, ctx, loc, rng)) {
          use = NULL;
          found += 1;
          rep = i;
          break;
        }
      }
      if (rep == -1) break;
    }
    /* TODO: did we substitute something we weren't
     * supposed to substitute? check via hash-set? */
  }
  return found;
}

tree check_usage(tree *tp, int *check_subtree, void *data) {
  SubContext *ctx = (SubContext *)(data);
  tree t = *tp;
  tree z;
  subu_node *use = NULL;
  location_t loc = EXPR_LOCATION(t);
  source_range rng = EXPR_LOCATION_RANGE(t);

  if (ctx->active == 0 || ctx->mods->count == 0) {
    /* DEBUGF("substitutions complete\n"); */
    *check_subtree = 0;
    return NULL_TREE;
  }

  if (LOCATION_AFTER2(loc, rng.m_start)) {
    loc = rng.m_start;
  } else {
    rng.m_start = loc;
  }

  if (ctx->prev && LOCATION_BEFORE2(ctx->mods->start, rng.m_start)) {
    auto vloc = DECL_SOURCE_LOCATION(DECL_EXPR_DECL(*(ctx->prev)));
    /* below inequality holds inside this if condition:
     *    vloc <= ctx->mods->start <= rng.m_start
     * this means that there was a macro substitution
     * between vloc and rng.m_start, which was not
     * eliminated when we went through the other parts
     * of the parse tree earlier. thus, the decl_expr
     * that we have stored in ctx->prev needs to be
     * checked for possible macro substitutions */
    DEBUGF("did we miss a decl? vloc=%u,%u, loc=%u,%u, rng.mstart=%u,%u, "
           "start=%u,%u\n",
           LOCATION_LINE(vloc), LOCATION_COLUMN(vloc),  //
           LOCATION_LINE(loc), LOCATION_COLUMN(loc),    //
           LOCATION_LINE(rng.m_start), LOCATION_COLUMN(rng.m_start),
           LOCATION_LINE(ctx->mods->start), LOCATION_COLUMN(ctx->mods->start));
    auto z = ctx->initcount;
    build_modded_declaration(ctx->prev, ctx, rng.m_start);
    if (z != ctx->initcount) {
      ctx->prev = NULL;
      check_context_clear(ctx, loc);
    }
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
     * anyway (UM....) go through their subtrees, which will have the
     * expressions we need to mod */
    return NULL_TREE;
  }

  if (TREE_CODE(t) == DECL_EXPR) {
    DEBUGF("decl_expr at (%u,%u)-(%u,%u)\n", LOCATION_LINE(rng.m_start),
           LOCATION_COLUMN(rng.m_start), LOCATION_LINE(rng.m_finish),
           LOCATION_COLUMN(rng.m_finish));

    auto code_type = TREE_CODE(TREE_TYPE(DECL_EXPR_DECL(t)));
    if (POINTER_TYPE == code_type) {
      return NULL_TREE;
    }
    // debug_tree(DECL_EXPR_DECL(t));
    if ((RECORD_TYPE == code_type || ARRAY_TYPE == code_type) &&
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
      auto vloc = DECL_SOURCE_LOCATION(DECL_EXPR_DECL(t));
      // debug_tree(t);
      if (DECL_INITIAL(DECL_EXPR_DECL(t)) != NULL_TREE &&
          LOCATION_LINE(ctx->mods->start) - LOCATION_LINE(vloc) < 20) {
        /* if you have a multi-line comment of more than 20 lines
         * between a variable declaration and it's initial value,
         * then the plugin will fail. This deterministic failure is
         * better than crashing randomly because we tried to DECL_INITIAL
         * some variable which had no macro substitutions near it.
         *
         * TODO: can we get better location handling for comments?
         * that might help with initializers, although there is a
         * nastier alternative. */
        DEBUGF("just in case %u-%u, %u-%u\n", LOCATION_LINE(ctx->mods->start),
               LOCATION_COLUMN(ctx->mods->start), LOCATION_LINE(ctx->mods->end),
               LOCATION_COLUMN(ctx->mods->end));
        ctx->prev = tp;
      }
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
      check_expr(tp, ctx, loc, rng);
      return NULL_TREE;
    }
    DEBUGF("got in but no use %s at %u-%u\n", get_tree_code_str(t),
           LOCATION_LINE(rng.m_start), LOCATION_LINE(rng.m_finish));

    // debug_tree(t);
  }
  return NULL_TREE;
}

void handle_pre_genericize(void *gcc_data, void *user_data) {
  tree t = (tree)gcc_data;
  SubContext *ctx = (SubContext *)user_data;
  tree t2;
  if (ctx->active && TREE_CODE(t) == FUNCTION_DECL && DECL_INITIAL(t) != NULL &&
      TREE_STATIC(t)) {
    /* this function is defined within the file I'm processing */
    if (ctx->mods->count == 0) {
      // DEBUGF("no substitutions were made in %s\n", IDENTIFIER_NAME(t));
      return;
    }
    t2 = DECL_SAVED_TREE(t);
    ctx->prev = NULL;
    walk_tree_without_duplicates(&t2, check_usage, ctx);
    /* now at this stage, all uses of our macros have been
     * fixed, INCLUDING case labels. Let's confirm that: */
    check_context_clear(ctx, MAX_LOCATION_T);
  }
}
