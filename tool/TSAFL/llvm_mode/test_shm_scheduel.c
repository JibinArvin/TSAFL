/* This file is a really simple test file, which contained two different mode
 * fitting test.c under same dir. There are two mode including test_basic and
 * test_while right now. For test_while mode, the first check piont of t1 should
 * be after than the last one of t2.(A real simple test!! >_<).
 */

#include <stddef.h>
#include <stdint.h>
#define AFL_MAIN
#define MESSAGES_TO_STDOUT

#define __USE_GNU /* It have been changed. */
#define _FILE_OFFSET_BITS 64
#define TSF_OUT

#include "../alloc-inl.h"
#include "../config.h"
#include "../debug.h"
#include "../hash.h"
#include "../types.h"

#include <ctype.h>
#include <dirent.h>
#include <dlfcn.h>
#include <errno.h>
#include <fcntl.h>

#include <sched.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>

/* added by dongjibin */
#include "limits.h"
/*wrong way to include*/ // TODO:
#include "../llvm_mode/Currency-instr.h"

static s32 shm_id; /* ID of the SHM region             */
u8 *trace_bits;    /* SHM with instrumentation bitmap  */
/* JIBIN Vars add. */
static s32 t_info_id;
static struct Thread_info *t_info;
static u8 schedule_var;
static u8 need_scheduel; /* For run_target check if need scheduel? */
static u64 time_cost_run_target_single;

void init_tInfo_mem(struct Thread_info *t_info) {
  char *need_sch = getenv("NEED_SCH");
  if (need_sch == NULL) {
    // FATAL("Can't find NEED_SCH envoriment!");
    TSF("This time run without sch!");
    t_info->is_scheduel = false;
  }
  if (need_sch && !strcmp(need_sch, "TRUE")) {
    t_info->is_scheduel = true;
  }

  /* The tested program will init the part info store.*/
  /* Init sche-part is meaningless while */
}

static inline void enable_schedule() {
  if (need_scheduel == 1) {
    SAYF("[ERROR] Before set, 'need_scheduel' is true!\n");
  }
  need_scheduel = 1;
  setenv("NEED_SCH", "TRUE", 1);
  char *need_sch = getenv("NEED_SCH");
  if (!need_sch) {
    FATAL("Can't find the NEED_SCH!");
  } else {
    TSF("Find NEED_SCH: %s!", need_sch);
  }
}

static inline void disable_schedule() {
  if (need_scheduel == 0) {
    fprintf(stderr, "[ERROR] Before reset, 'need_scheduel' is false!\n");
  }
  setenv("NEED_SCH", "FALSE", 1);
}

static inline void remove_shm_t_info() {
  shmctl(t_info_id, IPC_RMID, NULL);
  TSF("clean shared t_info memory.");
}

void setup_shm_t_info(void) {
  fprintf(stdout, "SETTING UP SHM T INFO!\n");
  u8 *t_info_str;
  key_t key_info = ftok("/tmp", 111);
  t_info_id = shmget(key_info, sizeof(struct Thread_info), IPC_CREAT | 0666);
  if (t_info_str < 0) {
    PFATAL("t_info shmget() build failed");
  }
  atexit(remove_shm_t_info);
  t_info_str = alloc_printf("%d", t_info_id);

  setenv("THREAD_INFO_VAR", (char *)t_info_str, 1);

  ck_free(t_info_str);

  t_info = shmat(t_info_id, NULL, 0);

  if (!t_info) {
    PFATAL("t_info shmat() failed");
  }

  SAYF("Set t_info shared_memory successful!\n");
}

static void remove_shm(void) {
  shmctl(shm_id, IPC_RMID, NULL);
  TSF("clean shared t_info memory.");
}

void setup_shm(void) {
  u8 *shm_str;
  key_t shm_trace_Key = ftok("/tmp", 11);
  if (-1 == shm_trace_Key) {
    printf("ERROR: ftok faield, %s.\n", strerror(errno));
    exit(-1);
  }
  shm_id = shmget(shm_trace_Key, MAP_SIZE, IPC_CREAT | 0666);

  if (shm_id < 0)
    PFATAL("shmget() failed");

  atexit(remove_shm);

  shm_str = alloc_printf("%d", shm_id);
  setenv(SHM_ENV_VAR, (char *)shm_str, 1);

  ck_free(shm_str);

  trace_bits = shmat(shm_id, NULL, 0);
  setup_shm_t_info();
}

void init_t_info_as_test_shm_con() {
  t_info->test_flag1 = 1;
  t_info->test_flag2 = 100;
}

#define TEST_WHILE
void init_t_info_as_test_shm_sch1() {
  t_info->thread_care[0] = 1;
  t_info->thread_care[0] = 2;
  memset(t_info->kp_times, 0,
         sizeof(u16) * MY_PTHREAD_CREATE_MAX * SHARE_MEMORY_ARRAY_SIZE);
  memset(t_info->kp_action, 0,
         sizeof(u16) * MY_PTHREAD_CREATE_MAX * SHARE_MEMORY_ARRAY_SIZE);
  memset(t_info->kp_mem, 0,
         sizeof(struct Kp_action_info) * SHARE_MEMORY_ARRAY_SIZE_MAX);
#ifdef TEST_BASIC
  t_info->kp_times[0][0] = 1;
  t_info->kp_times[1][1] = 1;
#endif

#ifdef TEST_WHILE
  t_info->time = 10000000;
  t_info->thread_care[0] = 1;
  t_info->thread_care[1] = 2;

  t_info->kp_times[0][0] = 0;
  t_info->kp_times[1][0] = 1;
  // for (size_t i = 0; i < 200; i++) {
  //   t_info->kp_times[1][i] = 1;
  // }
#endif
}

int main() {
  setup_shm();
  enable_schedule();
  init_t_info_as_test_shm_sch1();
  init_tInfo_mem(t_info);
  // init_t_info_as_test_shm_con();
  sleep(100);
  disable_schedule();
  return 1;
}