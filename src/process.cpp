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
#define BUILD_STRING_AS_TREE(str) build_string_literal(strlen((str)) + 1, (str))

struct context {
  subu_list *list;
  subu_list *special;
  tree cur;
  tree prev;
};

tree build_modded_case_label(unsigned int count, const char *case_str,
                             location_t loc = UNKNOWN_LOCATION) {
  char dest[96] = {0};
  snprintf(dest, sizeof(dest), "__tmp_ifs_%u_%s", count, case_str);
  tree lab = build_decl(loc, LABEL_DECL, get_identifier(dest), void_type_node);
  /* gcc's GIMPLE needs to know that this label
   * is within the current function declaration */
  DECL_CONTEXT(lab) = current_function_decl;
  return build1(LABEL_EXPR, void_type_node, lab);
}

tree build_modded_exit_label(unsigned int count) {
  return build_modded_case_label(count, "__end");
}

tree build_modded_if_stmt(tree condition) {
  return NULL_TREE;
}

void load_cases_and_conditions(tree swexpr, vec<tree> *&ifs,
                               vec<tree> *&case_labels, tree *default_label,
                               subu_list *list, unsigned int count) {
  tree t;
  tree swcond = save_expr(SWITCH_STMT_COND(swexpr));
  tree swbody = SWITCH_STMT_BODY(swexpr);

  tree case_label = NULL_TREE;
  char case_str[72] = {0};

  subu_node *use = NULL;
  for (auto it = tsi_start(swbody); !tsi_end_p(it); tsi_next(&it)) {
    t = tsi_stmt(it);
    if (TREE_CODE(t) == CASE_LABEL_EXPR) {
      if (CASE_LOW(t) == NULL_TREE) {
        /* default label of the switch case, needs to be last */
        *default_label =
            build_modded_case_label(count, "__dflt", EXPR_LOCATION(t));
      } else if (CASE_LOW(t) != NULL_TREE && CASE_HIGH(t) == NULL_TREE) {
        /* a case label */
        if (get_subu_elem(list, EXPR_LOCATION(t), &use)
            /* the case is on a line we substituted */
            && tree_to_shwi(CASE_LOW(t)) == get_value_of_const(use->name)
            /* the case value is the one we substituted */) {
          remove_subu_elem(list, use);
          case_label =
              build_modded_case_label(count, use->name, EXPR_LOCATION(t));
          ifs->safe_push(build_modded_if_stmt(build2(
              EQ_EXPR, void_type_node, swcond, get_identifier(use->name))));
        } else {
          /* a case label that we didn't substitute */
          snprintf(case_str, sizeof(case_str), "%x", tree_to_shwi(CASE_LOW(t)));
          case_label =
              build_modded_case_label(count, case_str, EXPR_LOCATION(t));
          ifs->safe_push(build_modded_if_stmt(
              build2(EQ_EXPR, void_type_node, swcond, CASE_LOW(t))));
        }
        case_labels->safe_push(case_label);
      } else {
        /* CASE_LOW(t) != NULL_TREE && CASE_HIGH(t) != NULL_TREE */
        /* this is a case x .. y sort of range */
        snprintf(case_str, sizeof(case_str), "%x_%x", tree_to_shwi(CASE_LOW(t)),
                 tree_to_shwi(CASE_HIGH(t)));
        case_label = build_modded_case_label(count, case_str, EXPR_LOCATION(t));
        case_labels->safe_push(case_label);
      }
    }
    case_label = NULL_TREE;
  }
}

