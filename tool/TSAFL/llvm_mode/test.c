#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int ret;
char *ptr2;
char buf[7];
char *ptr1;
#define TEST_WHILE
void *t1(void *data) {
#ifdef TEST_WHILE
  for (size_t i = 0; i < 4; i++) {
    ret += 1;
  }
#endif
  printf("--t1 01--\n");
  printf("--t1 02--\n");
  printf("--t1 03--\n");
}

void *t2(void *data) {
  printf("--t2 01--\n");
  printf("--t2 02--\n");
  printf("--t2 03--\n");
}

void useless();
int main(int argc, char **argv) {
  printf("start\n");
  int a = 0;
  int b = 100;
  b = a + b;
  ret = 0;
  ptr1 = (char *)malloc(7);
  ptr2 = (char *)malloc(7);
  pthread_t id1, id2;
  pthread_create(&id2, NULL, t2, NULL);
  pthread_create(&id1, NULL, t1, NULL);
  printf("--main 01--\n");
  printf("--main 02--\n");
  printf("--main 03--\n");
  pthread_join(id1, NULL);
  pthread_join(id2, NULL);
}