#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX_ORDER_LENGTH 100

typedef struct {
  char order_id[20];
  char status[20];
  double amount;
} Order;

typedef struct {
  Order order;
  FILE *output_file;
} ThreadArgs;

double total_amount = 0;

void *process_order(void *arg) {
  ThreadArgs *args = (ThreadArgs *)arg;
  Order order = args->order;
  FILE *output_file = args->output_file;
  if (strcmp(order.status, "PENDING") == 0) {
    // 处理待处理订单
    fprintf(output_file, "Processing order: %s, Amount: %.2f\n", order.order_id,
            order.amount);
    total_amount += order.amount;
    // 执行订单处理逻辑,如更新库存、生成发票等
    // ...
  } else if (strcmp(order.status, "SHIPPED") == 0) {
    // 处理已发货订单
    fprintf(output_file, "Updating shipping status for order: %s\n",
            order.order_id);
    total_amount -= order.amount;
    // 执行发货后的操作,如更新物流信息、发送通知等
    // ...
  } else if (strcmp(order.status, "COMPLETED") == 0) {
    // 处理已完成订单
    fprintf(output_file, "Archiving completed order: %s\n", order.order_id);
    // 执行完成订单后的操作,如归档、生成报告等
    // ...
  } else {
    fprintf(output_file, "Invalid order status for order: %s\n",
            order.order_id);
  }

  fprintf(output_file, "Here is total_amount right now %.2f\n", total_amount);

  free(arg);
  char s = 's';
  fprintf(output_file, "Here is total_amount right now %s\n", &s);
  return NULL;
}

void process_orders_file(const char *input_file, FILE *output_file) {
  FILE *file = fopen(input_file, "r");
  if (file == NULL) {
    printf("Failed to open input file.\n");
    return;
  }

  char line[MAX_ORDER_LENGTH];
  pthread_t threads[MAX_ORDER_LENGTH];
  int thread_count = 0;

  while (fgets(line, sizeof(line), file) != NULL) {
    // 解析订单信息
    Order order;
    sscanf(line, "%[^,],%[^,],%lf", order.order_id, order.status,
           &order.amount);

    ThreadArgs *args = (ThreadArgs *)malloc(sizeof(ThreadArgs));
    args->order = order;
    args->output_file = output_file;

    pthread_create(&threads[thread_count], NULL, process_order, (void *)args);
    thread_count++;
  }

  for (int i = 0; i < thread_count; i++) {
    pthread_join(threads[i], NULL);
  }

  fclose(file);
}

int main(int argc, char *argv[]) {
  const char *input_file = NULL;
  const char *output_file_name = "order_processing_log.txt";

  int opt;
  while ((opt = getopt(argc, argv, "i:o:")) != -1) {
    switch (opt) {
    case 'i':
      input_file = optarg;
      break;
    case 'o':
      output_file_name = optarg;
      break;
    default:
      printf("Usage: %s -i <input_file> [-o <output_file>]\n", argv[0]);
      return 1;
    }
  }

  if (input_file == NULL) {
    printf("Usage: %s -i <input_file> [-o <output_file>]\n", argv[0]);
    return 1;
  }

  FILE *output_file = fopen(output_file_name, "w");
  if (output_file == NULL) {
    printf("Failed to create output file.\n");
    return 1;
  }

  process_orders_file(input_file, output_file);

  fclose(output_file);

  return 0;
}