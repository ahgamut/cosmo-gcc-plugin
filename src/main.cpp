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

int plugin_is_GPL_compatible; /* ISC */

static struct plugin_name_args ifswitch_info = {
    .base_name = IFSWITCH, .version = IFSWITCH_VERSION, .help = IFSWITCH_HELP};

void check_macro_define(cpp_reader *reader, location_t loc,
                        cpp_hashnode *node) {
  const char *defn = (const char *)cpp_macro_definition(reader, node);
  if (strstr(defn, " LITERALLY")) {
    DEBUGF("at %u defining macro.. %s\n", loc, defn);
  }
}

void override_cosmo_macros(cpp_reader *reader = NULL) {
  if (!reader) {
    reader = parse_in;
  }
  cpp_undef(reader, "LITERALLY");
  cpp_undef(reader, "SYMBOLIC");
  cpp_define_formatted(reader, "LITERALLY(X) = __tmp_ifs_ ## X");
  cpp_define_formatted(reader, "SYMBOLIC(X) = __tmp_ifs_ ## X");
}

void reset_cosmo_macros(cpp_reader *reader = NULL) {
  if (!reader) {
    reader = parse_in;
  }
  cpp_undef(reader, "LITERALLY");
  cpp_undef(reader, "SYMBOLIC");
  cpp_define_formatted(reader, "LITERALLY(X) = X");
  cpp_define_formatted(reader, "SYMBOLIC(X) = X");
}

void check_macro_use(cpp_reader *reader, location_t loc, cpp_hashnode *node) {
  const char *defn = (const char *)cpp_macro_definition(reader, node);
  const char *prev = NULL;
  const char *next = NULL;
  location_t caseloc;
  if (strstr(defn, " LITERALLY(")) {
    DEBUGF("at %u,%u checking macro.. %s\n", LOCATION_LINE(loc),
           LOCATION_COLUMN(loc), defn);
    if ((in_statement & IN_SWITCH_STMT)) {
      /* we're currently at LITERALLY(X), the expansion of X */
      /* if this is indeed a ifswitch case, then the previous token would be X,
       * and the token before that would be "case", so let's check for that */
      _cpp_backup_tokens(reader, 2);
      prev = (const char *)cpp_token_as_text(reader, cpp_peek_token(reader, 0));
      cpp_get_token_with_location(reader, &caseloc);
      cpp_get_token(reader);
      next = (const char *)cpp_token_as_text(reader, cpp_peek_token(reader, 0));

      if (!strcmp(prev, "case") && !strcmp(next, ":")) {
        DEBUGF("this is a case statement at %u,%u\n", LOCATION_LINE(caseloc),
               LOCATION_COLUMN(caseloc));
      } else if (strcmp(prev, "case") && strcmp(next, ":")) {
        DEBUGF("this is NOT a case statement at %u,%u\n",
               LOCATION_LINE(caseloc), LOCATION_COLUMN(caseloc));
      } else {
        /* inside a switch, note that we should not allow things like:
         *
         * case MYVAR + 27:
         * case MYVAR ... OTHERVAR:
         * case MACRO(MYVAR):
         */
        error_at(loc, "invalid use of #pragma ifswitch\n");
        return;
      }
      DEBUGF("we're inside a switch, previous was '%s', next was '%s'\n", prev,
             next);
    }
  }
}

cpp_hashnode *check_macro_expand(cpp_reader *reader, const cpp_token *tok) {
  const unsigned char *prev = cpp_token_as_text(reader, tok);
  DEBUGF("looking to expand %s\n\n\n", (char *)prev);
  return cpp_lookup(reader, prev, strlen((char *)prev));
}

void handle_ifswitch_rearrange(cpp_reader *reader, void *data) {
  location_t hello = 0;
  cpp_callbacks *cbs = cpp_get_callbacks(reader);
  tree stpls = NULL;
  auto t = pragma_lex(&stpls);

  if (t != CPP_EOF) {
    DEBUGF("glitch wtf\n");
    return;
  }
  DEBUGF("cleaned out the pragma eof\n");
  override_cosmo_macros();

  DEBUGF("data is %p, NULL? %d\n", data, data == NULL);
  DEBUGF("cbs is %p, NULL? %d\n", cbs, cbs == NULL);
  DEBUGF("cbs->used is %p, NULL? %d\n", cbs->used, cbs->used == NULL);
  if (cbs && cbs->used == NULL) {
    cbs->used = check_macro_use;
  }
  if (cbs && cbs->macro_to_expand == NULL) {
    DEBUGF("registered macro_to_expand\n");
    cbs->macro_to_expand = check_macro_expand;
  }
}

