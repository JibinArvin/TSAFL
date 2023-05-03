#include "../readxml.h"
#include "../alloc-inl.h"
#include "libxml/globals.h"
#include "libxml/tree.h"
#include "libxml/xmlstring.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void format_function_arrary();
static void clean_v_array();
static int parse_distance_child(xmlDocPtr doc, xmlNodePtr cur,
                                struct distance_container *dis_node);
static int parse_function_node(xmlDocPtr doc, xmlNodePtr cur,
                               struct function_container *func_node);
static int parse_op_childs(xmlDocPtr doc, xmlNodePtr cur,
                           struct variable_container *v_node);
struct op_node parse_op_node(xmlDocPtr doc, xmlNodePtr cur);

void init_xml_config(struct function_array **f_array,
                     struct variable_array **v_array,
                     struct distance_array **d_array,
                     struct groupKey_array **g_array) {

  xmlKeepBlanksDefault(0);
  *f_array = (struct function_array *)ck_alloc(sizeof(struct function_array));
  *v_array = (struct variable_array *)ck_alloc(sizeof(struct variable_array));
  *d_array = (struct distance_array *)ck_alloc(sizeof(struct distance_array));
  *g_array = (struct groupKey_array *)ck_alloc(sizeof(struct groupKey_array));

  for (int i = 0; i < OP_MAX_COUNT; i++) {
    (*(*f_array)).list[i] = NULL;
    (*(*d_array)).list[i] = NULL;
    (*(*v_array)).list[i] = NULL;
    (*(*g_array)).list[i] = NULL;
  }
  (*(*f_array)).size = 0;
  (*(*d_array)).size = 0;
  (*(*v_array)).size = 0;
  (*(*g_array)).size = 0;
}

static int parse_distance_child(xmlDocPtr doc, xmlNodePtr cur,
                                struct distance_container *dis_node) {
  assert(doc || cur);
  dis_node->size = 0;
  if (cur != NULL) {
    cur = cur->children;
    while (cur != NULL) {
      if (!xmlStrcmp(cur->name, (xmlChar *)"End")) {
        xmlChar *message = xmlGetProp(cur, (xmlChar *)"Node_count");
        dis_node->nexter[dis_node->size] = atoi((char *)message);
        xmlFree(message);
        message = xmlGetProp(cur, (xmlChar *)"Value");
        dis_node->path[dis_node->size] = (struct distance_node_info *)ck_alloc(
            sizeof(struct distance_node_info));
        struct distance_node_info *tmp = dis_node->path[dis_node->size];
        tmp->value = atoi((char *)message);
        tmp->node_size = 0;
        memset(tmp->node, -1, PATH_NODE_MAX * sizeof(int));
        xmlNode *child = cur->children;
        while (child != NULL) {
          if (!xmlStrcmp(child->name, (const xmlChar *)"Node_count ")) {
            xmlChar *message_child = xmlGetProp(child, (xmlChar *)"Value");
            tmp->node[tmp->node_size] = atoi((char *)message_child);
            tmp->node_size++;
            xmlFree(message_child);
          }
          child = child->next;
        }
        xmlFree(message);
      }
      dis_node->size++;
      cur = cur->next;
    }
  } else {
    fprintf(stderr, "Error: NULL node error");
  }
  return 0;
}

int parse_root_node_finalXML(const char *file_name,
                             struct function_array *f_array) {
  assert(file_name);
  xmlDocPtr doc;  // xml整个文档的树形结构
  xmlNodePtr cur; // xml节点
  doc = xmlParseFile(file_name);
  if (doc == NULL) {
    fprintf(stderr, "Failed to parse xml file:%s\n", file_name);
    goto FAILED;
  }

  cur = xmlDocGetRootElement(doc);
  if (cur == NULL) {
    fprintf(stderr, "Root is empty.\n");
    goto FAILED;
  }

  if (xmlStrcmp(cur->name, (xmlChar *)"ROOTNODE")) {
    fprintf(stderr, "The root name is not ROOTNODE.\n");
    goto FAILED;
  }

  cur = cur->xmlChildrenNode;
  while (cur != NULL) {
    if (!xmlStrcmp(cur->name, (const xmlChar *)"FUNCTION")) {
      xmlChar *function_name = xmlGetProp(cur, (xmlChar *)"functionName");
      struct function_container *func_node =
          (struct function_container *)ck_alloc(
              sizeof(struct function_container));
      f_array->list[f_array->size++] = func_node;
#ifdef DEBUG_READ_XML_C_ALL
      printf("f_size update%d\n", f_array->size);
#endif
      strcpy(func_node->name, (char *)function_name);
      parse_function_node(doc, cur, func_node);
      xmlFree(function_name);
    }
    cur = cur->next;
  }
  xmlFreeDoc(doc);
  return 0;

FAILED:
  if (doc) {
    xmlFreeDoc(doc);
  }
  return -1;
}

