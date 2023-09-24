#include <stdio.h>
#include <string.h>

/* testing __attribute__((fallthrough)) */

void exam_func(int value) {
  switch (value) {
    case TWO:
      printf("you got TWO, which is %d\n", TWO);
      __attribute__ ((fallthrough));
    case 2:
      printf("actually 2\n");
      break;
    default:
      printf("value = %d\n", value);
  }
}

int main(int argc, char **argv) {
  exam_func(TWO);
  exam_func(2);
  exam_func(THREE);
  return 0;
}