void handle_pragma_setup(void *gcc_data, void *user_data) {
  c_register_pragma_with_data("ifswitch", "rearrange",
                              handle_ifswitch_rearrange, NULL);
  cpp_callbacks *cbs = cpp_get_callbacks(parse_in);

  DEBUGF("cbs is %p, NULL? %d\n", cbs, cbs == NULL);
  DEBUGF("cbs->define is %p, NULL? %d\n", cbs->define, cbs->define == NULL);
  if (cbs && cbs->define == NULL) {
    cbs->define = check_macro_define;
  }
  DEBUGF("registered rearrange\n");
}

void handle_start_parsef(void *gcc_data, void *user_data) {
  tree t = (tree)gcc_data;
  tree t2;
  struct c_language_function *l;

  if (TREE_CODE(t) == FUNCTION_DECL) {
    /* this function is defined within the file I'm processing */
    DEBUGF("\nhandle_start_parsef calling %s\n", IDENTIFIER_NAME(t));
    if (!strcmp("exam_func", IDENTIFIER_NAME(t))) {
      t2 = DECL_SAVED_TREE(t);
      // process_stmt(&t2);
      // debug_tree(t2);
      auto &stv = cur_stmt_list;
    }
  }
}

void handle_pre_genericize(void *gcc_data, void *user_data) {
  tree t = (tree)gcc_data;
  tree t2;
  // DEBUGF("error count is %d\n", errorcount);
  if (TREE_CODE(t) == FUNCTION_DECL && DECL_INITIAL(t) != NULL &&
      TREE_STATIC(t)) {
    /* this function is defined within the file I'm processing */
    DEBUGF("pre-genericize calling %s\n", IDENTIFIER_NAME(t));
    t2 = DECL_SAVED_TREE(t);
    // process_stmt(&t2);
    // debug_tree(DECL_SAVED_TREE(t));
  }
}

void handle_end_parsef(void *gcc_data, void *user_data) {
  tree t = (tree)gcc_data;
  if (!t || t == error_mark_node) return;
  if (DECL_INITIAL(t) != NULL && TREE_STATIC(t)) {
    DEBUGF("handle_end_parsef calling %s\n", IDENTIFIER_NAME(t));
  }
  reset_cosmo_macros();
}

void handle_finish_cleanup(void *gcc_data, void *user_data) {
  /* here we would check if all our transformations
   * actually happened or not, because if someone tried
   * some wacky things within a switch there is a chance
   * we might need to error out */
  DEBUGF("Attempting cleanup...\n");
  return;
}

int plugin_init(struct plugin_name_args *plugin_info,
                struct plugin_gcc_version *version) {
  if (!plugin_default_version_check(version, &gcc_version)) {
    DEBUGF("GCC version incompatible!\n");
    return 1;
  }
  DEBUGF("Loading plugin %s on GCC %s...\n", plugin_info->base_name,
         version->basever);
  register_callback(plugin_info->base_name, PLUGIN_INFO, NULL, &ifswitch_info);
  /* register_callback(plugin_info->base_name, PLUGIN_START_PARSE_FUNCTION,
                    handle_start_parsef, NULL); */
  register_callback(plugin_info->base_name, PLUGIN_PRAGMAS, handle_pragma_setup,
                    NULL);
  register_callback(plugin_info->base_name, PLUGIN_PRE_GENERICIZE,
                    handle_pre_genericize, NULL);
  register_callback(plugin_info->base_name, PLUGIN_FINISH_PARSE_FUNCTION,
                    handle_end_parsef, NULL);
  register_callback(plugin_info->base_name, PLUGIN_FINISH,
                    handle_finish_cleanup, NULL);
  return 0;
}
