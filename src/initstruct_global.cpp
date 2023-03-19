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

void create_hidden_ctor(tree tmpvar, tree body) {
  tree block = make_node(BLOCK);
  BLOCK_VARS(block) = tmpvar;
  TREE_SET_BLOCK(tmpvar, block);

  tree bexpr = build3(BIND_EXPR, void_type_node, tmpvar, body, block);
  cgraph_build_static_cdtor('I', bexpr, 0);
}

void rewrite_declarations_into_ctor(tree dcl, SubContext *ctx) {
  tree body = alloc_stmt_list();
  subu_node *use = NULL;
  char chk[128];
  if (INTEGRAL_TYPE_P(TREE_TYPE(dcl)) &&
      get_subu_elem(ctx->mods, ctx->mods->start, &use) &&
      /* use is non-NULL if get_subu_elem succeeds */
      check_magic_equal(DECL_INITIAL(dcl), use->name)) {
    if (TREE_READONLY(dcl)) {
      error_at(EXPR_LOCATION(dcl), "cannot substitute this constant\n");
      /* actually I can, but the issue is if one of gcc's optimizations
       * perform constant folding(and they do), I don't know all the spots
       * where this variable has been folded, so I can't substitute there */
      clear_subu_list(ctx->mods);
      return;
    }
    append_to_statement_list(
        build2(MODIFY_EXPR, void_type_node, dcl, VAR_NAME_AS_TREE(use->name)),
        &body);
    append_to_statement_list(
        build_call_expr(VAR_NAME_AS_TREE("printf"), 2,
                        BUILD_STRING_AS_TREE("ctor initstruct %s\n"),
                        BUILD_STRING_AS_TREE(IDENTIFIER_NAME(dcl))),
        &body);
    remove_subu_elem(ctx->mods, use);
    cgraph_build_static_cdtor('I', body, 0);
  } else if ((RECORD_TYPE == TREE_CODE(TREE_TYPE(dcl)) ||
              ARRAY_TYPE == TREE_CODE(TREE_TYPE(dcl))) &&
             DECL_INITIAL(dcl) != NULL_TREE) {
    if (TREE_READONLY(dcl)) {
      warning_at(DECL_SOURCE_LOCATION(dcl), 0,
                 "not sure if modding const structs is good\n");
      TREE_READONLY(dcl) = 0;
    }

    snprintf(chk, sizeof(chk), "__tmp_ifs_%s", IDENTIFIER_NAME(dcl));
    tree tmpvar = build_decl(UNKNOWN_LOCATION, VAR_DECL, get_identifier(chk),
                             TREE_TYPE(dcl));
    DECL_INITIAL(tmpvar) = copy_struct_ctor(DECL_INITIAL(dcl));
    // debug_tree(DECL_INITIAL(tmpvar));
    TREE_USED(tmpvar) = TREE_USED(dcl);
    DECL_READ_P(tmpvar) = DECL_READ_P(dcl);
    tree tmpexpr = build1(DECL_EXPR, void_type_node, tmpvar);
    modify_local_struct_ctor(DECL_INITIAL(tmpvar), ctx->mods, ctx->mods->end);

    append_to_statement_list(tmpexpr, &body);
    append_to_statement_list(
        build_call_expr(VAR_NAME_AS_TREE("__builtin_memcpy"), 3,
                        build1(NOP_EXPR, void_type_node,
                               build1(ADDR_EXPR, ptr_type_node, dcl)),
                        build1(NOP_EXPR, void_type_node,
                               build1(ADDR_EXPR, ptr_type_node, tmpvar)),
                        DECL_SIZE_UNIT(dcl)),
        &body);
    append_to_statement_list(
        build_call_expr(VAR_NAME_AS_TREE("printf"), 2,
                        BUILD_STRING_AS_TREE("ctor initstruct %s\n"),
                        BUILD_STRING_AS_TREE(IDENTIFIER_NAME(dcl))),
        &body);
    debug_tree(DECL_CONTEXT(tmpvar));
    /* cgraph_build_static_cdtor('I', body, 0); */
    /* create_hidden_ctor(tmpvar, body); */
    DEBUGF("uploaded ctor\n");
  }
}

void handle_decl(void *gcc_data, void *user_data) {
  tree t = (tree)gcc_data;
  SubContext *ctx = (SubContext *)user_data;
  if (DECL_INITIAL(t) != NULL && DECL_CONTEXT(t) == NULL_TREE &&
      ctx->mods->count > 0 &&
      strncmp(IDENTIFIER_NAME(t), "__tmp_ifs_", strlen("__tmp_ifs_"))) {
    auto rng = EXPR_LOCATION_RANGE(t);
    rng.m_finish = DECL_SOURCE_LOCATION(t);
    rng.m_start = input_location;
    DEBUGF("handle_decl with %s %u,%u - %u-%u\n", IDENTIFIER_NAME(t),
           LOCATION_LINE(rng.m_start), LOCATION_COLUMN(rng.m_start),
           LOCATION_LINE(rng.m_finish), LOCATION_COLUMN(rng.m_finish));
    rewrite_declarations_into_ctor(t, ctx);
    ctx->initcount += ctx->mods->count;
    int errcount = 0;
    /* now at this stage, all uses of our macros have been
     * fixed, INCLUDING case labels. Let's confirm that: */
    for (auto it = ctx->mods->head; it; it = it->next) {
      error_at(it->loc, "unable to substitute constant\n");
      errcount += 1;
    }
    if (errcount != 0) {
      clear_subu_list(ctx->mods);
    }
  }
}
