#include <stdio.h>

extern void something_0();
extern void something_1();
extern void something_2();
extern void something_3();

#define HAVE_TWO
#ifdef HAVE_TWO
extern const int TWO;
#endif

#define THREE 3

void exam_func(int value) {
  switch (value) {
    case 1: {
      // might create a variable in this scope
      something_1();
      printf("you got a 1\n");
      break;
    }

#ifdef HAVE_TWO
    case TWO:
      something_2();
      printf("you got a 2\n");
      // fall-through
#endif

    case THREE:
      something_3();
      printf("you got a 3\n");
      break;

    case 0:
      something_0();
      // fall-through

    default:
      printf("you got a %d\n", value);
  }
  printf("DONE WITH SWITCH\n");
  printf("----------------\n");
}

int main(int argc, char **argv) {
  printf("This is the modded example\n");
  exam_func(1);
  exam_func(2);
  exam_func(3);
  exam_func(0);
  exam_func(8);
  return 0;
}
