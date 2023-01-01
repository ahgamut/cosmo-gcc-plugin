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
    printf("at %u defining macro.. %s\n", loc, defn);
  }
}

void check_macro_use(cpp_reader *reader, location_t loc, cpp_hashnode *node) {
  const char *defn = (const char *)cpp_macro_definition(reader, node);

  printf("at %u checking macro.. %s\n", loc, defn);
  if (strstr(defn, "LITERALLY(X)") && (in_statement & IN_SWITCH_STMT)) {
    printf("we're inside a switch\n");
  }
}

void override_cosmo_macros() {
  cpp_undef(parse_in, "LITERALLY");
  cpp_undef(parse_in, "SYMBOLIC");
  cpp_define_formatted(parse_in, "LITERALLY(X) = __tmp_ifs_ ## X");
  cpp_define_formatted(parse_in, "SYMBOLIC(X) = __tmp_ifs_ ## X");
}

void reset_cosmo_macros() {
  cpp_undef(parse_in, "LITERALLY");
  cpp_undef(parse_in, "SYMBOLIC");
  cpp_define_formatted(parse_in, "LITERALLY(X) = X");
  cpp_define_formatted(parse_in, "SYMBOLIC(X) = X");
}

void handle_ifswitch_rearrange(cpp_reader *reader, void *data) {
  location_t hello = 0;
  cpp_callbacks *cbs = cpp_get_callbacks(reader);
  tree stpls = NULL;
  auto t = pragma_lex(&stpls);

  if (t != CPP_EOF) {
    printf("glitch wtf\n");
    return;
  }
  printf("cleaned out the pragma eof\n");
  override_cosmo_macros();

  printf("data is %p, NULL? %d\n", data, data == NULL);
  printf("cbs is %p, NULL? %d\n", cbs, cbs == NULL);
  printf("cbs->used is %p, NULL? %d\n", cbs->used, cbs->used == NULL);
  if (cbs->used == NULL) {
    cbs->used = check_macro_use;
  }
}

void setup_ifswitch_pragma(void *gcc_data, void *user_data) {
  c_register_pragma_with_data("ifswitch", "rearrange",
                              handle_ifswitch_rearrange, NULL);
  cpp_callbacks *cbs = cpp_get_callbacks(parse_in);

  printf("cbs is %p, NULL? %d\n", cbs, cbs == NULL);
  printf("cbs->define is %p, NULL? %d\n", cbs->define, cbs->define == NULL);
  if (cbs && cbs->define == NULL) {
    cbs->define = check_macro_define;
  }
  fprintf(stderr, "registered rearrange\n");
}

void start_decl(void *gcc_data, void *user_data) {
  tree t = (tree)gcc_data;
  tree t2;
  struct c_language_function *l;

  if (TREE_CODE(t) == FUNCTION_DECL) {
    /* this function is defined within the file I'm processing */
    printf("\nstart_decl calling %s\n", IDENTIFIER_NAME(t));
    if (!strcmp("exam_func", IDENTIFIER_NAME(t))) {
      t2 = DECL_SAVED_TREE(t);
      // process_stmt(&t2);
      debug_tree(t2);
      auto &stv = cur_stmt_list;
    }
  }
}

void handle_pre_genericize(void *gcc_data, void *user_data) {
  tree t = (tree)gcc_data;
  tree t2;
  printf("pre-genericize calling %s\n", IDENTIFIER_NAME(t));
  printf("error count is %d\n", errorcount);
  if (TREE_CODE(t) == FUNCTION_DECL && DECL_INITIAL(t) != NULL &&
      TREE_STATIC(t)) {
    /* this function is defined within the file I'm processing */
    t2 = DECL_SAVED_TREE(t);
    // process_stmt(&t2);
    debug_tree(DECL_SAVED_TREE(t));
  }
}

void end_decl(void *gcc_data, void *user_data) {
  tree t = (tree)gcc_data;
  if (!t || t == error_mark_node) return;
  printf("end_decl calling %s\n", IDENTIFIER_NAME(t));
  reset_cosmo_macros();
  // tree t2;
}

int plugin_init(struct plugin_name_args *plugin_info,
                struct plugin_gcc_version *version) {
  if (!plugin_default_version_check(version, &gcc_version)) {
    fprintf(stderr, "GCC version incompatible!\n");
    return 1;
  }
  printf("Loading plugin %s on GCC %s...\n", plugin_info->base_name,
         version->basever);
  register_callback(plugin_info->base_name, PLUGIN_INFO, NULL, &ifswitch_info);
  /* register_callback(plugin_info->base_name, PLUGIN_START_PARSE_FUNCTION,
                    start_decl, NULL); */
  register_callback(plugin_info->base_name, PLUGIN_PRAGMAS,
                    setup_ifswitch_pragma, NULL);
  register_callback(plugin_info->base_name, PLUGIN_PRE_GENERICIZE,
                    handle_pre_genericize, NULL);
  register_callback(plugin_info->base_name, PLUGIN_FINISH_PARSE_FUNCTION,
                    end_decl, NULL);
  return 0;
}
