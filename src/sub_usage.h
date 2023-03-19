#ifndef SUB_USAGE_H
#define SUB_USAGE_H
#include "headers.h"

enum SubstType { UNKNOWN = 0, SW_CASE = 1, INITVAL = 2 };

struct _subu_node {
  /* a node indicating that an ifswitch substitution has occurred.
   *
   * Details include:
   *
   * - location_t of the substitution
   * - char* of name of the macro that was substituted (alloc'd)
   * - whether the substitution was inside a switch statement
   * - _subu_node* pointer to the next element in the list (NULL if last)
   *
   * the idea is that every time one of our modified macros is used,
   * we record the substitution, and then we delete this record if
   * we find the appropriate location_t during pre-genericize and
   * construct the necessary parse trees at that point.
   *
   * at the end of compilation (ie PLUGIN_FINISH), there should be
   * no subu_nodes allocated.
   */
  location_t loc;
  SubstType tp;
  char *name;
  struct _subu_node *next;
};

typedef struct _subu_node subu_node;

subu_node *build_subu(const location_t, const char *, unsigned int, SubstType);
void delete_subu(subu_node *);

struct _subu_list {
  subu_node *head;
  /* inclusive bounds, range containing all recorded substitutions */
  location_t start, end;
  /* number of substitutions */
  int count;
};
typedef struct _subu_list subu_list;

subu_list *init_subu_list();

void add_subu_elem(subu_list *, subu_node *);
int check_loc_in_bound(subu_list *, location_t);
int valid_subu_bounds(subu_list *, location_t, location_t);
int get_subu_elem(subu_list *, location_t, subu_node **);
int get_subu_elem2(subu_list *, source_range, subu_node **);
void remove_subu_elem(subu_list *, subu_node *);
void pop_subu_list(subu_list *);
void clear_subu_list(subu_list *);
void delete_subu_list(subu_list *);
void check_empty_subu_list(subu_list *);

struct SubContext {
  /* record all macro uses */
  subu_list *mods;
  /* address of the previous statement we walked through,
   * in case we missed modding it and have to retry */
  tree *prev;
  /* count number of switch statements rewritten */
  unsigned int switchcount;
  /* count number of initializations rewritten */
  unsigned int initcount;
  /* count number of other substitutions rewritten */
  unsigned int subcount;
};


#endif /* SUB_USAGE_H */
