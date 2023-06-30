extern void something_0();
extern void something_1();
extern void something_2();
extern void something_3();

#ifdef USING_PLUGIN
#ifndef ACTUALLY
#define ACTUALLY(X) X
#endif

extern const int TWO;
extern const int THREE;
#define TWO   TWO
#define THREE THREE

#define __tmpcosmo_TWO -420
#define __tmpcosmo_THREE -422

#else

#define TWO 17
#define THREE 11

#endif
