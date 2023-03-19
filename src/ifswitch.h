#ifndef IFSWITCH_H
#define IFSWITCH_H
#include "headers.h"
#include "sub_usage.h"

#define IFSWITCH         "ifswitch"
#define IFSWITCH_VERSION "0.0.1"
#define IFSWITCH_HELP    "convert switch statements to if statements"

/* in ifswitch.cpp */
source_range get_switch_bounds(tree);
unsigned int count_mods_in_switch(tree, subu_list *);
tree build_modded_switch_stmt(tree, SubContext *);

/* in initstruct_local.cpp */
void build_modded_declaration(tree *, SubContext *, location_t);
int build_modded_int_declaration(tree *, subu_node *);
tree copy_struct_ctor(tree);
void modify_local_struct_ctor(tree, subu_list *, location_t);

/* in process.cpp */
void process_body(tree *, SubContext *);

/* in utils.cpp */
const char *get_tree_code_str(tree);
int get_value_of_const(char *);
tree get_ifsw_identifier(char *);
int check_magic_equal(tree, char *);

void handle_start_tu(void *, void *);
void handle_finish_tu(void *, void *);
void handle_pre_genericize(void *, void *);
void handle_decl(void *, void *);

#endif /* IFSWITCH_H */
