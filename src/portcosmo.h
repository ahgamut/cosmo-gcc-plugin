#ifndef PORTCOSMO_H
#define PORTCOSMO_H
#include <utils.h>
/* gcc utils first */
#include <ifswitch/ifswitch.h>
#include <initstruct/initstruct.h>
#include <subcontext.h>

#define PORTCOSMO         "portcosmo"
#define PORTCOSMO_VERSION "0.0.1"
#define PORTCOSMO_HELP    "help porting C software to Cosmopolitan Libc"

void handle_start_tu(void *, void *);
void handle_finish_tu(void *, void *);
void handle_pre_genericize(void *, void *);

#endif /* PORTCOSMO_H */
