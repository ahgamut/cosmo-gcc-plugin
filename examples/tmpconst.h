extern void something_0();
extern void something_1();
extern void something_2();
extern void something_3();

#ifdef USING_PLUGIN
#ifndef ACTUALLY
#define ACTUALLY(X) X
#endif

static const int __tmpcosmo_TWO = -420;
static const int __tmpcosmo_THREE = -422;

extern const int TWO;
extern const int THREE;
#define TWO   ACTUALLY(TWO)
#define THREE ACTUALLY(THREE)

#else

#define TWO 17
#define THREE 11

#endif
