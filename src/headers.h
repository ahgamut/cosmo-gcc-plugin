#ifndef IFSWITCH_HEADERS_H
#define IFSWITCH_HEADERS_H

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

/* useful macros */
#define VAR_NAME_AS_TREE(fname) lookup_name(get_identifier((fname)))
#define IDENTIFIER_NAME(z) IDENTIFIER_POINTER(DECL_NAME((z)))
#define EXPR_LOC_LINE(x)   LOCATION_LINE(EXPR_LOCATION((x)))
#define EXPR_LOC_COL(x)    LOCATION_COLUMN(EXPR_LOCATION((x)))
#define LOCATION_APPROX(x, y)                   \
  (LOCATION_LINE((x)) == LOCATION_LINE((y)))
#define LOCATION_BEFORE(x, y)                   \
  (LOCATION_LINE((x)) <= LOCATION_LINE((y)))
#define LOCATION_AFTER(x, y)                    \
  (LOCATION_LINE((x)) >= LOCATION_LINE((y)))

#ifndef NDEBUG
#define DEBUGF(...) fprintf(stderr, "<DEBUG> " __VA_ARGS__)
#else
#define DEBUGF(...)
#endif

#endif
