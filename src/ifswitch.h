#ifndef IFSWITCH_H
#define IFSWITCH_H

/* first stdlib headers */
#include <stdio.h>

/* now all the plugin headers */
#include <gcc-plugin.h>
/* first gcc-plugin, then the others */
#include <c-family/c-common.h>
#include <c-family/c-pragma.h>
#include <c-tree.h>
#include <cpplib.h>
#include <diagnostic.h>
#include <plugin-version.h>
#include <print-tree.h>
#include <stringpool.h>
#include <tree-iterator.h>
#include <tree.h>

#define IFSWITCH         "ifswitch"
#define IFSWITCH_VERSION "0.0.1"
#define IFSWITCH_HELP    "convert switch statements to if statements"

/* in process.cpp */
// void process_body(tree);
void process_stmt(tree*);
// void process_stmt_list(tree);
// bool is_switch_moddable(tree);

/* in utils.cpp */
const char* get_tree_code_str(tree);

/* useful macros */
#define IDENTIFIER_NAME(z) IDENTIFIER_POINTER(DECL_NAME((z)))
#define EXPR_LOC_LINE(x)   LOCATION_LINE(EXPR_LOCATION((x)))
#define EXPR_LOC_COL(x)    LOCATION_COLUMN(EXPR_LOCATION((x)))

#ifndef NDEBUG
#define DEBUGF(...) fprintf(stderr, "<DEBUG> " __VA_ARGS__)
#else
#define DEBUGF(...)
#endif

#endif /* IFSWITCH_H */
