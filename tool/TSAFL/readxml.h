#pragma once
#ifndef _READ_XML_C
#define _READ_XML_C
#include "config.h"
#include <assert.h>
#include <libxml/parser.h>
#include <libxml/xmlmemory.h>
#include <stdio.h>
#include <string.h>

//#define DEBUG_READ_XML_C
#define OP_MAX_COUNT 4096
#define NOT_EXSIT -1
#define PATH_NODE_MAX 128

struct op_node {
  int tochMemLoc; /* -1 means NON */
  char actionType[16];
  char sourceLoc[SOURCELOC_LEN];
  int count;
  int loop;
} __attribute__((aligned(16)));

struct function_container {
  char name[OP_MAX_COUNT]; /* Which could be assign anthor value. */
  struct op_node op_list[OP_MAX_COUNT];
  int size;
} __attribute__((aligned(16)));

struct function_array {
  int size;
  struct function_container *list[OP_MAX_COUNT];
} __attribute__((aligned(16)));

struct variable_container {
  int size;
  int v_name;
  struct op_node op_list[OP_MAX_COUNT];
} __attribute__((aligned(16)));

struct variable_array {
  int size;
  struct variable_container *list[OP_MAX_COUNT];
} __attribute__((aligned(16)));

struct distance_node_info {
  int value;
  int node_size;
  int node[PATH_NODE_MAX];
} __attribute__((aligned(16)));

struct distance_container {
  int size;
  int owner;
  /* Init as -1. */
  int nexter[NODE_REALATION_MAX_NUMBER];
  // TODO: change
  /* Init as 0 (not NULL) */
  struct distance_node_info *path[NODE_REALATION_MAX_NUMBER];
} __attribute__((aligned(16)));

struct distance_array {
  int size;
  struct distance_container *list[OP_MAX_COUNT];
} __attribute__((aligned(16)));

struct group_key_container {
  int size;
  int key;
  int list[OP_MAX_COUNT];
} __attribute__((aligned(16)));

struct groupKey_array {
  int size;
  struct group_key_container *list[OP_MAX_COUNT];
} __attribute__((aligned(16)));

typedef enum {
  lock_action,
  unlock_action,
  read_action,
  write_action,
  default_action,
} action_type;

#define TOUCH_MEM_MAX 10
struct op_node_extend {
  int touchMemsize;
  int tochMemLoc[TOUCH_MEM_MAX]; /* Also to sigh V_array */
  action_type actionType;
  char sourceLoc[SOURCELOC_LEN];
  int loop;
  u16 hashcode;
  int key;      /* -1 means it's key is itself. */
  int nextSize; /*make sure init as zero. */
  int nexter[NODE_REALATION_MAX_NUMBER];
  /* NOT Share with the d_array.*/
  struct distance_node_info *path[NODE_REALATION_MAX_NUMBER];
} __attribute__((aligned(16)));

/* Use Flexible array */
struct op_info {
  int size;
  struct op_node_extend *list[OP_MAX_COUNT];
} __attribute__((aligned(16)));

void init_xml_config(struct function_array **f_array,
                     struct variable_array **v_array,
                     struct distance_array **d_array,
                     struct groupKey_array **g_array);
int parse_root_node_finalXML(const char *file_name,
                             struct function_array *f_array);
int parse_root_node_sensitiveXML(const char *file_name,
                                 struct variable_array *v_array);
int parse_distance_xml(const char *file_name, struct distance_array *d_array);
int parse_groupKey_xml(const char *file_name, struct groupKey_array *g_array);
void clean_xml_config(struct function_array *f_array,
                      struct variable_array *v_array,
                      struct distance_array *d_array,
                      struct groupKey_array *g_array);

#endif