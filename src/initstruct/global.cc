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
#include <initstruct/initstruct.h>

void update_global_decls(tree dcl, SubContext *ctx) {
  tree body = alloc_stmt_list();
  subu_node *use = NULL;
  char chk[STRING_BUFFER_SIZE];

  /* dcl, the global declaration we have is like these:
   *
   * static int foo = __tmpcosmo_VAR;
   * static struct toy myvalue = {.x=1, .y=__tmpcosmo_VAR};
   *
   * we're going to add functions as follows:
   *
   * static int foo = __tmpcosmo_VAR;
   * __attribute__((constructor)) __hidden_ctor1() {
   *   foo = VAR;
   * }
   * static struct toy myvalue = {.x=1, .y=__tmpcosmo_VAR};
   * __attribute__((constructor)) __hidden_ctor2() {
   *    myvalue.y = VAR;
   * }
   *
   * the modifier functions have the constructor attribute,
   * so it they run before anything uses the static. it
   * works recursively too: you can have a struct of structs,
   * an array of structs, whatever, and it will figure out
   * where the substitutions are and attempt to mod them.
   *
   * a unique constructor for each declaration. probably
   * we could have a common constructor for the entire
   * file, but that's left as an exercise for the reader. */
  if (INTEGRAL_TYPE_P(TREE_TYPE(dcl)) &&
      get_subu_elem(ctx->mods, ctx->mods->start, &use) &&
      /* use is non-NULL if get_subu_elem succeeds */
      check_magic_equal(DECL_INITIAL(dcl), use->name)) {
    if (TREE_READONLY(dcl)) {
      error_at(EXPR_LOCATION(dcl), "cannot substitute this constant\n");
      /* actually I can, but the issue is if one of gcc's optimizations
       * perform constant folding(and they do), I don't know all the spots
       * where this variable has been folded, so I can't substitute there */
      ctx->active = 0;
      return;
    }
    append_to_statement_list(
        build2(MODIFY_EXPR, void_type_node, dcl, VAR_NAME_AS_TREE(use->name)),
        &body);
    /*
    append_to_statement_list(
        build_call_expr(VAR_NAME_AS_TREE("printf"), 2,
                        BUILD_STRING_AS_TREE("ctor initstruct %s\n"),
                        BUILD_STRING_AS_TREE(IDENTIFIER_NAME(dcl))),
        &body);
        */
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
    if (LOCATION_BEFORE2(ctx->mods->end, input_location)) {
      set_values_based_on_ctor(DECL_INITIAL(dcl), ctx->mods, body, dcl,
                               input_location);
    } else {
      set_values_based_on_ctor(DECL_INITIAL(dcl), ctx->mods, body, dcl,
                               ctx->mods->end);
    }
    /*
    append_to_statement_list(
        build_call_expr(VAR_NAME_AS_TREE("printf"), 2,
                        BUILD_STRING_AS_TREE("ctor initstruct %s\n"),
                        BUILD_STRING_AS_TREE(IDENTIFIER_NAME(dcl))),
        &body);
        */
    cgraph_build_static_cdtor('I', body, 0);
    DEBUGF("uploaded ctor\n");
  }
}

void handle_decl(void *gcc_data, void *user_data) {
  tree t = (tree)gcc_data;
  SubContext *ctx = (SubContext *)user_data;
  if (ctx->active && ctx->mods->count > 0 && DECL_INITIAL(t) != NULL &&
      DECL_CONTEXT(t) == NULL_TREE) {
    int internal_use =
        !strncmp(IDENTIFIER_NAME(t), "__tmpcosmo_", strlen("__tmpcosmo_"));
    if (internal_use || DECL_EXTERNAL(t)) {
      error_at(input_location, "the ACTUALLY is before the declaration!\n");
      ctx->active = 0;
      return;
    }
    auto rng = EXPR_LOCATION_RANGE(t);
    rng.m_start = DECL_SOURCE_LOCATION(t);
    rng.m_finish = input_location;

    DEBUGF("handle_decl with %s %u,%u - %u-%u\n", IDENTIFIER_NAME(t),
           LOCATION_LINE(rng.m_start), LOCATION_COLUMN(rng.m_start),
           LOCATION_LINE(rng.m_finish), LOCATION_COLUMN(rng.m_finish));
    ctx->initcount += ctx->mods->count;
    update_global_decls(t, ctx);
    /* now at this stage, all uses of our macros have been
     * fixed, INCLUDING case labels. Let's confirm that: */
    check_context_clear(ctx, MAX_LOCATION_T);
  }
}
