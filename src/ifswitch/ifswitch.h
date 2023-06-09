#ifndef IFSWITCH_H
#define IFSWITCH_H
#include <utils.h>
/* gcc utils first */
#include <subcontext.h>

source_range get_switch_bounds(tree);
unsigned int count_mods_in_switch(tree, subu_list *);
tree build_modded_switch_stmt(tree, SubContext *);

#endif /* IFSWITCH_H */
