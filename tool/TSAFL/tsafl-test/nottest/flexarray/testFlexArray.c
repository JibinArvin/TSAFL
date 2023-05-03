#include "alloc-inl.h"
#include "types.h"
#include <stdio.h>

struct var_action_token {
  u64 size;
  u32 list[];
};

int main() {
  struct var_action_token* v1 = ck_alloc(sizeof(struct var_action_token) + 100*sizeof(u32));
  v1->size = 100;
  v1->list[1] = 2;
  v1->list[99] = 3;
  printf("v1->list[1]: %d.\n", v1->list[1]);
  ck_free(v1);
}