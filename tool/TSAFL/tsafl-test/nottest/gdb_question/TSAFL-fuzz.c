#include "alloc-inl.h"
#include "config.h"
#include "hashmap.h"
#include "readxml.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define JIBIN_DEBUG

/* array to store vars bits. */
struct var_info_bits {
  int trace_bits;
};

static int vitop_array_size = 0;
static struct var_info_bits *vitop_array[1000];
static struct hashmap *vitop_map = NULL;

#define VAR_ITOP_NUMBER_LENTGH 8
struct var_int_to_ptr {
  char number[VAR_ITOP_NUMBER_LENTGH]; /* Example : 1 (which actually is
                                          number).*/
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
  struct var_int_to_ptr *search = ck_alloc(sizeof(struct var_int_to_ptr));
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

void free_vitop_map(struct hashmap *vitop_map) {
  size_t iter = 0;
  void *item;
  while (hashmap_iter(vitop_map, &iter, &item)) {
    const struct var_int_to_ptr *v = item;
    if (v->ptr != NULL) {
      ck_free(v->ptr);
    }
  }
  hashmap_free(vitop_map);
}

// NOTE: could be changed to ptr
struct function_array *f_array;
struct variable_array *v_array;
struct distance_array *d_array;
struct groupKey_array *g_array;

const char *final_xml = "finalXml.xml";
const char *s_xml = "sensitive.xml";
const char *d_xml = "distance.xml";
const char *g_xml = "groupKeys.xml";

struct op_info *op_info;

void init_op_info(struct op_info **op_info_ptr) {
  (*op_info_ptr) = (struct op_info *)ck_alloc(sizeof(struct op_info));
  (*op_info_ptr)->size = 0;
}
/* Only used to create op_info hash */
struct node_hash {
  char loc[SOURCELOC_LEN];
  struct op_node_extend *ptr;
};

int nodea_hash_cmpare(const void *a, const void *b, void *udata) {
  const struct node_hash *na = a;
  const struct node_hash *nb = b;
  return strcmp(na->loc, nb->loc);
}

uint64_t node_hash_func(const void *item, uint64_t seed0, uint64_t seed1) {
  const struct node_hash *ni = item;
  // printf("\nduring hash ni loc:   %s.\n", ni->loc);
  return hashmap_sip(ni->loc, strlen(ni->loc), seed0, seed1);
}

bool node_hash_iter(const void *item, void *udata) {
  const struct node_hash *n = item;
  printf("iter in node_hash loc: %s", n->loc);
  return true;
}
/* TODO: free memory don't finish. */
struct hashmap *op_map = NULL;

action_type get_from_char(const char *s) {
  if (strcmp(s, (char *)"read")) {
    return read;
  } else if (strcmp(s, (char *)"lock")) {
    return lock;
  } else if (strcmp(s, (char *)"write")) {
    return write;
  } else if (strcmp(s, (char *)"unlock")) {
    return unlock;
  }
  return default_action;
}

/* Used to free op_node_extend */

void *free_op_node_extend(struct op_node_extend *op_node) {
  assert(op_node != NULL);
  for (int i = 0; i < op_node->nextSize; i++) {
    assert(op_node->path[i] != NULL);
    ck_free(op_node->path[i]);
  }
  ck_free(op_node);
  return NULL;
}

static  struct op_node_extend *node_extend_TEMP; 
/* Used to free_op_info, Only be called once time. */
void free_op_info(struct op_info *op_info) {
  /* Make sure the op_map exist in the end. */
  size_t iter = 0;
  void *item;
  while (hashmap_iter(op_map, &iter, &item)) {
    const struct node_hash *node_extend = item;
    node_extend_TEMP = node_extend->ptr ;
    // free_op_node_extend(node_extend->ptr);
  }
  ck_free(op_info);
}

void add_toucMem_to_op_node(struct op_node_extend *op, int touchMem) {
  if (op->touchMemsize == TOUCH_MEM_MAX) {
    fprintf(stderr, "[ERROR] There "
                    "are more than 10 touchMem in single op_node_extend");
    exit(1);
  }
  op->tochMemLoc[op->touchMemsize] = touchMem;
  op->touchMemsize++;
}

void generate_op_info(struct function_array *f_array,
                      struct variable_array *v_array,
                      struct distance_array *d_array,
                      struct groupKey_array *g_array,
                      struct op_info **op_info_ptr) {
  assert(f_array != NULL && v_array != NULL && g_array != NULL &&
         d_array != NULL);
  op_map = hashmap_new(sizeof(struct node_hash), 0, 0, 0, node_hash_func,
                       nodea_hash_cmpare, NULL, NULL);
  struct node_hash *node_h;
  /* get op_info size from the f_array */
  u32 op_info_size = 0;
  for (int i = 0; i < f_array->size; i++) {
    op_info_size++;
    struct function_container *f_node = f_array->list[i];
    op_info_size += f_node->size;
  }

