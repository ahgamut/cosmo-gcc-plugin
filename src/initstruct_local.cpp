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

static inline tree build_modded_if_stmt(tree condition, tree then_clause,
                                        tree else_clause = NULL_TREE) {
  return build3(COND_EXPR, void_type_node, condition, then_clause, else_clause);
}

int build_modded_int_declaration(tree *dxpr, subu_node *use) {
  char chk[128];
  tree dcl = DECL_EXPR_DECL(*dxpr);

  if (INTEGRAL_TYPE_P(TREE_TYPE(dcl)) &&
      check_magic_equal(DECL_INITIAL(dcl), use->name)) {
    if (TREE_READONLY(dcl)) {
      error_at(EXPR_LOCATION(dcl), "cannot substitute this constant\n");
      /* actually I can, but the issue is if one of gcc's optimizations
       * perform constant folding(and they do), I don't know all the spots
       * where this variable has been folded, so I can't substitute there */
      return 0;
    }

    if (!TREE_STATIC(dcl)) {
      DECL_INITIAL(dcl) = VAR_NAME_AS_TREE(use->name);
      return 1;
    }

    DEBUGF("fixing decl for a static integer\n");
    /* (*dxpr), the statement we have is this:
     *
     * static int myvalue = __tmp_ifs_VAR;
     *
     * we're going to modify it to this:
     *
     * static int myvalue = __tmp_ifs_VAR;
     * static uint8 __chk_ifs_myvalue = 0;
     * if(__chk_ifs_myvalue != 1) {
     *   __chk_ifs_myvalue = 1;
     *   myvalue = VAR;
     * }
     *
     * so the modified statement runs exactly once,
     * whenever the function is first called, right
     * after the initialization of the variable we
     * wanted to modify. */

    /* build __chk_ifs_myvalue */
    snprintf(chk, sizeof(chk), "__chk_ifs_%s", IDENTIFIER_NAME(dcl));
    tree chknode = build_decl(DECL_SOURCE_LOCATION(dcl), VAR_DECL,
                              get_identifier(chk), uint8_type_node);
    DECL_INITIAL(chknode) = build_int_cst(uint8_type_node, 0);
    TREE_STATIC(chknode) = TREE_STATIC(dcl);
    TREE_USED(chknode) = TREE_USED(dcl);
    DECL_READ_P(chknode) = DECL_READ_P(dcl);
    DECL_CONTEXT(chknode) = DECL_CONTEXT(dcl);
    DECL_CHAIN(chknode) = DECL_CHAIN(dcl);
    DECL_CHAIN(dcl) = chknode;

    /* create the then clause of the if statement */
    tree then_clause = alloc_stmt_list();
    append_to_statement_list(build2(MODIFY_EXPR, void_type_node, chknode,
                                    build_int_cst(uint8_type_node, 1)),
                             &then_clause);
    append_to_statement_list(
        build2(MODIFY_EXPR, void_type_node, dcl, VAR_NAME_AS_TREE(use->name)),
        &then_clause);
    /*
    append_to_statement_list(
        build_call_expr(VAR_NAME_AS_TREE("printf"), 1,
                        BUILD_STRING_AS_TREE("initstruct magic\n")),
        &then_clause);
    */

    /* create the if statement into the overall result mentioned above */
    tree res = alloc_stmt_list();
    append_to_statement_list(*dxpr, &res);
    append_to_statement_list(build1(DECL_EXPR, void_type_node, chknode), &res);
    append_to_statement_list(
        build_modded_if_stmt(build2(NE_EXPR, void_type_node, chknode,
                                    build_int_cst(uint8_type_node, 1)),
                             then_clause),
        &res);
    /* overwrite the input tree with our new statements */
    *dxpr = res;
    // debug_tree(res);
    return 1;
  }
  return 0;
}

