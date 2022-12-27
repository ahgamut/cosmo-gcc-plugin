#ifndef IFSWITCH_H
#define IFSWITCH_H

/* first stdlib headers */
#include <stdio.h>

/* now all the plugin headers */
#include <gcc-plugin.h>
/* first gcc-plugin, then the others */
#include <plugin-version.h>
#include <print-tree.h>
#include <tree.h>
#include <tree-iterator.h>
#include <c-family/c-common.h>
#include <stringpool.h>

#define IFSWITCH         "ifswitch"
#define IFSWITCH_VERSION "0.0.1"
#define IFSWITCH_HELP    "convert switch statements to if statements"

const char* get_tree_code_str(tree);

#endif /* IFSWITCH_H */
