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

/* DON'T FREE THE PARTS OF THIS */
extern SubContext plugin_context;

void check_macro_use(cpp_reader *reader, location_t loc, cpp_hashnode *node) {
  const char *defn = (const char *)cpp_macro_definition(reader, node);
  /* the definitions I am looking for are EXACTLY of the form
   *
   * #define X SYMBOLIC(X)
   *          ^^^^^^^^^^    ----> (test for this)
   * So the left-hand side of the macro (ie before the space)
   * should be the same as the argument inside the macro
   * (ie within the parentheses), otherwise this substitution
   * is most likely not worth recording, or an error. */
  if (plugin_context.active == 1 && strstr(defn, " SYMBOLIC(")) {
    DEBUGF("at %u,%u checking macro.. %s\n", LOCATION_LINE(loc),
           LOCATION_COLUMN(loc), defn);
    const char *arg_start = strstr(defn, "(");
    const char *arg_end = strstr(defn, ")");
    if (!arg_start || !arg_end) return;
    arg_start += 1; /* move from '(' to start of arg */
    if (strncmp(defn, arg_start, arg_end - arg_start) == 0) {
      /* This is most likely a substitution we need to
       * record, but I don't have enough info to say whether
       * this is a case label or just a normal statement
       * inside a switch, or something outside a switch */
      add_context_subu(&plugin_context, loc, defn, (arg_end - arg_start),
                       UNKNOWN);
    } else {
      cpp_error_at(reader, CPP_DL_ERROR, loc, "unable to check macro usage\n");
      plugin_context.active = 0;
    }
  }
}

void check_macro_define(cpp_reader *reader, location_t loc,
                        cpp_hashnode *node) {
  const char *defn = (const char *)cpp_macro_definition(reader, node);
  if (plugin_context.active == 0 && strstr(defn, " SYMBOLIC(")) {
    plugin_context.active = 1;
    cpp_get_callbacks(reader)->used = check_macro_use;
    inform(loc, "recording usage of SYMBOLIC() macro...\n");
  }
  /* TODO: at this point in execution, is it possible to
   *
   * #define __tmp_ifs_X <number we control>
   *
   * and use this instead of defining __tmp constants in 
   * tmpconst.h? update a hash-table here, to use later in
   * lookups and substitution checks during the parsing. */
}

void activate_macro_check(cpp_reader *reader = NULL) {
  if (!reader) {
    reader = parse_in; /* TODO: figure out a better way */
  }
  cpp_callbacks *cbs = cpp_get_callbacks(reader);
  cpp_options *opts = cpp_get_options(reader);
  if (opts->lang == CLK_ASM) {
    inform(UNKNOWN_LOCATION, "plugin does not activate for ASM\n");
    return;
  }
  cpp_undef(reader, "SYMBOLIC");
  cpp_define_formatted(reader, "SYMBOLIC(X) = __tmp_ifs_ ## X");
  if (cbs && cbs->define == NULL) {
    cbs->define = check_macro_define;
  }
}

void deactivate_macro_check(cpp_reader *reader = NULL) {
  if (!reader) {
    reader = parse_in; /* TODO: figure out a better way */
  }
  cpp_callbacks *cbs = cpp_get_callbacks(reader);
  if (cbs && cbs->define == check_macro_define) {
    cbs->define = NULL;
  }
  if (cbs && cbs->used == check_macro_use) {
    cbs->used = NULL;
  }
  plugin_context.active = 0;
  cpp_undef(reader, "SYMBOLIC");
  cpp_define_formatted(reader, "SYMBOLIC(X) = X");
}

void handle_start_tu(void *gcc_data, void *user_data) {
  activate_macro_check();
}

void handle_finish_tu(void *gcc_data, void *user_data) {
  DEBUGF("Attempting cleanup...\n");
  SubContext *ctx = (SubContext *)user_data;
  if (ctx != &plugin_context) {
    fatal_error(MAX_LOCATION_T, "unable to clear plugin data!");
  } else {
    check_context_clear(ctx, MAX_LOCATION_T);
    ctx->prev = NULL;
  }
  deactivate_macro_check();
}
