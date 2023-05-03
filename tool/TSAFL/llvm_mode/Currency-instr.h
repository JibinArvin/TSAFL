#ifndef _INS_FUNCTION_
#define _INS_FUNCTION_

#include "../config.h"
#include "sched.h"
#include "types.h"
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/syscall.h>
#include <time.h>
#include <unistd.h>
#ifdef __linux__
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/types.h>
#endif
struct Kp_action_info {
  u32 threadid; // count_exec not real gettid()
  u32 loc;      // TODO: fit the real number i want.
};

#define NAMELENGTHMAX 100
#define SHARE_MEMORY_THREAD_NUM 100
#define SHARE_MEMORY_ARRAY_SIZE 4096
#define SHARE_MEMORY_ARRAY_SIZE_MAX 10240 /* Which seems too big */
#define SHARE_MEMORY_ARRAY_SIZE_SHORT 1024
#define MY_PTHREAD_CREATE_MAX 16
#define SAME_THREAD_CONSIDER_NUM 4
#define PERIOD_MAX 16
#define THREAD_NUMBER 4
#define THREAD_NUMBERT_SCH 2
// means we just consider four threads inlcuding main thread.
// HACK: need change. 256 may be too small
#define KEYPOINTNUMBER 1024 // temporarily use

/* real Info need be set */
struct Thread_info {
  int8_t is_scheduel;
  uint64_t time; // the execution time of the program (nsec)
  u64 N;         // number of threads concerned default equals to 4
  u64 final_thread_number;
  u32 test_flag1, /* Add two test_flag to check if the shared memory connected
                     in */
      test_flag2; /* right way. */

  /*  Get every action with thread count
      For example :
      kp_thread_array[1][30] : 1 means thread count, 30 means action whose
      number is 30
  */
  /* use -1 to color the array */
  int32_t mainid;
  /* Init as -1. */
  int32_t kp_thread_array[MY_PTHREAD_CREATE_MAX][SHARE_MEMORY_ARRAY_SIZE];
  /* Convert thread_count to tid */
  int64_t thread_count_to_tid[MY_PTHREAD_CREATE_MAX]; /* Init as -1. */
  /* Remember thread create and jion infomation */
  int64_t thread_create_jion[4 * MY_PTHREAD_CREATE_MAX]; /* Init as 0. */
  int32_t thread_main_eara[MY_PTHREAD_CREATE_MAX][2];

  // Reason for design in this way: for repeated actions not just in a loop for
  // more
  int32_t thread_same_period_size;
  int32_t thread_same_time[MY_PTHREAD_CREATE_MAX];
  int32_t thread_care[THREAD_NUMBERT_SCH]; /* Init as -1. */
  u16 kp_times[THREAD_NUMBERT_SCH][SHARE_MEMORY_ARRAY_SIZE];
  /* The data is keyPiont number for scheduel */
  u16 kp_action[THREAD_NUMBERT_SCH][SHARE_MEMORY_ARRAY_SIZE * 4];
  /* used to record the real action during scheduel(Just for assert!) */
  struct Kp_action_info kp_mem[SHARE_MEMORY_ARRAY_SIZE_MAX *
                               THREAD_NUMBERT_SCH * 2]; /* Init as 0.*/
  u64 kp_mem_size;                                      /* Init as zero. */
} __attribute__((aligned(32)));

#endif