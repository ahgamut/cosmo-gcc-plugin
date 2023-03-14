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

/* DON'T FREE THIS */
subu_list recorder = {
    .head = NULL,
    .start = 0,
    .end = 0,
    .count = 0,
};

void handle_finish_cleanup(void *gcc_data, void *user_data) {
  /* here we would check if all our transformations
   * actually happened or not, because if someone tried
   * some wacky things within a switch there is a chance
   * we might need to error out */
  DEBUGF("Attempting cleanup...\n");
  subu_list *list = (subu_list *)user_data;
  if (list != NULL && list == &recorder) {
    /* check count here */
    clear_subu_list(list);
  } else {
    error_at(MAX_LOCATION_T, "fatal error with plugin, could not clear data\n");
  }
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
                    &recorder);
  register_callback(plugin_info->base_name, PLUGIN_PRE_GENERICIZE,
                    handle_pre_genericize, &recorder);
  register_callback(plugin_info->base_name, PLUGIN_FINISH_PARSE_FUNCTION,
                    handle_end_parsef, &recorder);
  register_callback(plugin_info->base_name, PLUGIN_FINISH,
                    handle_finish_cleanup, &recorder);
  return 0;
}
