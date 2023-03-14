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

extern subu_list recorder;

void check_macro_define(cpp_reader *reader, location_t loc,
                        cpp_hashnode *node) {
  const char *defn = (const char *)cpp_macro_definition(reader, node);
  if (strstr(defn, " LITERALLY")) {
    DEBUGF("nty=%d, at %u,%u defining macro.. %s\n", node->type,
           LOCATION_LINE(loc), LOCATION_COLUMN(loc), defn);
  }
}

void check_macro_lazy(cpp_reader *reader, cpp_macro *m, unsigned int K) {
  DEBUGF("lazy macro.. %p\n", m);
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
  unsigned space_at = 0;
  if (strstr(defn, " LITERALLY(")) {
    DEBUGF("nty = %d, at %u,%u checking macro.. %s\n", node->type,
           LOCATION_LINE(loc), LOCATION_COLUMN(loc), defn);
    /* I can subtract pointers because I did the strstr;
     * I just want the name of macro (ie the 'constant'
     * that is being LITERALLY used) */
    space_at = strchr(defn, ' ') - defn;
    if ((in_statement & IN_SWITCH_STMT)) {
      add_subu_elem(&recorder, build_subu(loc, defn, space_at, 1));
    } else {
      add_subu_elem(&recorder, build_subu(loc, defn, space_at, 0));
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
  if (cbs) {
    if (cbs->used == NULL) {
      cbs->used = check_macro_use;
    }
    if (cbs->macro_to_expand == NULL) {
      cbs->macro_to_expand = check_macro_expand;
    }
  }
}

void handle_pragma_setup(void *gcc_data, void *user_data) {
  c_register_pragma_with_data("ifswitch", "rearrange",
                              handle_ifswitch_rearrange, user_data);
  cpp_callbacks *cbs = cpp_get_callbacks(parse_in);

  DEBUGF("cbs is %p, NULL? %d\n", cbs, cbs == NULL);
  DEBUGF("cbs->define is %p, NULL? %d\n", cbs->define, cbs->define == NULL);
  if (cbs && cbs->define == NULL) {
    cbs->define = check_macro_define;
  }
  DEBUGF("registered rearrange\n");
}

void handle_end_parsef(void *gcc_data, void *user_data) {
  tree t = (tree)gcc_data;
  if (!t || t == error_mark_node) return;
  if (DECL_INITIAL(t) != NULL && TREE_STATIC(t)) {
    DEBUGF("handle_end_parsef calling %s\n", IDENTIFIER_NAME(t));
  }
  reset_cosmo_macros();
}

