#include <stdio.h>
#include <string.h>

/* testing duff's device */

void exam_func(int value, int n) {
  switch (value) {
    case THREE: {
      int c = 22;
      something_3();
      printf("you got a 3\n");
      if (n > 10) {
          printf("exiting\n");
          return;
      }
      break;
    }

    case -THREE:
      printf("you got a -3\n");
      break;

    case ~TWO:
      int d = 111;
      printf("you got a ~2\n");
      break;

    case (TWO * THREE):
      printf("this is an arbitrary expression: %d\n", TWO * THREE);
      break;

    case 19 ... 27:
      printf("sorry you don't get a number\n");
      printf("hello TWO is %d\n", TWO);
      break;

    do {
    case 28 ... 36:
      printf("new case\n");
      if (value % 2 == 0) {
        if (value % 4 == 0) {
          printf("bye bye value was %d\n", value);
          break;
        }
      }

    case 0:
      something_0();
      printf("INSIDE THE DUFF: value is %d, n is %d\n", value, n);
      // fall-through
    } while (--n > 1);

    default:
      int z = 12;
      printf("default you got a %d\n", value);
      break;
  }
  printf("DONE WITH SWITCH\n");
  printf("----------------\n");
}

int main(int argc, char **argv) {
  exam_func(0, 0);
  exam_func(THREE, 0);
  exam_func(THREE, 22);
  exam_func(~TWO, 1);
  exam_func(-THREE, 1);
  exam_func(TWO * THREE, 1);
  exam_func(22, 0);
  exam_func(32, 3);
  exam_func(33, 3);
  return 0;
}
