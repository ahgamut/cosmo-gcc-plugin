#include <stdio.h>

extern void something_0();
extern void something_1();
extern void something_2();
extern void something_3();
extern const int TWO;
extern const int THREE;

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

int main(int argc, char **argv) {
  printf("This is what I expect after modding\n");
  printf("hello TWO is %d\n", TWO);
  exam_func(1);
  exam_func(2);
  exam_func(3);
  exam_func(0);
  exam_func(8);
  exam_func(22);
  return 0;
}
