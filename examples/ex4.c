#include <stdio.h>
#include <string.h>

/* testing initializations of local structs */
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

void init_func() {
  static const struct toy t = {.id = 22,
                               /* OMG why do a comment here */
                               .value = TWO};
  static int vals[] = {1, TWO, -THREE, ~TWO, 1};

  printf("bye t.id = %d, t.value = %d\n", t.id, t.value);
  for (int i = 0; i < 5; ++i) {
    printf("vals[%d] = %d\n", i, vals[i]);
    vals[i] += 11;
  }

  struct toy box[] = {
      {.id = 1, .value = 1},       {.id = -THREE, .value = THREE},
      {.id = THREE, .value = 1},   {.id = -TWO, .value = ~TWO},
      {.id = TWO, .value = THREE}, {.id = ~THREE, .value = TWO},
  };
  for (int i = 0; i < 6; ++i) {
    printf("box[%d]: id=%d, value=%d\n", i, box[i].id, box[i].value);
    box[i].value -= i;
  }

  static const struct toyroom r2 = {
      .id = 2,
      .value = THREE,
      .x1 = {.id = 1, .value = TWO},
      .x2 = {.id = ~TWO, .value = 1},
      .x3 = {.id = TWO, .value = ~THREE},
      .x4 = {.id = THREE, .value = -THREE},
  };

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
  init_func();
  init_func();
  return 0;
}