/**
 * @brief parse root node distance_xml
 *
 * @param filename
 * @return int
 */
int parse_distance_xml(const char *file_name, struct distance_array *d_array) {
  assert(file_name);

  xmlDocPtr doc;  // xml整个文档的树形结构
  xmlNodePtr cur; // xml节点
  doc = xmlParseFile(file_name);
  if (doc == NULL) {
    fprintf(stderr, "Failed to parse xml file:%s\n", file_name);
    goto FAILED;
  }

  cur = xmlDocGetRootElement(doc);
  if (cur == NULL) {
    fprintf(stderr, "Root is empty.\n");
    goto FAILED;
  }
  if (xmlStrcmp(cur->name, (xmlChar *)"ROOTNODE")) {
    fprintf(stderr, "The root name is not ROOTNODE.\n");
    goto FAILED;
  }

  cur = cur->xmlChildrenNode;
  while (cur != NULL) {
    if (!xmlStrcmp(cur->name, (const xmlChar *)"Start")) {
      // Node_count means
      xmlChar *v_name = xmlGetProp(cur, (xmlChar *)"Node_count");
      struct distance_container *d_c_temp =
          (struct distance_container *)ck_alloc(
              sizeof(struct distance_container));
      d_c_temp->size = 0;
      memset(d_c_temp->nexter, -1, NODE_REALATION_MAX_NUMBER * sizeof(int));
      // TODO: change
      memset(d_c_temp->path, 0,
             NODE_REALATION_MAX_NUMBER * sizeof(struct distance_node_info *));
      d_c_temp->owner = atoi((const char *)v_name);
      d_array->list[d_array->size++] = d_c_temp;
      // TODO: parse child
      parse_distance_child(doc, cur, d_c_temp);
      xmlFree(v_name);
    }
    cur = cur->next;
  }
  xmlFreeDoc(doc);
  return 0;
FAILED:
  if (doc) {
    xmlFreeDoc(doc);
  }
  return -1;
}

/**
 * @brief parse root node sensitiveXML
 *
 * @param file_name
 * @return int
 */

int parse_root_node_sensitiveXML(const char *file_name,
                                 struct variable_array *v_array) {

  assert(file_name);

  xmlDocPtr doc;  // xml整个文档的树形结构
  xmlNodePtr cur; // xml节点

  // NOTE: repeated in both root parse
  doc = xmlParseFile(file_name);
  if (doc == NULL) {
    fprintf(stderr, "Failed to parse xml file:%s\n", file_name);
    goto FAILED;
  }

  cur = xmlDocGetRootElement(doc);
  if (cur == NULL) {
    fprintf(stderr, "Root is empty.\n");
    goto FAILED;
  }

  if (xmlStrcmp(cur->name, (xmlChar *)"ROOTNODE")) {
    fprintf(stderr, "The root name is not ROOTNODE.\n");
    goto FAILED;
  }

  cur = cur->xmlChildrenNode;
  while (cur != NULL) {
    // FIXME: why there is value? Value have no releationship with name.
    if (!xmlStrcmp(cur->name, (const xmlChar *)"Value")) {
      xmlChar *v_name = xmlGetProp(cur, (xmlChar *)"VName");
      struct variable_container *v_c_tem =
          (struct variable_container *)ck_alloc(
              sizeof(struct variable_container));
      v_c_tem->v_name = atoi((char *)v_name);
      v_array->list[v_array->size++] = v_c_tem;
      parse_op_childs(doc, cur, v_c_tem);
      xmlFree(v_name);
    }
    cur = cur->next;
  }
  xmlFreeDoc(doc);
  return 0;

FAILED:
  if (doc) {
    xmlFreeDoc(doc);
  }
  return -1;
}

static int parse_function_node(xmlDocPtr doc, xmlNodePtr cur,
                               struct function_container *func_node) {
  assert(doc || cur);
  func_node->size = 0;
  if (cur != NULL) {
    cur = cur->children;
    while (cur != NULL) {
      if ((!xmlStrcmp(cur->name, (xmlChar *)"OPERATION"))) {
        func_node->op_list[func_node->size++] = parse_op_node(doc, cur);
#ifdef DEBUG_READ_XML_C
        printf("action %s\n",
               func_node->op_list[func_node->size - 1].actionType);
#endif
      }
      cur = cur->next;
    }
  } else {
    fprintf(stderr, "Error: NULL node error");
  }
  return 0;
}

