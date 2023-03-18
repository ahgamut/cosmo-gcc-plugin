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

int main(int argc, char **argv) {
  printf("This is the modded example\n");
  static int LOL = TWO;
  printf("hello TWO is %d\n",
         /* hello I want a comment here */
         TWO);
  exam_func(1);
  exam_func(2);
  exam_func(3);
  exam_func(0);
  exam_func(8);
  exam_func(22);
  printf("bye LOL is %d\n", LOL);
  for (int i = 0; i < TWO; i++) {
    printf("%d ", i);
  }
  printf("\n");
  /* I can't the below expr because of constant folding */
  /*
  for (int j = TWO-1; j > 0; j--) {
    printf("%d ", j);
  }
  */
  printf("\n");
  return 0;
}
