#include <stdio.h>

extern void something_0();
extern void something_1();
extern void something_2();
extern void something_3();
extern const int TWO;
extern const int THREE;

struct toy {
  int id;
  int value;
};

void exam_func(int value) {
  if (1 == value) goto __plugin_switch_case_1;
  if (TWO == value) goto __plugin_switch_case_TWO;
  if (THREE == value) goto __plugin_switch_case_THREE;
  if (19 <= value && 27 >= value) goto __plugin_switch_case_FOUR;
  if (0 == value) goto __plugin_switch_case_0;

  goto __plugin_switch_default;

  {
  __plugin_switch_case_1 : {
    something_1();
    printf("you got a 1\n");
    goto __plugin_switch_end;
  }

  __plugin_switch_case_TWO:
    something_2();
    printf("you got a 2\n");

  __plugin_switch_case_THREE:
    something_3();
    printf("you got a 3\n");
    goto __plugin_switch_end;

  __plugin_switch_case_FOUR:
    printf("sorry you don't get a number\n");
    printf("hello TWO is %d\n", TWO);
    goto __plugin_switch_end;

  __plugin_switch_case_0:
    something_0();
    // fall-through

  __plugin_switch_default:
    printf("default you got a %d\n", value);
    goto __plugin_switch_end;
  }
__plugin_switch_end:;
  ;

  // rest of your code after the switch is unchanged
  printf("DONE WITH SWITCH\n");
  printf("----------------\n");
}

void init_func() {
  struct toy t = {.id = 22,
                  /* OMG why do a comment here */
                  .value = TWO};
  int vals[] = {1, TWO, 3, TWO, 1};

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
  int LOL = /*
                        commenting for posterity
                       */
      TWO;
  printf("LOL is %d\n", LOL);
}

int main(int argc, char **argv) {
  /* printf("This is what I expect after modding\n"); */
  printf("hello TWO is %d\n", TWO);
  exam_func(1);
  exam_func(2);
  exam_func(3);
  exam_func(0);
  exam_func(8);
  exam_func(22);
  init_func();
  return 0;
}
