#include <stdio.h>

extern void something_0();
extern void something_1();
extern void something_2();
extern void something_3();
extern const int abcd;

#ifndef LITERALLY
#define LITERALLY(X) X
#endif

#define HAVE_TWO 1
#if HAVE_TWO
extern const int TWO;
#define TWO LITERALLY(TWO)
#endif

#define LOLMAX(X) case X:

struct toy {
  int id;
  int value;
};

static int LOLGlobal = TWO;
struct toy t1 = {.id = 31, .value = TWO};
static __attribute__((constructor)) void myctor() {
  printf("ctor lolglobal is %d\n", LOLGlobal);
  printf("t1.id = %d, t1.value = %d\n", t1.id, t1.value);
}
static int okpls = 1;

int adder(int x, int y) {
  return TWO + x + y + TWO;
}

void exam_func(int value) {
  const int THREE = 3;
  switch (value) {
    case 1: {
      // might create a variable in this scope
      something_1();
      printf("you got a 1\n");
      break;
    }

#ifdef HAVE_TWO
      LOLMAX(TWO)
      something_2();
      printf("you got a 2\n");
      // fall-through
#endif

    case THREE:
      something_3();
      printf("you got a 3\n");
      break;

    case 19 ... 27:
      printf("sorry you don't get a number\n");
      printf("hello TWO is %d\n", TWO);
      break;

    case 0:
      something_0();
      // fall-through

    default:
      printf("default you got a %d\n", value);
  }
  printf("DONE WITH SWITCH\n");
  printf("----------------\n");
}

void init_func() {
  static const struct toy t = {.id = 22,
                               /* OMG why do a comment here */
                               .value = TWO};
  static int vals[] = {1, TWO, 3, TWO, 1};

  printf("bye t.id = %d, t.value = %d\n", t.id, t.value);
  for (int i = 0; i < 5; ++i) {
    printf("vals[%d] = %d\n", i, vals[i]);
  }

  struct toy box[] = {
      {.id = 1, .value = 1},
      {.id = 1, .value = TWO},
      {.id = TWO, .value = 1},
      {.id = TWO, .value = TWO},
  };
  for (int i = 0; i < 4; ++i) {
    printf("box[%d]: id=%d, value=%d\n", i, box[i].id, box[i].value);
  }

  for (int i = 0; i < TWO; i++) {
    printf("%d ", i);
  }
  printf("\n");
  /* I can't mod the below expr because of constant folding */
  /*
  for (int j = TWO-1; j > 0; j--) {
    printf("%d ", j);
  }
  */
  static int LOL = /*
                        commenting for posterity
                       */
      TWO;
  printf("LOL is %d\n", LOL);
}

int main(int argc, char **argv) {
  /* printf("This is the modded example\n"); */
  printf("hello TWO is %d\n",
         /* hello I want a comment here */
         TWO);
  exam_func(1);
  exam_func(2);
  exam_func(3);
  exam_func(0);
  exam_func(8);
  exam_func(22);
  init_func();
  return 0;
}