void modify_local_struct_ctor(tree ctor, subu_list *list, location_t bound) {
  subu_node *use = NULL;
  unsigned int iprev = 0;
  bool started = true;
  while (list->count > 0 && LOCATION_BEFORE2(list->start, bound)) {
    tree val = NULL_TREE;
    unsigned int i = 0;
    bool found = false;
    FOR_EACH_CONSTRUCTOR_VALUE(CONSTRUCTOR_ELTS(ctor), i, val) {
      DEBUGF("value %u is %s\n", i, get_tree_code_str(val));
      // debug_tree(val);
      if (TREE_CODE(val) == INTEGER_CST) {
        for (use = list->head; use; use = use->next) {
          found = check_magic_equal(val, use->name);
          if (found) break;
        }
        if (found) {
          iprev = i;
          started = false;
          break;
        }
      } else if (TREE_CODE(val) == CONSTRUCTOR) {
        modify_local_struct_ctor(val, list, bound);
        use = NULL; /* might've gotten stomped */
        if (list->count == 0 || LOCATION_AFTER2(list->start, bound)) return;
      }
    }
    if (found) {
      DEBUGF("found\n");
      // debug_tree(CONSTRUCTOR_ELT(ctor, i)->index);
      CONSTRUCTOR_ELT(ctor, i)->value = VAR_NAME_AS_TREE(use->name);
      // debug_tree(CONSTRUCTOR_ELT(ctor, i)->value);
      remove_subu_elem(list, use);
    } else {
      /* we did not find any (more) substitutions to fix */
      break;
    }
  }
}

tree copy_struct_ctor(tree ctor) {
  tree ind = NULL_TREE;
  tree val = NULL_TREE;
  constructor_elt x = {.index = NULL, .value = NULL};
  unsigned int i = 0;
  tree dupe = build0(CONSTRUCTOR, TREE_TYPE(ctor));
  FOR_EACH_CONSTRUCTOR_ELT(CONSTRUCTOR_ELTS(ctor), i, ind, val) {
    x.index = copy_node(ind);
    if (TREE_CODE(val) == CONSTRUCTOR) {
      x.value = copy_struct_ctor(val);
    } else {
      x.value = copy_node(val);
    }
    vec_safe_push(((tree_constructor *)(dupe))->elts, x);
  }
  return dupe;
}

