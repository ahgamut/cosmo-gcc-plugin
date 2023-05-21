#include <stdio.h>
#include <string.h>

/* testing initializations of global structs */

struct toy {
  int id;
  int value;
};

struct toyroom {
  int id;
  int value;
  struct toy x1;
  struct toy x2;
  struct toy x3;
  struct toy x4;
};

static int v1 = TWO;
static int arr1[] = {TWO, THREE, ~THREE, ~TWO};
struct toy t0 = {.id = 30, .value = THREE};
static struct toy t1 = {.id = 31, .value = TWO};
static const struct toy ta[] = {{.id = 1, .value = ~TWO},
                                {.id = ~THREE, .value = TWO},
                                {.value = TWO, .id = -THREE},
                                {.id = THREE, .value = 1},
                                {.id = 7, .value = THREE}};
static const struct toyroom r1 = {
    .id = 2,
    .value = -TWO,
    .x1 = {.id = 1, .value = TWO},
    .x2 = {.id = ~TWO, .value = 1},
    .x3 = {.value = TWO, .id = -THREE},
    .x4 = {.id = THREE, .value = ~THREE},
};

static __attribute__((constructor)) void myctor() {
  printf("v1 is %d\n", v1);
  printf("t0.id = %d, t0.value = %d\n", t1.id, t1.value);
  printf("t1.id = %d, t1.value = %d\n", t1.id, t1.value);
  for (int i = 0; i < 5; ++i) {
    printf("ta[%d].id = %d, ta[%d].value = %d\n", i, ta[i].id, i, ta[i].value);
  }
  printf("r1.id = %d, r1.value = %d\n", r1.id, r1.value);
  printf("r1.x1.id = %d, r1.x1.value = %d\n", r1.x1.id, r1.x1.value);
  printf("r1.x2.id = %d, r1.x2.value = %d\n", r1.x2.id, r1.x2.value);
  printf("r1.x3.id = %d, r1.x3.value = %d\n", r1.x3.id, r1.x3.value);
  printf("r1.x4.id = %d, r1.x4.value = %d\n", r1.x4.id, r1.x4.value);
}
static int okpls = 1;

int main(int argc, char **argv) {
  printf("hi\n");
  return 0;
}
