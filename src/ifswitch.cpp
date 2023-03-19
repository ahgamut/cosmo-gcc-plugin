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

source_range get_switch_bounds(tree sws) {
  auto rng1 =
      EXPR_LOCATION_RANGE(STATEMENT_LIST_HEAD(SWITCH_STMT_BODY(sws))->stmt);
  auto rng2 =
      EXPR_LOCATION_RANGE(STATEMENT_LIST_TAIL(SWITCH_STMT_BODY(sws))->stmt);
  source_range rng;
  rng.m_start = rng1.m_start;
  rng.m_finish = rng2.m_finish;
  return rng;
}

unsigned int count_mods_in_switch(tree swexpr, subu_list *list) {
  tree body = SWITCH_STMT_BODY(swexpr);
  tree t = NULL;
  subu_node *use = NULL;
  unsigned int count = 0;
  for (auto i = tsi_start(body); !tsi_end_p(i); tsi_next(&i)) {
    t = tsi_stmt(i);
    if (TREE_CODE(t) == CASE_LABEL_EXPR) {
      if (get_subu_elem(list, EXPR_LOCATION(t),
                        &use)          /* on a line we substituted */
          && CASE_LOW(t) != NULL_TREE  /* not a x..y range */
          && CASE_HIGH(t) == NULL_TREE /* not a default */
          && check_magic_equal(CASE_LOW(t), use->name)
          /* the case is the one we substituted */) {
        DEBUGF("we substituted a case label at %u,%u\n", EXPR_LOC_LINE(t),
               EXPR_LOC_COL(t));
        // debug_tree(CASE_LOW(t));
        count += 1;
      }
    }
  }
  return count;
}

tree build_modded_label(unsigned int swcount, const char *case_str,
                        location_t loc = UNKNOWN_LOCATION) {
  char dest[128] = {0};
  snprintf(dest, sizeof(dest), "__tmp_ifs_%u_%s", swcount, case_str);
  tree lab = build_decl(loc, LABEL_DECL, get_identifier(dest), void_type_node);
  /* gcc's GIMPLE needs to know that this label
   * is within the current function declaration */
  DECL_CONTEXT(lab) = current_function_decl;
  return build1(LABEL_EXPR, void_type_node, lab);
}

tree build_modded_exit_label(unsigned int swcount) {
  return build_modded_label(swcount, "__end");
}

static inline tree build_modded_if_stmt(tree condition, tree then_clause,
                                        tree else_clause = NULL_TREE) {
  return build3(COND_EXPR, void_type_node, condition, then_clause, else_clause);
}

tree modded_case_label(tree t, unsigned int i, tree swcond, vec<tree> *&ifs,
                       SubContext *ctx, tree *default_label) {
  // debug_tree(t);
  tree result;
  subu_node *use = NULL;
  char case_str[128] = {0};

  if (CASE_LOW(t) == NULL_TREE) {
    DEBUGF("default case\n");
    /* default label of the switch case, needs to be last */
    result = build_modded_label(ctx->switchcount, "__dflt", EXPR_LOCATION(t));
    *default_label = result;
  } else if (CASE_LOW(t) != NULL_TREE && CASE_HIGH(t) == NULL_TREE) {
    /* a case label */
    if (get_subu_elem(ctx->mods, EXPR_LOCATION(t), &use)
        /* the case is on a line we substituted */
        && check_magic_equal(CASE_LOW(t), use->name)
        /* the case value is the one we substituted */) {
      DEBUGF("modded case\n");
      result =
          build_modded_label(ctx->switchcount, use->name, EXPR_LOCATION(t));
      ifs->safe_push(build_modded_if_stmt(
          build2(EQ_EXPR, integer_type_node, swcond,
                 VAR_NAME_AS_TREE(use->name)),
          build1(GOTO_EXPR, void_type_node, LABEL_EXPR_LABEL(result))));
      remove_subu_elem(ctx->mods, use);
    } else {
      /* a case label that we didn't substitute */
      DEBUGF("unmodded case\n");
      snprintf(case_str, sizeof(case_str), "%lx_", i);
      result = build_modded_label(ctx->switchcount, case_str, EXPR_LOCATION(t));
      ifs->safe_push(build_modded_if_stmt(
          build2(EQ_EXPR, integer_type_node, swcond, CASE_LOW(t)),
          build1(GOTO_EXPR, void_type_node, LABEL_EXPR_LABEL(result))));
    }
  } else {
    DEBUGF("unmodded case range\n");
    /* CASE_LOW(t) != NULL_TREE && CASE_HIGH(t) != NULL_TREE */
    /* this is a case x .. y sort of range */
    snprintf(case_str, sizeof(case_str), "%lx_", i);
    result = build_modded_label(ctx->switchcount, case_str, EXPR_LOCATION(t));
    ifs->safe_push(build_modded_if_stmt(
        build2(TRUTH_ANDIF_EXPR, integer_type_node,
               build2(GE_EXPR, integer_type_node, swcond, CASE_LOW(t)),
               build2(LE_EXPR, integer_type_node, swcond, CASE_HIGH(t))),
        build1(GOTO_EXPR, void_type_node, LABEL_EXPR_LABEL(result))));
  }
  return result;
}