static int parse_op_childs(xmlDocPtr doc, xmlNodePtr cur,
                           struct variable_container *v_node) {
  assert(doc || cur);
  v_node->size = 0;
  if (cur != NULL) {
    cur = cur->children;
    while (cur != NULL) {
      if (!xmlStrcmp(cur->name, (xmlChar *)"OPERATION")) {
        v_node->op_list[v_node->size++] = parse_op_node(doc, cur);
#ifdef DEBUG_READ_XML_C
        printf("action %s\n", v_node->op_list[v_node->size - 1].actionType);
#endif
      }
      cur = cur->next;
    }
  } else {
    fprintf(stderr, "Error: NULL node error");
  }
  return 0;
}

struct op_node parse_op_node(xmlDocPtr doc, xmlNodePtr cur) {
  assert(doc || cur);
  struct op_node re;
  xmlChar *message;
  if (cur != NULL) {
    if ((!xmlStrcmp(cur->name, (xmlChar *)"OPERATION"))) {
      message = xmlGetProp(cur, (xmlChar *)"tochMemLoc");
      if (xmlStrcmp(message, (xmlChar *)"N0N")) {
        re.tochMemLoc = -1;
      } else {
        int tochMemLoc = atoi((char *)message);
        re.tochMemLoc = tochMemLoc;
      }
      xmlFree(message);

      message = xmlGetProp(cur, (xmlChar *)"actionType");
      strcpy(re.actionType, (char *)message);
      xmlFree(message);

      message = xmlGetProp(cur, (xmlChar *)"sourceLoc");
      strcpy(re.sourceLoc, (char *)message);
      xmlFree(message);

      message = xmlGetProp(cur, (xmlChar *)"count");
      int count = atoi((char *)message);
      re.count = count;
      xmlFree(message);

      message = xmlGetProp(cur, (xmlChar *)"loop");
      if (message != NULL) {
        re.loop = atoi((char *)message);
        xmlFree(message);
      } else {
        re.loop = NOT_EXSIT;
      }
      // TODO: read loop node
      // printf("action: %s\n", re.actionType);
    } else {
      re.count = NOT_EXSIT;
    }
  } else {
    fprintf(stderr, "Error: NULL node error");
    exit(1);
  }
  return re;
}

static int parse_value_node(xmlDocPtr doc, xmlNodePtr cur,
                            struct group_key_container *g_inner) {
  assert(doc || cur);
  g_inner->size = 0;
  xmlChar *message;
  if (cur != NULL) {
    cur = cur->children;
    while (cur != NULL) {
      if (!xmlStrcmp(cur->name, (xmlChar *)"VALUE")) {
        message = xmlGetProp(cur, (xmlChar *)"Number");
        char *temp = (char *)message;
        assert(message != NULL);
        int number = strtol(temp, NULL, 0);
        g_inner->list[g_inner->size++] = 0;
        xmlFree(message);
      }
      cur = cur->next;
    }
  }
  return 0;
}

int parse_groupKey_xml(const char *file_name, struct groupKey_array *g_array) {
  assert(file_name);
  xmlDocPtr doc;
  xmlNodePtr cur;

  doc = xmlParseFile(file_name);
  if (doc == NULL) {
    fprintf(stderr, "Failed to parse xml file:%s\n", file_name);
    goto FAILED;
  }

  cur = xmlDocGetRootElement(doc);
  if (cur == NULL) {
    fprintf(stderr, "Root is empty.\n");
    goto FAILED;
  }

  if (xmlStrcmp(cur->name, (xmlChar *)"ROOTNODE")) {
    fprintf(stderr, "The root name is not ROOTNODE.\n");
    goto FAILED;
  }

  cur = cur->xmlChildrenNode;
  while (cur != NULL) {
    if (!xmlStrcmp(cur->name, (const xmlChar *)"KEY")) {
      xmlChar *key_Number = xmlGetProp(cur, (xmlChar *)"Number");
      assert(key_Number != NULL);
      char *temp = (char *)key_Number;
      int key_Number_int = strtol(temp, NULL, 0);
      struct group_key_container *g_item =
          (struct group_key_container *)ck_alloc(
              sizeof(struct group_key_container));
      g_array->list[g_array->size++] = g_item;
      assert(g_item);
      g_item->key = key_Number_int;

      //   /* Parse child node */
      parse_value_node(doc, cur, g_item);

      xmlFree(key_Number);
    }
    cur = cur->next;
  }
FAILED:
  if (doc) {
    xmlFreeDoc(doc);
  }
  return -1;
}

static void dump_g_array(struct groupKey_array *g_array) {
  int size = g_array->size;
  printf("g_array size : %d. \n", size);
  int real_size = 0;
  for (int i = 0; i < OP_MAX_COUNT; i++) {
    if (g_array->list[i] == NULL) {
      real_size = i;
      break;
    }
    printf("g_array[%d]->key: %d, size:%d.\n", i, g_array->list[i]->key,
           g_array->list[i]->size);
  }
}