  /* Give the info_op memory (one more) here */
  init_op_info(op_info_ptr);
  /* Generate real node info from f_array.
  (node in f_array is almost complete and f_array include "lock"..)
  */
  /* TODO: delete same code. */
  for (int i = 0; i < f_array->size; i++) {
    struct function_container *f_node = f_array->list[i];

    for (int j = 0; j < f_node->size; j++) {
      struct node_hash *node_t = ck_alloc(sizeof(struct node_hash));
      struct op_node *const op_n = &(f_node->op_list[j]);
      strcpy(node_t->loc, op_n->sourceLoc);

#ifdef DEBUG_TEST
      printf("node_t->loc: %s. \n", node_t->loc);
      printf("count: %d.\n", i);
      printf("op_n->sourceLoc: %s.\n", op_n->sourceLoc);
#endif
      node_h = hashmap_get(op_map, node_t);
      int count = op_n->count;
      if (node_h == NULL) {
        struct op_node_extend *op_node_inInfo =
            (struct op_node_extend *)ck_alloc(sizeof(struct op_node_extend));
        op_node_inInfo->nextSize = 0;
        op_info->list[count] = op_node_inInfo;
        op_node_inInfo->actionType = get_from_char(op_n->actionType);
        strcpy(op_node_inInfo->sourceLoc, op_n->sourceLoc);
        op_node_inInfo->loop = op_n->loop;
        // add_toucMem_to_op_node(op_node_inInfo, op_n->tochMemLoc);
        node_t->ptr = op_node_inInfo;
        hashmap_set(op_map, node_t);
        op_info->size++;
      } else {
        struct op_node_extend *op_node_inInfo = node_h->ptr;
        op_info->list[count] = op_node_inInfo;
        // add_toucMem_to_op_node(op_node_inInfo, op_n->tochMemLoc);
      }
      /* Can't free mem here! */
      // free(node_t->loc);
      // free(node_t);
    }
  }
  /* Get th gruop info from g_array */
  for (int i = 0; i < g_array->size; i++) {
    struct group_key_container *g_node = g_array->list[i];
    const int key = g_node->key;
    struct op_node_extend *const op_node = op_info->list[key];
    assert(op != NULL); /* Make sure there is one. */
    op_node->key = -1;
    for (int j = 0; j < g_node->size; j++) {
      int value = g_node->list[j];
      assert(value <= op_node->size);
      struct op_node_extend *op_value_node = op_info->list[value];
      op_value_node->key = key;
    }
  }

  /* Get distance info from the d_array. */
  /* Share same ptr with d_array */
  for (int i = 0; i < d_array->size; i++) {
    struct distance_container *dis_c = d_array->list[i];
    int key = dis_c->owner;
    struct op_node_extend *op_node_inInfo = op_info->list[key];
    assert(op_node_inInfo->nextSize == 0);
    for (int j = 0; j < dis_c->size; j++) {
      int n = op_node_inInfo->nextSize;
      op_node_inInfo->nexter[n] = dis_c->nexter[j];
      struct distance_node_info *dis_tmp =
          (struct distance_node_info *)ck_alloc(
              sizeof(struct distance_node_info));
      op_node_inInfo->path[n] = dis_tmp;
      memcpy(op_node_inInfo->path[n], dis_c->path[j],
             sizeof(struct distance_node_info));
      op_node_inInfo->nextSize++;
    }
  }
}

void build_var_itop_map_and_array(struct hashmap *v_map,
                                  struct variable_array *v_array) {
  v_map = hashmap_new(sizeof(struct var_int_to_ptr), 0, 0, 0, var_itop_hash,
                      var_itop_cmp, NULL, NULL);
  for (int i = 0; i < v_array->size; i++) {
    struct var_int_to_ptr *var = ck_alloc(sizeof(struct var_int_to_ptr));
    int count = snprintf(var->number, VAR_ITOP_NUMBER_LENTGH, "%d",
                         v_array->list[i]->v_name);
#ifdef JIBIN_DEBUG
    fprintf(stderr,
            "[DEBUG] build_var_itop_map_and_array -> build one var_int_to_ptr: "
            "name: %s.\n",
            var->number);
#endif
    var->ptr = ck_alloc(sizeof(struct var_info_bits));
    add_var_vitop(v_map, var);
#ifdef JIBIN_DEBUG
    fprintf(stderr, "[DEBUG] build_var_itop_map_and_array -> finish add.\n");
    struct var_int_to_ptr *var_test;
    var_test = hashmap_get(v_map, var);
    if (var_test == NULL) {
      fprintf(stderr, "[ERROR] build_var_itop_map_and_array -> CAN't find same "
                      "node just insert.\n");
      exit(1);
      return;
    }
#endif
  }
#ifdef JIBIN_DEBUG
  printf("build_var_itop_map_and_array end");
#endif 
}

void dump_op_map(struct hashmap *op_map) {
  size_t iter = 0;
  void *item;
  while (hashmap_iter(op_map, &iter, &item)) {
    const struct node_hash *node = item;
    printf("op_node loc: %s .\n", node->loc);
  }
  printf("op_map count : %d", (int)hashmap_count(op_map));
  struct node_hash *node;
  node = hashmap_get(op_map, &(struct node_hash){.loc = "uaf.c:40"});
  printf("final : %s", node->loc);
}

int main() {
  init_xml_config(&f_array, &v_array, &d_array, &g_array);
  parse_root_node_finalXML(final_xml, f_array);
  parse_root_node_sensitiveXML(s_xml, v_array);
  parse_distance_xml(d_xml, d_array);
  parse_groupKey_xml(g_xml, g_array);
  generate_op_info(f_array, v_array, d_array, g_array, &op_info);
  // dump_op_map(op_map);
  // build_var_itop_map_and_array(vitop_map, v_array);

  // free_op_info(op_info);

  // hashmap_free(op_map);
  // assert(node_extend_TEMP->touchMemsize);
  free_vitop_map(vitop_map);
  clean_xml_config(f_array, v_array, d_array, g_array);
}