void build_modded_declaration(tree *dxpr, SubContext *ctx, location_t bound) {
  char chk[128];
  tree dcl = DECL_EXPR_DECL(*dxpr);
  subu_node *use = NULL;
  subu_list *list = ctx->mods;
  unsigned int oldcount = list->count;

  // debug_tree(DECL_INITIAL(dcl));

  if (INTEGRAL_TYPE_P(TREE_TYPE(dcl))) {
    get_subu_elem(list, list->start, &use);
    if (build_modded_int_declaration(dxpr, use)) {
      remove_subu_elem(list, use);
      use = NULL;
      ctx->initcount += 1;
    }
    return;
  }

  if ((RECORD_TYPE == TREE_CODE(TREE_TYPE(dcl)) ||
       ARRAY_TYPE == TREE_CODE(TREE_TYPE(dcl))) &&
      DECL_INITIAL(dcl) != NULL_TREE) {
    if (TREE_READONLY(dcl)) {
      warning_at(EXPR_LOCATION(*dxpr), 0,
                 "not sure if modding const structs is good\n");
      TREE_READONLY(dcl) = 0;
      build_modded_declaration(dxpr, ctx, bound);
      return;
    } else if (TREE_STATIC(dcl)) {
      DEBUGF("fixing decl for a static struct\n");
      /* (*dxpr), the statement we have is this:
       *
       * static struct toy myvalue = {.x=1, .y=__tmp_ifs_VAR};
       *
       * we're going to modify it to this:
       *
       * static struct toy myvalue = {.x=1, .y=__tmp_ifs_VAR};
       * static uint8 __chk_ifs_myvalue = 0;
       * if(__chk_ifs_myvalue != 1) {
       *   struct toy __tmp_myvalue = {.x=1, .y=VAR};
       *   __chk_ifs_myvalue = 1;
       *   memcpy(&myvalue, &__tmp_myvalue, sizeof(myvalue));
       * }
       *
       * so the modified statement runs exactly once,
       * whenever the function is first called, right
       * after the initialization of the variable we
       * wanted to modify. */

      /* build __chk_ifs_myvalue */
      snprintf(chk, sizeof(chk), "__chk_ifs_%s", IDENTIFIER_NAME(dcl));
      tree chknode = build_decl(DECL_SOURCE_LOCATION(dcl), VAR_DECL,
                                get_identifier(chk), uint8_type_node);
      DECL_INITIAL(chknode) = build_int_cst(uint8_type_node, 0);
      TREE_STATIC(chknode) = TREE_STATIC(dcl);
      TREE_USED(chknode) = TREE_USED(dcl);
      DECL_READ_P(chknode) = DECL_READ_P(dcl);
      DECL_CONTEXT(chknode) = DECL_CONTEXT(dcl);
      DECL_CHAIN(chknode) = DECL_CHAIN(dcl);
      DECL_CHAIN(dcl) = chknode;

      /* build a scope block for the temporary value */
      tree tmpscope = build0(BLOCK, void_type_node);
      BLOCK_SUPERCONTEXT(tmpscope) = TREE_BLOCK(*dxpr);
      // debug_tree(BLOCK_SUPERCONTEXT(tmpscope));

      /* build __tmp_myvalue */
      snprintf(chk, sizeof(chk), "__tmp_ifs_%s", IDENTIFIER_NAME(dcl));
      tree tmpvar = build_decl(DECL_SOURCE_LOCATION(dcl), VAR_DECL,
                               get_identifier(chk), TREE_TYPE(dcl));
      DECL_INITIAL(tmpvar) = copy_struct_ctor(DECL_INITIAL(dcl));
      // debug_tree(DECL_INITIAL(tmpvar));
      TREE_USED(tmpvar) = TREE_USED(dcl);
      DECL_READ_P(tmpvar) = DECL_READ_P(dcl);
      DECL_CONTEXT(tmpvar) = DECL_CONTEXT(dcl);
      tree tmpexpr = build1(DECL_EXPR, void_type_node, tmpvar);
      TREE_SET_BLOCK(tmpexpr, tmpscope);
      BLOCK_VARS(tmpscope) = tmpvar;
      modify_local_struct_ctor(DECL_INITIAL(tmpvar), list, bound);

      /* create the then clause of the if statement */
      tree then_clause = alloc_stmt_list();
      append_to_statement_list(tmpexpr, &then_clause);
      append_to_statement_list(build2(MODIFY_EXPR, void_type_node, chknode,
                                      build_int_cst(uint8_type_node, 1)),
                               &then_clause);
      append_to_statement_list(
          build_call_expr(VAR_NAME_AS_TREE("__builtin_memcpy"), 3,
                          build1(NOP_EXPR, void_type_node,
                                 build1(ADDR_EXPR, ptr_type_node, dcl)),
                          build1(NOP_EXPR, void_type_node,
                                 build1(ADDR_EXPR, ptr_type_node, tmpvar)),
                          DECL_SIZE_UNIT(dcl)),
          &then_clause);
      /*
      append_to_statement_list(
          build_call_expr(VAR_NAME_AS_TREE("printf"), 2,
                          BUILD_STRING_AS_TREE("initstruct magic %lu bytes\n"),
                          DECL_SIZE_UNIT(dcl)),
          &then_clause);
      */

      /* create the if statement into the overall result mentioned above */
      tree res = alloc_stmt_list();
      append_to_statement_list(*dxpr, &res);
      append_to_statement_list(build1(DECL_EXPR, void_type_node, chknode),
                               &res);
      append_to_statement_list(
          build_modded_if_stmt(
              build2(NE_EXPR, void_type_node, chknode,
                     build_int_cst(uint8_type_node, 1)),
              build3(BIND_EXPR, void_type_node, tmpvar, then_clause, tmpscope)),
          &res);
      /* overwrite the input tree with our new statements */
      *dxpr = res;
    } else {
      /* if it's a local struct, we can
       * just mod the constructor itself */
      auto ctor = DECL_INITIAL(dcl);
      modify_local_struct_ctor(ctor, list, bound);
    }
  }
  ctx->initcount += (oldcount - list->count);
}
