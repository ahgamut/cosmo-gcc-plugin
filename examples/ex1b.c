#include <stdio.h>
#include <string.h>

/* testing switch without default */

#define LOLMAX(X) case X:

void exam_func(int value) {
  switch (value) {
    case 1: {
      // might create a variable in this scope
      int a = 21;
      something_1();
      printf("you got a 1\n");
      break;
    }

#ifdef TWO
      LOLMAX(TWO)
      something_2();
      printf("you got a 2\n");
      // fall-through
#endif

    case THREE: {
      int c = 22;
      something_3();
      printf("you got a 3\n");
      break;
    }

    case 19 ... 27:
      printf("sorry you don't get a number\n");
      printf("hello TWO is %d\n", TWO);
      break;

    case 0:
      something_0();
      // fall-through
  }
  printf("DONE WITH SWITCH\n");
  printf("----------------\n");
}

int main(int argc, char **argv) {
  exam_func(1);
  exam_func(0);
  exam_func(TWO);
  exam_func(22);
  exam_func(100);
  return 0;
}
