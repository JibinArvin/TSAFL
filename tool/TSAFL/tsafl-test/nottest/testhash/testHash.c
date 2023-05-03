#include "alloc-inl.h"
#include "hashmap.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* array to store vars bits. */
struct var_info_bits {
  int trace_bits;
};

static int vitop_array_size = 0;
static struct var_info_bits *vitop_array[1000];
static struct hashmap *vitop_map = NULL;

struct var_int_to_ptr {
  char number[8]; /* Example : 1 (which actually is number).*/
  struct var_info_bits *ptr;
};


static int var_itop_cmp(const void *a, const void *b, void *udata) {
  const struct var_int_to_ptr *va = a;
  const struct var_int_to_ptr *vb = b;
  return strcmp(va->number, vb->number);
}

static uint64_t var_itop_hash(const void *item, uint64_t seed0,
                              uint64_t seed1) {
  const struct var_int_to_ptr *va = item;
  return hashmap_sip(va->number, strlen(va->number), seed0, seed1);
}

static bool var_itop_iter(const void *item, void *udata) {
  const struct var_int_to_ptr *v = item;
  printf("iter in var_top_map -> number: %s, ptr?: %s.\n", v->number,
         v->ptr == NULL ? "not exist" : "exist");
  return true;
}

/* Search avalibale vitop in map. */
static struct var_info_bits *get_vitop_byChar(struct hashmap *var_map,
                                              const char *name) {
  if (var_map == NULL) {
    fprintf(stderr, "[ERROR] Init vitop_map firstly!");
    return NULL;
  }
  struct var_int_to_ptr *res;
  struct var_int_to_ptr *search = malloc(sizeof(struct var_int_to_ptr));
  strcpy(search->number, name);
  res = hashmap_get(var_map, search);
  free(search);
  if (res == NULL) {
    return NULL;
  } else {
    return res->ptr;
  }
  return NULL;
}

static bool add_var_vitop(struct hashmap *var_map, struct var_int_to_ptr *v) {
  if (!var_map || !v) {
    fprintf(stderr, "[ERROR] add_var_vitop func -> invalid var_map or v.");
    return false;
  }
  hashmap_set(var_map, v);
  if (vitop_array_size < 1000) {
    vitop_array[vitop_array_size++] = v->ptr;
  } else {
    fprintf(stderr, "[ERROR] too much var for var_info_bits array.");
  }
  return true;
}

void free_vitop_map(struct hashmap* vitop_map) {
  size_t iter = 0;
  void *item;
  while(hashmap_iter(vitop_map, &iter, &item)) {
    const struct var_int_to_ptr* v = item;
    if(v->ptr!= NULL) {
      ck_free(v->ptr);
    }
  }
  hashmap_free(vitop_map);
}

struct user {
  char name[1000];
  int age;
};

int user_compare(const void *a, const void *b, void *udata) {
  const struct user *ua = a;
  const struct user *ub = b;
  return strcmp(ua->name, ub->name);
}

bool user_iter(const void *item, void *udata) {
  const struct user *user = item;
  printf("%s (age=%d)\n", user->name, user->age);
  return true;
}

uint64_t user_hash(const void *item, uint64_t seed0, uint64_t seed1) {
  const struct user *user = item;
  printf("\nduring hash user name:   %s.\n", user->name);
  return hashmap_sip(user->name, strlen(user->name), seed0, seed1);
}

int main() {
  // create a new hash map where each item is a `struct user`. The second
  // argument is the initial capacity. The third and fourth arguments are
  // optional seeds that are passed to the following hash function.
  struct hashmap *map = hashmap_new(sizeof(struct user), 0, 0, 0, user_hash,
                                    user_compare, NULL, NULL);

  // Here we'll load some users into the hash map. Each set operation
  // performs a copy of the data that is pointed to in the second argument.
  hashmap_set(map, &(struct user){.name = "Dale", .age = 44});
  hashmap_set(map, &(struct user){.name = "Roger", .age = 68});
  hashmap_set(map, &(struct user){.name = "Jane", .age = 47});
  struct user *u = malloc(sizeof(struct user));
  strcpy(u->name, "Arvin");
  u->age = 26;
  hashmap_set(map, u);
  free(u); /* Which tell error in address sanitizer. */
  struct user *user;
  // free(u->name);
  // free(u);
  printf("\n-- get some users --\n");
  user = hashmap_get(map, &(struct user){.name = "Jane"});
  printf("%s age=%d\n", user->name, user->age);

  user = hashmap_get(map, &(struct user){.name = "Roger"});
  printf("%s age=%d\n", user->name, user->age);

  user = hashmap_get(map, &(struct user){.name = "Arvin"});
  printf("%s age=%d\n", user->name, user->age);

  user = hashmap_get(map, &(struct user){.name = "Dale"});
  printf("%s age=%d\n", user->name, user->age);

  user = hashmap_get(map, &(struct user){.name = "Tom"});
  printf("%s\n", user ? "exists" : "not exists");

  printf("\n-- iterate over all users (hashmap_scan) --\n");
  hashmap_scan(map, user_iter, NULL);

  printf("\n-- iterate over all users (hashmap_iter) --\n");
  size_t iter = 0;
  void *item;
  while (hashmap_iter(map, &iter, &item)) {
    const struct user *user = item;
    printf("%s (age=%d)\n", user->name, user->age);
  }

  hashmap_free(map);
  printf("var_itop test init");

  vitop_map = hashmap_new(sizeof(struct var_int_to_ptr), 0, 0, 0, var_itop_hash,
                          var_itop_cmp, NULL, NULL);
  for(int i = 0; i < 3; i++) {
    struct var_int_to_ptr* tmp = malloc(sizeof(struct var_int_to_ptr));
    char name[100] = "1111";
    name[0] += i;
    strcpy(tmp->number, name);
    tmp->ptr = NULL;
    add_var_vitop(vitop_map, tmp);
    free(tmp);
  }
  struct var_int_to_ptr* tmp = malloc(sizeof(struct var_int_to_ptr));
  strcpy(tmp->number, "999"); 
  tmp->ptr = ck_alloc(sizeof(struct var_info_bits));
  add_var_vitop(vitop_map, tmp);
  iter = 0;
  while(hashmap_iter(vitop_map, &iter, &item)) {
    const struct var_int_to_ptr* v = item;
    printf("v_number: %s, if have ptr:  %s.\n", v->number, v->ptr?"exist":"no exist");
  }
  free(tmp);
  free_vitop_map(vitop_map);
}

// output:
// -- get some users --
// Jane age=47
// Roger age=68
// Dale age=44
// not exists
//
// -- iterate over all users (hashmap_scan) --
// Dale (age=44)
// Roger (age=68)
// Jane (age=47)
//
// -- iterate over all users (hashmap_iter) --
// Dale (age=44)
// Roger (age=68)
// Jane (age=47)
