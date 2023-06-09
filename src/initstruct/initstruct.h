#ifndef INITSTRUCT_H
#define INITSTRUCT_H
#include <utils.h>
/* gcc utils first */
#include <subcontext.h>

void build_modded_declaration(tree *, SubContext *, location_t);
int build_modded_int_declaration(tree *, SubContext *, subu_node *);
tree copy_struct_ctor(tree);
void modify_local_struct_ctor(tree, subu_list *, location_t);

void set_values_based_on_ctor(tree, subu_list *, tree, tree, location_t);
void handle_decl(void *, void *);
#endif /* INITSTRUCT_H */
