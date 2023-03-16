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

/* DON'T FREE THIS */
subu_list recorder = {
    .head = NULL,
    .start = 0,
    .end = 0,
    .count = 0,
};
int mod_active;

void check_macro_use(cpp_reader *reader, location_t loc, cpp_hashnode *node) {
  const char *defn = (const char *)cpp_macro_definition(reader, node);
  unsigned space_at = 0;
  if (strstr(defn, " LITERALLY(") || strstr(defn, "SYMBOLIC(")) {
    DEBUGF("flags = %03o, at %u,%u checking macro.. %s\n", node->flags,
           LOCATION_LINE(loc), LOCATION_COLUMN(loc), defn);
    /* I can subtract pointers because I did the strstr;
     * I just want the name of macro (ie the 'constant'
     * that is being LITERALLY used) */
    space_at = strchr(defn, ' ') - defn;
    /* I don't have enough info to say whether
     * this is a case label or just a normal statement
     * inside a switch, or something outside a switch */
    add_subu_elem(&recorder, build_subu(loc, defn, space_at, UNKNOWN));
  }
}

void check_macro_define(cpp_reader *reader, location_t loc,
                        cpp_hashnode *node) {
  const char *defn = (const char *)cpp_macro_definition(reader, node);
  if (strstr(defn, " LITERALLY") || strstr(defn, " SYMBOLIC(")) {
    DEBUGF("flags:%x, at %u,%u defining macro.. %s\n", node->flags,
           LOCATION_LINE(loc), LOCATION_COLUMN(loc), defn);
    if (mod_active == 0) {
      DEBUGF("activating macro logging...\n");
      mod_active = 1;
      cpp_get_callbacks(reader)->used = check_macro_use;
    }
  }
}

void activate_macro_check(cpp_reader *reader = NULL) {
  if (!reader) {
    reader = parse_in; /* TODO: figure out a better way */
  }
  cpp_callbacks *cbs = cpp_get_callbacks(reader);

  DEBUGF("cbs is %p, NULL? %d\n", cbs, cbs == NULL);
  DEBUGF("cbs->define is %p, NULL? %d\n", cbs->define, cbs->define == NULL);
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
    mod_active = 0;
    cbs->used = NULL;
  }
}

void handle_start_tu(void *gcc_data, void *user_data) {
  DEBUGF("handle_start_tu\n");
  activate_macro_check();
}

void handle_finish_tu(void *gcc_data, void *user_data) {
  /* here we would check if all our transformations
   * actually happened or not, because if someone tried
   * some wacky things within a switch there is a chance
   * we might need to error out */
  DEBUGF("Attempting cleanup...\n");
  subu_list *list = (subu_list *)user_data;
  if (list != NULL && list == &recorder) {
    /* check count here */
    if (list->count != 0) {
      for (auto it = list->head; it; it = it->next) {
        error_at(it->loc, "error with plugin, could not substitute constant\n");
      }
    }
    clear_subu_list(list);
  } else {
    error_at(MAX_LOCATION_T, "fatal error with plugin, could not clear data\n");
  }
  deactivate_macro_check();
}