tree build_modded_switch_stmt(tree swexpr, subu_list *list) {
  int case_count = 0, glitch_count = 0, break_count = 0;
  tree stmt = SWITCH_STMT_BODY(swexpr);
  tree t = NULL;
  tree *tp = NULL;
  subu_node *use = NULL;
  char dest[64] = {0};
  unsigned int count = 1;

  vec<tree> *ifss = (vec<tree> *)xmalloc(sizeof(vec<tree>));
  vec<tree> *case_labels = (vec<tree> *)xmalloc(sizeof(vec<tree>));
  tree exit_label = build_modded_exit_label(count);
  tree default_label = NULL_TREE;
  // load_cases_and_conditions(swexpr, ifs, case_labels, &default_label, list,
  // count);

  for (auto it = tsi_start(stmt); !tsi_end_p(it); tsi_next(&it)) {
    tp = tsi_stmt_ptr(it);
    if (TREE_CODE(*tp) == CASE_LABEL_EXPR) {
      case_count += 1;
      if (get_subu_elem(list, EXPR_LOCATION(*tp),
                        &use)            /* on a line we substituted */
          && CASE_LOW(*tp) != NULL_TREE  /* not a x..y range */
          && CASE_HIGH(*tp) == NULL_TREE /* not a default */
          && tree_to_shwi(CASE_LOW(*tp)) == get_value_of_const(use->name)
          /* the case is the one we substituted */) {
        glitch_count += 1;
        remove_subu_elem(list, use);
      }
    } else if (TREE_CODE(*tp) == BREAK_STMT) {
      break_count += 1;
      /* replace the break statement with a goto to the end */
      *tp = build1(GOTO_EXPR, void_type_node, LABEL_EXPR_LABEL(exit_label));
    }
  }
  append_to_statement_list(build_empty_stmt(UNKNOWN_LOCATION), &stmt);
  append_to_statement_list(exit_label, &stmt);
  append_to_statement_list(build_empty_stmt(UNKNOWN_LOCATION), &stmt);
  snprintf(dest, sizeof(dest),
           "above switch had %d cases (%d replaced) and %d breaks\n",
           case_count, glitch_count, break_count);
  append_to_statement_list(build_call_expr(VAR_NAME_AS_TREE("printf"), 1,
                                           BUILD_STRING_AS_TREE(dest)),
                           &stmt);
  return swexpr;
}

int count_mods_in_switch(tree swexpr, subu_list *list) {
  tree body = SWITCH_STMT_BODY(swexpr);
  tree t = NULL;
  subu_node *use = NULL;
  int modcount = 0;
  for (auto i = tsi_start(body); !tsi_end_p(i); tsi_next(&i)) {
    t = tsi_stmt(i);
    if (TREE_CODE(t) == CASE_LABEL_EXPR) {
      if (get_subu_elem(list, EXPR_LOCATION(t),
                        &use)          /* on a line we substituted */
          && CASE_LOW(t) != NULL_TREE  /* not a x..y range */
          && CASE_HIGH(t) == NULL_TREE /* not a default */
          && tree_to_shwi(CASE_LOW(t)) == get_value_of_const(use->name)
          /* the case is the one we substituted */) {
        modcount += 1;
      }
    }
  }
  return modcount;
}

static source_range get_switch_bounds(tree sws) {
  auto rng1 =
      EXPR_LOCATION_RANGE(STATEMENT_LIST_HEAD(SWITCH_STMT_BODY(sws))->stmt);
  auto rng2 =
      EXPR_LOCATION_RANGE(STATEMENT_LIST_TAIL(SWITCH_STMT_BODY(sws))->stmt);
  source_range rng;
  rng.m_start = rng1.m_start;
  rng.m_finish = rng2.m_finish;
  return rng;
}

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

  if (TREE_CODE(t) == SWITCH_STMT) {
    source_range rng = get_switch_bounds(t);
    if (valid_subu_bounds(ctx->list, rng.m_start, rng.m_finish) &&
        count_mods_in_switch(t, ctx->list) > 0) {
      /* this is one of the switch statements
       * where we modified a case label */
      DEBUGF("modding the switch \n");
      *tp = build_modded_switch_stmt(t, ctx->list);
      t = SWITCH_STMT_BODY(*tp);
      walk_tree_without_duplicates(&t, check_usage, ctx);
      /* due to the above call, I don't need to check
       * any subtrees from this current location */
      *check_subtree = 0;
    }
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
  for (auto it = list->head; it; it->next) {
    error_at(it->loc, "unable to substitute constant\n");
    errcount += 1;
  }
  if (errcount != 0) {
    delete_subu_list(myctx.special);
    return;
  }
  /* now we can do what we want */
  delete_subu_list(myctx.special);
  myctx.special = NULL;
}
