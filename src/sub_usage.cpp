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
#include "sub_usage.h"

subu_node *build_subu(const location_t loc, const char *name,
                      unsigned int namelen, int in_switch) {
  /* xmalloc because malloc is poisoned by gcc-plugin's system.h */
  subu_node *res = (subu_node *)xmalloc(sizeof(subu_node));
  res->next = NULL;
  res->loc = loc;
  res->name = xstrndup(name, namelen);
  res->in_switch = in_switch;
  DEBUGF("allocated subu_node at %p\n", res);
  return res;
};

void delete_subu(subu_node *node) {
  node->loc = 0x0;
  node->in_switch = 0;
  free(node->name);
  node->next = NULL;
  DEBUGF("freeing subu_node at %p\n", node);
  free(node);
}

subu_list *init_subu_list() {
  subu_list *res = (subu_list *)xmalloc(sizeof(subu_list)); /* allocation 3 */
  res->head = NULL;
  res->count = 0;
  res->start = 0;
  res->end = 0;
  DEBUGF("allocated subu_node at %p\n", res);
  return res;
}

static void recount_subu_list(subu_list *list) {
  int i = 0;
  location_t s = MAX_LOCATION_T;
  location_t e = 0;
  subu_node *it = list->head;
  for (; it != NULL; it = it->next) {
    i += 1;
    /* is it possible to compare for s and e? */
    if (LOCATION_BEFORE(it->loc, s)) s = it->loc;
    if (LOCATION_AFTER(it->loc, e)) e = it->loc;
  }
  if (s > e) {
    s = e;
  }
  list->start = s;
  list->end = e;
  list->count = i;
  DEBUGF("list with %d subus, start = %u,%u end = %u,%u\n", list->count,
         LOCATION_LINE(list->start), LOCATION_COLUMN(list->start),
         LOCATION_LINE(list->end), LOCATION_COLUMN(list->end));
}

void add_subu_elem(subu_list *list, subu_node *node) {
  subu_node *tmp;
  if (list->head == NULL) {
    list->head = node;
  } else {
    for (tmp = list->head; tmp->next != NULL; tmp = tmp->next)
      ;
    tmp->next = node;
    node->next = NULL;
  }
  recount_subu_list(list);
}

void pop_subu_list(subu_list *list) {
  if (list->head != NULL) {
    subu_node *tmp = list->head;
    list->head = list->head->next;
    delete_subu(tmp);
  }
  recount_subu_list(list);
}

int valid_subu_bounds(subu_list *list, location_t start, location_t end) {
  /* return 1 if the bounds of list and provided bounds overlap */
  if (LOCATION_BEFORE(list->start, end) && LOCATION_AFTER(list->start, start))
    return 1;
  if (LOCATION_BEFORE(start, list->end) && LOCATION_AFTER(start, list->start))
    return 1;
  return 0;
}

int check_subu_elem(subu_list *list, location_t loc) {
  /* return 1 if loc is within the bounds */
  if (LOCATION_BEFORE(list->start, loc) && LOCATION_AFTER(list->end, loc)) {
    return 1;
  } else {
    return 0;
  }
}

int get_subu_elem(subu_list *list, location_t loc, subu_node **node) {
  /* *node is overwritten on returning 1 ie success */
  subu_node *it = list->head;
  for (; it != NULL; it = it->next) {
    if (LOCATION_APPROX(it->loc, loc)) {
      *node = it;
      return 1;
    }
  }
  return 0;
}

void remove_subu_elem(subu_list *list, subu_node *node) {
  subu_node *cur, *prev;
  if (list->head != NULL) {
    if (list->head == node) {
      cur = list->head;
      list->head = list->head->next;
      delete_subu(cur);
    } else {
      prev = list->head;
      cur = list->head->next;
      for (; cur != NULL; prev = cur, cur = cur->next) {
        if (cur == node) {
          prev->next = cur->next;
          delete_subu(cur);
          break;
        }
      }
    }
    recount_subu_list(list);
  }
}

void clear_subu_list(subu_list *list) {
  subu_node *it, *tmp;
  for (it = list->head; it != NULL;) {
    tmp = it;
    it = it->next;
    delete_subu(tmp);
  }
  list->head = NULL;
  list->count = 0;
  list->start = 0;
  list->end = 0;
}

void delete_subu_list(subu_list *list) {
  clear_subu_list(list);
  free(list);
  DEBUGF("freeing subu_list at %p\n", list);
}
