#include <stdio.h>
#include <string.h>

/* testing function calls */

int adder(int x, int y) {
  printf("x is %d, y is %d\n", x, y);
  return x + y + THREE;
}

int dummy(int x) {
  if (x == 1) {
    return ~TWO;
  }
  return -TWO;
}

int main(int argc, char **argv) {
  printf("hello TWO is %d\n",
         /* hello I want a comment here */
         TWO);
  printf("dummy(1) = %d\n", dummy(1));
  printf("dummy(2) = %d\n", dummy(2));
  adder(1, TWO);
  return 0;
}
