#include <stdio.h>
#include <string.h>

/* testing switch inside switch */

void exam_func(int value) {
  switch (value) {
    case 1: {
      int a = 21;
      something_1();
      printf("you got a 1\n");
      break;
    }

    case TWO:
      something_2();
      printf("you got a 2\n");
      break;

    default:
      int z = 12;
      printf("default you got a %d\n", value);
      switch (value) {
        case THREE: {
          int c = 22;
          something_3();
          printf("you got a 3\n");
          break;
        }

        case -THREE:
          printf("you got a -3\n");
          break;

        case ~TWO:
          int d = 111;
          printf("you got a ~2\n");
          break;

        case 19 ... 27:
          printf("sorry you don't get a number\n");
          printf("hello TWO is %d\n", TWO);
          break;

        case 0:
          something_0();
          // fall-through

        default:
          printf("default within default\n");
          break;
      }
      break;
  }
  printf("DONE WITH SWITCH\n");
  printf("----------------\n");
}

int main(int argc, char **argv) {
  exam_func(1);
  exam_func(2);
  exam_func(3);
  exam_func(0);
  exam_func(8);
  exam_func(22);
  exam_func(~TWO);
  exam_func(-THREE);
  return 0;
}