tree build_modded_switch_stmt(tree swexpr, SubContext *ctx) {
  int case_count = 0, break_count = 0;
  int has_default = 0;

  tree swcond = save_expr(SWITCH_STMT_COND(swexpr));
  tree swbody = SWITCH_STMT_BODY(swexpr);
  tree *tp = NULL;
  char dest[128] = {0};

  vec<tree> *ifs;
  vec_alloc(ifs, 0);

  tree exit_label = build_modded_exit_label(ctx->switchcount);
  tree default_label = NULL_TREE;

  for (auto it = tsi_start(swbody); !tsi_end_p(it); tsi_next(&it)) {
    tp = tsi_stmt_ptr(it);
    if (TREE_CODE(*tp) == CASE_LABEL_EXPR) {
      case_count += 1;
      has_default = has_default || (CASE_LOW(*tp) == NULL_TREE);
      /* replace the case statement with a goto */
      *tp =
          modded_case_label(*tp, case_count, swcond, ifs, ctx, &default_label);
    } else if (TREE_CODE(*tp) == BREAK_STMT) {
      break_count += 1;
      /* replace the break statement with a goto to the end */
      *tp = build1(GOTO_EXPR, void_type_node, LABEL_EXPR_LABEL(exit_label));
    }
  }
  /* add all the if statements to the start of the switch body */
  /* TODO: do we have to combine them via COND_EXPR_ELSE? why,
   * is it not possible to just them as a list one after the other? */
  tree res;
  unsigned int zz = 0;
  if (ifs->length() > 0) {
    res = (*ifs)[0];
    for (zz = 1; zz < ifs->length(); ++zz) {
      COND_EXPR_ELSE(res) = (*ifs)[zz];
      res = (*ifs)[zz];
    }
    COND_EXPR_ELSE(res) =
        build1(GOTO_EXPR, void_type_node, LABEL_EXPR_LABEL(default_label));
    res = (*ifs)[0];
  } else if (has_default && default_label != NULL_TREE) {
    res = build1(GOTO_EXPR, void_type_node, LABEL_EXPR_LABEL(default_label));
  } else {
    /* this switch has no cases, and no default?! */
    error_at(EXPR_LOCATION(swcond), "what is this switch\n");
  }
  auto it = tsi_start(swbody);
  tsi_link_before(&it, res, TSI_SAME_STMT);
  tsi_link_before(&it, build_empty_stmt(UNKNOWN_LOCATION), TSI_SAME_STMT);

  /* add the 'outside' of the switch, ie the 'finally'
   * aka the target of the break statements, the 'exit_label',
   * to the end of the switch body */
  append_to_statement_list(build_empty_stmt(UNKNOWN_LOCATION), &swbody);
  append_to_statement_list(exit_label, &swbody);
  append_to_statement_list(build_empty_stmt(UNKNOWN_LOCATION), &swbody);
  /*
  snprintf(dest, sizeof(dest),
           "above switch had %d cases replaced and %d breaks\n", case_count,
           break_count);
  append_to_statement_list(build_call_expr(VAR_NAME_AS_TREE("printf"), 1,
                                           BUILD_STRING_AS_TREE(dest)),
                           &swbody); */

  /* debug_tree(swbody); */
  return swbody;
}
