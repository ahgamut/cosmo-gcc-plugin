#ifndef IFSWITCH_H
#define IFSWITCH_H
#include "headers.h"
#include "sub_usage.h"

#define IFSWITCH         "ifswitch"
#define IFSWITCH_VERSION "0.0.1"
#define IFSWITCH_HELP    "convert switch statements to if statements"

/* in process.cpp */
// void process_body(tree);
void process_stmt(tree *, subu_list *);
// void process_stmt_list(tree);
// bool is_switch_moddable(tree);

/* in utils.cpp */
const char *get_tree_code_str(tree);

void handle_end_parsef(void *, void *);
void handle_pragma_setup(void *, void *);
void handle_finish_cleanup(void *, void *);
void handle_pre_genericize(void *, void *);
void handle_start_parsef(void *, void *);

#endif /* IFSWITCH_H */
