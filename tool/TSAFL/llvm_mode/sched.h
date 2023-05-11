#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#include <bits/types/FILE.h>
#include <stddef.h>
#endif

#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/syscall.h>
#include <unistd.h>
#ifdef __cplusplus
#include <atomic>
#endif
#include <errno.h>
#if defined(__APPLE__) || defined(__FreeBSD__) || defined(__OpenBSD__)
#include <sched.h>
#include <stdint.h>
#include <sys/types.h>
typedef int32_t __s32;
typedef uint32_t __u32;
typedef uint8_t __u8;
typedef uint16_t __u16;
typedef uint64_t __u64;
/* Scheduling algorithms */
#define SCHED_RR 2
#define SCHED_BATCH 3
#define SCHED_IDLE 5
#define SCHED_DEADLINE 6
#endif /* __APPLE__ || __FreeBSD__ || __OpenBSD__ */

#ifdef __linux__
#include <linux/sched.h>
#include <linux/types.h>
#define gettid() syscall(__NR_gettid)
#else
#if MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_12
uint64_t gettid() {
  uint64_t tid;
  pthread_threadid_np(NULL, &tid);
  return tid;
}
#endif
#endif

/* XXX use the proper syscall numbers */
#ifdef __x86_64__
#define __NR_sched_setattr 314
#define __NR_sched_getattr 315
#endif

#ifdef __i386__
#define __NR_sched_setattr 351
#define __NR_sched_getattr 352
#endif

#ifdef __arm__
#define __NR_sched_setattr 380
#define __NR_sched_getattr 381
#endif

#ifdef __aarch64__
#define __NR_sched_setattr 274
#define __NR_sched_getattr 275
#endif

struct sched_attr {
  __u32 size;

  __u32 sched_policy;
  __u64 sched_flags;

  /* SCHED_NORMAL, SCHED_BATCH */
  __s32 sched_nice;

  /* SCHED_FIFO, SCHED_RR */
  __u32 sched_priority;

  /* SCHED_DEADLINE (nsec) */
  __u64 sched_runtime;
  __u64 sched_deadline;
  __u64 sched_period;
};

int sched_setattr(pid_t pid, const struct sched_attr *attr,
                  unsigned int flags) {
  return syscall(__NR_sched_setattr, pid, attr, flags);
}

int sched_getattr(pid_t pid, struct sched_attr *attr, unsigned int size,
                  unsigned int flags) {
  return syscall(__NR_sched_getattr, pid, attr, size, flags);
}

int set_sched_dl(__u64 runtime, __u64 deadline, __u64 period) {
  /* set up sheduling parameters */
  struct sched_attr attr;
  int ret;
  unsigned int flags = 0;

  attr.size = sizeof(attr);
  attr.sched_flags = 0;
  attr.sched_nice = 0;
  attr.sched_priority = 0;

  // set up working intervals
  attr.sched_policy = SCHED_DEADLINE;
  attr.sched_runtime = runtime;
  attr.sched_deadline = deadline;
  attr.sched_period = period;

  // apply scheduling settings
  ret = sched_setattr(0, &attr, flags);
  if (ret < 0) {
    perror("sched_setattr error\n");
    printf("错误：%d\n", errno);
    // Use a file to remember the failed number.
    FILE *fp;
    fp = fopen("error_log.txt", "w");
    fprintf(fp, "sched_setattr error: %d\n", errno);
    fclose(fp);
    exit(-1);
  }
  return ret;
}

int set_sched_FIFO() {
  struct sched_attr attr;
  int ret;
  unsigned int flags = 0;

  attr.size = sizeof(attr);
  attr.sched_flags = 0;
  attr.sched_nice = 0;
  attr.sched_priority = 1;
  attr.sched_policy = SCHED_FIFO;
  attr.sched_runtime = 0;
  attr.sched_deadline = 0;
  attr.sched_period = 0;

  ret = sched_setattr(0, &attr, flags);
  if (ret < 0) {
    perror("sched_setattr error\n");
    printf("错误：%d\n", errno);
    // Use a file to remember the failed number.
    FILE *fp;
    fp = fopen("error_log.txt", "w");
    fprintf(fp, "sched_setattr error: %d\n", errno);
    fclose(fp);
    exit(-1);
  }

  return ret;
}
#ifdef __cplusplus
// static volatile sig_atomic_t tid_to_run;
std::atomic<int> tid_to_run;

void set_tid_to_run(int tid) { tid_to_run = tid; }

int get_tid_to_run() { return tid_to_run; }
#endif