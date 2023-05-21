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

tree access_at(tree obj, tree ind) {
  if (TREE_CODE(TREE_TYPE(obj)) == ARRAY_TYPE) {
    return build_array_ref(input_location, obj, ind);
  }
  return build_component_ref(input_location, obj,
                             get_identifier(IDENTIFIER_NAME(ind)),
                             DECL_SOURCE_LOCATION(ind));
}

void set_values_based_on_ctor(tree ctor, subu_list *list, tree body, tree lhs,
                              location_t bound) {
  subu_node *use = NULL;
  unsigned int iprev = 0;
  bool started = true;
  while (list->count > 0 && LOCATION_BEFORE2(list->start, bound)) {
    tree index = NULL_TREE;
    tree val = NULL_TREE;
    unsigned int i = 0;
    bool found = false;
    FOR_EACH_CONSTRUCTOR_ELT(CONSTRUCTOR_ELTS(ctor), i, index, val) {
      DEBUGF("value %u is %s\n", i, get_tree_code_str(val));
      if (!started && i <= iprev) continue;
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
        auto sub = access_at(lhs, index);
        // debug_tree(sub);
        set_values_based_on_ctor(val, list, body, sub, bound);
        use = NULL; /* might've gotten stomped */
        if (list->count == 0) return;
        get_subu_elem(list, list->start, &use);
      }
    }
    if (found) {
      auto modexpr = build2(MODIFY_EXPR, TREE_TYPE(index),
                            access_at(lhs, index), VAR_NAME_AS_TREE(use->name));
      // debug_tree(modexpr);
      append_to_statement_list(modexpr, &body);
      remove_subu_elem(list, use);
      DEBUGF("found; %d left\n", list->count);
    } else {
      /* we did not find any (more) substitutions to fix */
      DEBUGF("exiting; %d left\n", list->count);
      break;
    }
  }
}