// FIXME: lead to segmentation fault
static void format_function_arrary(struct function_array *f_array) {
  int after_size = 0;
  int count[OP_MAX_COUNT];
  for (int i = 0; i < f_array->size; i++) {
    if (f_array->list[i]->size == 0) {
      ck_free(f_array->list[i]);
    } else {
      count[after_size++] = i;
    }
  }
  for (int i = 0; i < after_size; i++) {
    printf("count: %d, i:%d, loc: %d\n", after_size, i, count[i]);
  }
  f_array->size = after_size;
  struct function_container *list_temp[OP_MAX_COUNT];
  for (int i = 0; i < after_size; i++) {
    list_temp[i] = f_array->list[count[after_size]];
  }
  for (int i = 0; i < after_size; i++) {
    f_array->list[i] = list_temp[i];
  }
}

static void dump_f_array(struct function_array *f_array) {
  for (int i = 0; i < f_array->size; i++) {
    for (int j = 0; j < f_array->list[i]->size; j++) {
      printf("op_source %s\n", f_array->list[i]->op_list[j].sourceLoc);
    }
  }
}

static void dump_d_array(struct distance_array *d_array) {
  printf("Dunmping d_array. \n");
  for (int i = 0; i < d_array->size; i++) {
    printf("distance owner: %d, size: %d \n", d_array->list[i]->owner,
           d_array->list[i]->size);
    printf("in start node.\n");
    for (int j = 0; j < d_array->list[i]->size; j++) {
      printf("distance end node: %d.\n", d_array->list[i]->path[j]->value);
    }
  }
}

void ck_free_f_array(struct function_array *f_array) {
  for (int i = 0; i < f_array->size; i++) {
#ifdef DEBUG_READ_XML_C_ALL
    printf("f_size: %d\n", f_array.list[i]->size);
#endif
    if (f_array->list[i])
      ck_free(f_array->list[i]);
  }
  f_array->size = 0;
}

void clean_v_array(struct variable_array *v_array) {

  /* start clean v-array*/
  for (int i = 0; i < v_array->size; i++) {
#ifdef DEBUG_READ_XML_C_ALL
    printf("v_loc: %d\n", v_array->list[i]->size);
#endif
    ck_free(v_array->list[i]);
  }
  v_array->size = 0;
}

void clean_d_array(struct distance_array *d_array) {
  for (int i = 0; i < OP_MAX_COUNT; i++) {
    if (unlikely(d_array->list[i] == NULL)) {
      break;
    }
    for (int j = 0; j < d_array->list[i]->size; j++) {
      ck_free(d_array->list[i]->path[j]);
    }
    ck_free(d_array->list[i]);
  }
}

void clean_g_array(struct groupKey_array *g_array) {
  for (int i = 0; i < OP_MAX_COUNT; i++) {
    if (unlikely(g_array->list[i] == NULL)) {
      break;
    }
    ck_free(g_array->list[i]);
  }
}
/**
 * @brief ck_free memory in readxml.c
 *
 */
void clean_xml_config(struct function_array *f_array,
                      struct variable_array *v_array,
                      struct distance_array *d_array,
                      struct groupKey_array *g_array) {
  if (f_array == NULL) {
    printf("NULL pointer");
    exit(1);
  }
  ck_free_f_array(f_array);
  clean_v_array(v_array);
  clean_d_array(d_array);
  clean_g_array(g_array);
  ck_free(f_array);
  ck_free(v_array);
  ck_free(d_array);
  ck_free(g_array);
}

void check_v(struct variable_array *v_array) {
  printf("check v_array.\n");
  for (int i = 0; i < v_array->size; i++) {
    printf("size:  %d.\n", i);
    struct variable_container *v_c = v_array->list[i];
    printf("v_name:  %d.\n", v_c->v_name);
    for (int j = 0; j < v_c->size; j++) {
      printf("size:  %d | op source loc:  %s.\n", j, v_c->op_list[j].sourceLoc);
    }
  }
}

static void test_file() {
  struct function_array *f_array = NULL;
  struct variable_array *v_array = NULL;
  struct distance_array *d_array = NULL;
  struct groupKey_array *g_array = NULL;
  const char *final_xml = "finalXml.xml";
  const char *s_xml = "sensitive.xml";
  const char *d_xml = "distance.xml";
  const char *g_xml = "groupKeys.xml";
  init_xml_config(&f_array, &v_array, &d_array, &g_array);
  parse_root_node_finalXML(final_xml, f_array);
  parse_root_node_sensitiveXML(s_xml, v_array);
  parse_distance_xml(d_xml, d_array);
  parse_groupKey_xml(g_xml, g_array);
  dump_f_array(f_array);
  check_v(v_array);
  dump_d_array(d_array);
  dump_g_array(g_array);
  clean_xml_config(f_array, v_array, d_array, g_array);
}

// int main() {
//   test_file();
// //   dump_f_arrary();
// //   dump_d_array();
// //   format_function_arrary();
//   return 0;
// };
