#include <cstddef>
#include <ostream>
#include <string.h>
#include <unistd.h>
#define AFL_LLVM_PASS
#define TSF_OUT
#include "../config.h"
#include "../debug.h"
#include "../types.h"
#include "Currency-instr.h"
#include <bits/pthreadtypes.h>
#include <cassert>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <cxxabi.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <map>
#include <pthread.h>
#include <set>
#include <stdint.h>
#include <stdio.h>
#include <string>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#define OUT_DEBUG

using namespace std;
using namespace std::chrono;

// #define NDEBUG
#define likely_if(x) __builtin_expect(!!(x), 1)   // x true
#define unlikely_if(x) __builtin_expect(!!(x), 0) // x false

thread_local unsigned int preloc = 0x8888; /* Unlike like afl. */

pthread_spinlock_t thread_count_lock;
pthread_spinlock_t thread_create_lock;
pthread_spinlock_t thread_join_lock;
pthread_spinlock_t thread_create_join_lock;
pthread_spinlock_t thread_care_lock;
static volatile int count_exec = 0;
static volatile int thread_create_count = 0;
// static volatile int thread_jion_count = 0;
// real peroid number considering 0 this says that start form 0;

static bool Ischedule = false;

// static bool TupleScheduling = false;
<<<<<<< HEAD

=======
>>>>>>> 8a648cdbfaa7df1c0412c87833c06ce64bce1484
static int num_cpu = 0; // number of available CPUs
static int num_td = 0;  // number of threads concerned
static cpu_set_t mask;
// static cpu_set_t get;
<<<<<<< HEAD

=======
>>>>>>> 8a648cdbfaa7df1c0412c87833c06ce64bce1484
static uint64_t PSIEruntime = 1000 * 1000; // nsec
#ifdef MAIN_WAITING
static int main_sch_start;
#endif
static int mainTid;
static int main_create_number;

// traceEnd函数执行之前std::map就会释放资源，使用map的时候可能由于iter++导致出错，谨慎用
// std::map would release resource without ASAN
pthread_spinlock_t threadSelf_lock;
static std::map<unsigned long int, long int>
    threadSelf_to_pid_map; // the map from thread ID to tid
static std::map<unsigned long int, long int>::iterator T2Piter;
// the map to record the running action for each tid
static std::map<unsigned long int, int> tid_actionCount;
static std::map<unsigned long int, int>::iterator tid_actionCount_iter;

static std::map<long, int>
    tid_to_threadNumber; // from the  threadNumber to tid working

static int thread_create_join_number = 0;

// static system_clock::time_point startRun;
// static system_clock::time_point endRun;

#define THREAD_CARE 2
static std::unordered_map<u32, u32> thread_care;
static std::unordered_set<int32_t> thread_same_time;

struct Thread_info *t_info;
/* Used to describe  */ /* waiting! */
u32 thread_action_count[MY_PTHREAD_CREATE_MAX];

std::unordered_set<int64_t> thread_set;
pthread_spinlock_t thread_set_lock;

inline const char *get_funcname(const char *src) {
  int status = 99;
  const char *f = abi::__cxa_demangle(src, nullptr, nullptr, &status);
  return f == nullptr ? src : f;
}

/* WAITING...*/
static int get_add_count() {
  pthread_spin_lock(&thread_count_lock);
  int res = count_exec;
  count_exec++;
  pthread_spin_unlock(&thread_count_lock);
  return res;
}

/* WAITING...*/
static int get_add_thread_create_count() {
  pthread_spin_lock(&thread_create_lock);
  int res = thread_create_count;
  thread_create_count++;
  pthread_spin_unlock(&thread_create_lock);
  return res;
}

int DBDSdetectCPUs() {
  int ncpu;

  // Set default to 1 in case there is no auto-detect
  ncpu = 1;

// Autodetect the number of CPUs on a box, if available
#if defined(__APPLE__)
  size_t len = sizeof(ncpu);
  int mib[2];
  mib[0] = CTL_HW;
  mib[1] = HW_NCPU;
  if (sysctl(mib, 2, &ncpu, &len, 0, 0) < 0 || len != sizeof(ncpu))
    ncpu = 1;
#elif defined(_SC_NPROCESSORS_ONLN)
  ncpu = sysconf(_SC_NPROCESSORS_ONLN);
#elif defined(WIN32)
  SYSTEM_INFO si;
  GetSystemInfo(&si);
  ncpu = si.dwNumberOfProcessors;
#endif

  // Ensure we have at least one processor to use
  if (ncpu < 1)
    ncpu = 1;

  return ncpu;
}
void traceEnd(void);
void __attribute__((constructor)) traceBegin(void) {
  /*NOTE: How to solve the maintid loss? */
  /* Insert a instr_PthreadCall after main function, If you want to know more
   * check the passFile(may line 584!). */
  mainTid = gettid();
  tid_to_run = 0;
  main_create_number = 0;
#ifdef MAIN_WAITING
  main_sch_start = 0;
#endif

#ifdef OUT_DEBUG
  fprintf(stdout, "[OUT_DEBUG] traceBegin.\n");
#endif
  /* In case the traceEnd don't work! */
  atexit(traceEnd);
  pthread_spin_init(&thread_count_lock, PTHREAD_PROCESS_SHARED);
  pthread_spin_init(&thread_create_lock, PTHREAD_PROCESS_SHARED);
  pthread_spin_init(&thread_join_lock, PTHREAD_PROCESS_SHARED);
  pthread_spin_init(&thread_create_join_lock, PTHREAD_PROCESS_SHARED);
  pthread_spin_init(&thread_set_lock, PTHREAD_PROCESS_SHARED);
  pthread_spin_init(&threadSelf_lock, PTHREAD_PROCESS_SHARED);
  pthread_spin_init(&thread_care_lock, PTHREAD_PROCESS_SHARED);
#ifdef OUT_DEBUG
  fprintf(stdout, "[OUT_DEBUG] finish spinlock init.\n");
#endif
  /* init thread_action_count */
  memset(thread_action_count, 0, MY_PTHREAD_CREATE_MAX * sizeof(u32));

  /* Setting Thread_info part program. */
  int shmid_info;
  key_t key_info = ftok("/tmp", 111);
  if ((shmid_info = shmget(key_info, sizeof(struct Thread_info),
                           IPC_CREAT | 0666)) < 0) {
    fprintf(stderr, "Error: getting shared memory key_psie id: 1324\n");
  }
  if ((struct Thread_info *)-1 ==
      (t_info = (struct Thread_info *)shmat(shmid_info, NULL, 0))) {
    fprintf(stderr, "Error: attaching shared memory id.\n");
    exit(1);
  }
#ifdef NO_TSAFL
  t_info->flag_is_dry = 1;
#endif

  Ischedule = t_info->is_scheduel;
  TSF("Ischedule: %d", Ischedule);
  MEM_BARRIER();

#ifdef OUT_DEBUG
  fprintf(stdout, "[OUT_DEBUG] Ischedule state: %d.\n", Ischedule);
#endif
  if (!Ischedule) {
    t_info->time = 1000;
  }

  /* Init the part of shared memory. */
  t_info->final_thread_number = 0;
  t_info->mainid = -1; /* set as default value. */
  memset(t_info->thread_count_to_tid, -1,
         sizeof(int64_t) * MY_PTHREAD_CREATE_MAX);
  memset(t_info->kp_thread_array, -1,
         sizeof(int32_t) * MY_PTHREAD_CREATE_MAX * SHARE_MEMORY_ARRAY_SIZE);
  memset(t_info->thread_create_jion, -1,
         4 * MY_PTHREAD_CREATE_MAX * sizeof(int64_t));
  memset(t_info->kp_mem, 0,
         sizeof(struct Kp_action_info) * SHARE_MEMORY_ARRAY_SIZE_MAX *
             THREAD_NUMBERT_SCH * 2);
  memset(t_info->thread_main_eara, -1,
         MY_PTHREAD_CREATE_MAX * 2 * sizeof(int32_t));
  MEM_BARRIER();
#ifdef OUT_DEBUG
  TSF("Ischedule: %d", Ischedule);
#endif
  /* Special for Scheduel mode */
  if (Ischedule == true) {
#ifdef NUM_CPU
    num_cpu = NUM_CPU;
#else
    num_cpu = DBDSdetectCPUs();
#endif
    num_td = t_info->N;
    PSIEruntime = t_info->time;
    /* The deadline time max value is set by the */
    if (PSIEruntime > 1000000) {
      PSIEruntime = 1000000;
    }
#ifdef OUT_DEBUG
    TSF("---------Under Ischedule mode!------------");
#endif
  }
  /* Static variables（espical map） are initialized after trace traceBegin!*/

#ifdef OUT_DEBUG
  fprintf(stdout, "[OUT_DEBUG] traceBegin end.\n");
#endif

  if (Ischedule == true) {
    printf("cpus: %d\n", num_cpu);
    CPU_ZERO(&mask);
    CPU_SET(0, &mask);
  }
}

/* It is very dangerous to use global variables in the destructor function, and
 * it is very easy to get a null value to a unexpected result! */
void traceEnd(void) {

#ifdef DEBUG
  printf("[DEBUG] enter traceEnd\n");
#endif
  t_info->mainid = mainTid;

  // auto durationDryRun = duration_cast<microseconds>(endRun - startRun);
  // double DryRunTime = double(durationDryRun.count()) *
  //                     microseconds::period::num /
  //                     microseconds::period::den; // s为单位

  t_info->final_thread_number = get_add_thread_create_count();

  /* Add thread_count_to_tid */
  t_info->kp_mem_size = get_add_count();

  /* Add main thread end info here. */
  pthread_spin_lock(&thread_create_join_lock);
  t_info->thread_create_jion[thread_create_join_number] = -1 * mainTid;
  thread_create_join_number++;
  pthread_spin_unlock(&thread_create_join_lock);
  /* recover lost jion! */
  std::vector<int64_t> mm_unjioned_thread;
  pthread_spin_lock(&thread_create_join_lock);
  std::map<int64_t, size_t> mm;
  for (size_t i = 0; i < thread_create_join_number; i++) {
    auto number = t_info->thread_create_jion[i];
    if (number > 0) {
      if (mm.find(number) == mm.end())
        mm[number] = 0;
      mm[number]++;
    } else if (number < 0) {
      mm[-1 * number]--;
    }
  }
  for (auto item : mm) {
    if (item.second == 1) {
      mm_unjioned_thread.push_back(item.first);
      t_info->thread_create_jion[thread_create_join_number++] = -1 * item.first;
    }

    else if (item.second != 1 && item.second != 0)
      FATAL("item.f: %ld, item.second: %ld", item.first, item.second);
  }

  pthread_spin_unlock(&thread_create_join_lock);

  /* If a thread is unjioned, that means the t_info->thread_main_eara[i][1] is
  not filled which may lead to fail!*/
  /* This is meaningless! */

  /* Destory spin_lock */
  pthread_spin_destroy(&thread_count_lock);
  pthread_spin_destroy(&thread_create_lock);
  pthread_spin_destroy(&thread_join_lock);
  pthread_spin_destroy(&thread_set_lock);
  pthread_spin_destroy(&threadSelf_lock);
  pthread_spin_destroy(&thread_care_lock);
#ifdef OUT_DEBUG
  fprintf(stdout, "[OUT_DEBUG] traceEnd end.\n");
  fprintf(stdout, "[OUT_DEBUG] OUT INFO:\n thread_create_join,\n");
  for (int i = 0; i < thread_create_join_number; i++) {
    fprintf(stdout, "number: %ld.\n", t_info->thread_create_jion[i]);
  }
  fprintf(stdout, "thread_create_join END\nmyid to threadtid.\n");
  for (int i = 0; i < MY_PTHREAD_CREATE_MAX; i++) {
    if (t_info->thread_count_to_tid[i] == -1)
      break;
    printf("mytid: %d -> threadtid: %ld.\n", i, t_info->thread_count_to_tid[i]);
  }
  printf("myid to threadtid END.\n");

  for (size_t i = 0; i < t_info->final_thread_number; i++) {
    for (size_t j = 0; j < SHARE_MEMORY_ARRAY_SIZE; j++) {
      int32_t number = t_info->kp_thread_array[i][j];
      if (number == -1)
        break;
      TSF("threadNumber: %ld, count:%ld, loc:%d", i, j, number);
    }
  }
#endif
}

// to make sure which function had been called which is NOT NESECCERARY for
void instr_Call(void *func) {
#ifdef TRACE
  Dl_info info;
  const char *funcname = get_funcname(info.dli_sname);
  if (dladdr(func, &info) && fp_trace != NULL) {
    long int tid = gettid();
    fprintf(fp_trace, "Thread %ld (%lu) enter function: %s\n", tid,
            pthread_self(), funcname);
  }
#endif
}

void instr_Return(void *func) {
#ifdef TRACE
  Dl_info info;
  int loc = get_add_thread_jion_count();
  dry_s_info->thread_action[loc] = (-1) * (int64_t)gettid();
  if (dladdr(func, &info) && fp_trace != NULL) {
    fprintf(fp_trace, "Thread %ld (%lu) exit function: %s\n", gettid(),
            pthread_self(), get_funcname(info.dli_sname));
  }
#endif
}

inline void add_tid_to_threadNumber(long int tid) {
  pthread_spin_lock(&thread_create_lock);
  if (tid_to_threadNumber.find(tid) == tid_to_threadNumber.end()) {
    int res = thread_create_count;
    thread_create_count++;
    tid_to_threadNumber.insert({tid, res});
  }
  pthread_spin_unlock(&thread_create_lock);
}

void init_threadSelf(long int tid_temp) {
  pthread_spin_lock(&threadSelf_lock);
  if (threadSelf_to_pid_map.find(pthread_self()) !=
      threadSelf_to_pid_map.end()) {
    pthread_spin_unlock(&threadSelf_lock);
    return;
  }
  if (threadSelf_to_pid_map.find(pthread_self()) ==
      threadSelf_to_pid_map.end()) {
    threadSelf_to_pid_map.insert(
        pair<unsigned long, long>(pthread_self(), tid_temp));
  } else {
    threadSelf_to_pid_map[pthread_self()] = tid_temp;
  }
  pthread_spin_unlock(&threadSelf_lock);
}

void init_thread_info(long int tid_temp) {

  if (Ischedule == true) {
    pthread_spin_lock(&thread_care_lock);
    if (thread_care.size() == 0) {
#ifdef OUT_DEBUG
      TSF("Now it's the time init thread_care!");
#endif
      for (size_t i = 0; i < THREAD_CARE; i++) {
        thread_care[t_info->thread_care[i]] = i;
      }
    }
    t_info->test_flag1 = 1;
    pthread_spin_unlock(&thread_care_lock);
  }

  if (thread_set.count(tid_temp) != 0)
    return;

  pthread_spin_lock(&thread_set_lock);
  if (thread_set.count(tid_temp) == 0) {
    thread_set.insert(tid_temp);
  }
  pthread_spin_unlock(&thread_set_lock);

  // set the threadnumber by tid tp mytid
  add_tid_to_threadNumber(tid_temp);
<<<<<<< HEAD

=======
>>>>>>> 8a648cdbfaa7df1c0412c87833c06ce64bce1484
  int thread_number =
      tid_to_threadNumber[tid_temp]; // The map from std make sure the
                                     // read action is thread-safety.
  t_info->thread_count_to_tid[thread_number] = tid_temp; // No range check here!

  if (unlikely(tid_temp == mainTid)) {
    main_create_number = thread_number;
  }
<<<<<<< HEAD
=======

>>>>>>> 8a648cdbfaa7df1c0412c87833c06ce64bce1484
  // add create info into t_info
  pthread_spin_lock(&thread_create_join_lock);
  t_info->thread_create_jion[thread_create_join_number] = tid_temp;
  thread_create_join_number++;
  pthread_spin_unlock(&thread_create_join_lock);
<<<<<<< HEAD
=======

>>>>>>> 8a648cdbfaa7df1c0412c87833c06ce64bce1484
  if (Ischedule && thread_care.find(main_create_number) == thread_care.end() &&
      thread_care.find(thread_number) != thread_care.end()) {
    tid_to_run++;
#ifdef OUT_DEBUG
    TSF("main_create_number: %d", main_create_number);
    for (auto item : thread_care) {
      std::cout << "thread_care: " << item.first << "\n";
    }
#endif
    while (tid_to_run < 2)
      ;
  }

  if (Ischedule && thread_care.find(tid_temp) != thread_care.end()) {
<<<<<<< HEAD
=======

>>>>>>> 8a648cdbfaa7df1c0412c87833c06ce64bce1484
#ifdef OUT_DEBUG
    TSF("Seting PSIEruntime for mythreadid: %ld", tid_temp);
#endif
    set_sched_FIFO();
    usleep(1000); /* Make sure the setting work!*/
  }

  if (Ischedule && thread_care.find(tid_temp) != thread_care.end()) {
<<<<<<< HEAD

=======
>>>>>>> 8a648cdbfaa7df1c0412c87833c06ce64bce1484
    if (sched_setaffinity(0, sizeof(mask), &mask) == -1) {
      FILE *fp = fopen("sch_log.txt", "w");
      fprintf(fp, "Set CPU affinity failue, ERROR:%s\n", strerror(errno));
      printf("Set CPU affinity failue, ERROR:%s\n", strerror(errno));
      fclose(fp);
    }
  }

  MEM_BARRIER();

  if (tid_temp != mainTid) {
    int main_thread_number = tid_to_threadNumber[mainTid];
    int count_main = thread_action_count[main_thread_number];
    t_info->thread_main_eara[thread_number][0] = count_main;
  }
}

void instr_PthreadCall(void *func) // pthread_create中调用的那个函数
{
  return;
}

void instr_pthread_create(unsigned long int *threadId, void *func) {

  Dl_info info;
  long int SavePID;
  if (threadSelf_to_pid_map.find(*threadId) != threadSelf_to_pid_map.end())
    SavePID = threadSelf_to_pid_map[*threadId];
  else
    SavePID = 0;

#ifdef OUT_DEBUG
  if (dladdr(func, &info)) {
    fprintf(stdout, "[OUT_DEBUG] Thread %ld (%lu) execute pthread_create(%s)\n",
            SavePID, *threadId, get_funcname(info.dli_sname));
  }
#endif
}

void instr_pthread_join(unsigned long int threadId) {
  if (threadSelf_to_pid_map.find(threadId) == threadSelf_to_pid_map.end())
    FATAL("Can't find threadId in threadSelf_map");
  int tid = threadSelf_to_pid_map.at(threadId);
  pthread_spin_lock(&thread_create_join_lock);
  t_info->thread_create_jion[thread_create_join_number] = -1 * tid;
  thread_create_join_number++;
  pthread_spin_unlock(&thread_create_join_lock);

  if (tid != mainTid) {
    int threadNumber;
    add_tid_to_threadNumber(tid);
    threadNumber = tid_to_threadNumber[tid];
    /* NO check for this! */
    int main_thread_number = tid_to_threadNumber[mainTid];
    int count_main = thread_action_count[main_thread_number];
    t_info->thread_main_eara[threadNumber][1] = count_main;
  }

#ifdef OUT_DEBUG
  TSF("---jion tid: %d", tid);
  TSF("---jion thread_create_join_number: %d", thread_create_join_number);
  fprintf(stdout, "[DEBUG] instr_pthread_join-> threadId: %ld.\n", threadId);
  fprintf(stdout, "[DEBUG] Thread %ld (%lu) execute pthread_join()\n",
          threadSelf_to_pid_map[threadId], threadId);
#endif
}

/*The LOC function mainly provides the record of thread interleaving in the
 * scheduling situation. It should be emphasized that the LOC is always after
 * the LOC_string according to the insertion order, which can reflect whether
 * the scheduling scheme can be carried out according to the actual plan. The
 * original plan was to insert the LOC function after the expected function, but
 * it is easy to encounter the problem of inserting blank lines, which is
 * difficult to achieve. BUT actually now it's reseanable make instr_loc and
 * instr_loc_string as one...emmm.....emmm....*/
/**
 * @brief just log the kp may exit. When program is scheduling,
 * used to remember the real progress sequence.
 */
void instr_LOC(void *func, unsigned int a, unsigned int loc) {

  // fprintf(stdout, "[DEBUG] instr_LOC-> loc: %d .\n", loc);
  unsigned long tid = gettid();
  init_thread_info(tid);
  init_threadSelf(tid);
  int threadNumber;
  // only for tracefile no other use
#ifdef TRACE
  if (mapTid.find(tid) != mapTid.end()) {
    mapTid[tid]++;
  } else {
    mapTid.insert(pair<long int, unsigned int>(tid, 1));
  }
#endif
  add_tid_to_threadNumber(tid);
  threadNumber = tid_to_threadNumber[tid];
// TODO: add dry action
#ifdef OUT_DEBUG
  Dl_info info;
  if (dladdr(func, &info)) {
    fprintf(stdout, "[Debug] Tid: %ld, Function: %s, KeyPoint: %u\n", gettid(),
            get_funcname(info.dli_sname), a);
  }
#endif
  if (Ischedule == true) {
    int count = thread_action_count[threadNumber]; /* array init as 0 */
    if ((count > 2 && loc == t_info->kp_thread_array[threadNumber][count - 1] &&
         t_info->kp_thread_array[threadNumber][count - 2] ==
             t_info->kp_thread_array[threadNumber][count - 1])) {
      return;
    }
    if (thread_care.find(threadNumber) == thread_care.end()) {
      return;
    }
    pthread_spin_lock(&thread_count_lock);
    int number_action = count_exec;
    /* skip the */ /* But need change. */
    t_info->kp_mem[number_action] = {(u32)threadNumber, (u16)loc};
    count_exec++;
    pthread_spin_unlock(&thread_count_lock);
  }
}

//
/**
 * @brief First record the current execution status, and then perform scheduling
 * behavior according to the prompt of the kp array. It should be noted that
 * such behaviors are filtered.
 * @param func
 * @param intHash
 * @param loc_single_period
 */
void instr_LOC_string(void *func, unsigned int loc) {
#ifdef OUT_DEBUG
  fprintf(stdout, "[DEBUG] instr_LOC_string-> loc: %d .\n", loc);
#endif
  unsigned long tid = gettid();
  int threadNumber;
  init_thread_info(tid);
  add_tid_to_threadNumber(tid);
  threadNumber = tid_to_threadNumber[tid];

  int count = thread_action_count[threadNumber]; /* array init as 0 */

  if ((count > 2 && loc == t_info->kp_thread_array[threadNumber][count - 1] &&
       t_info->kp_thread_array[threadNumber][count - 2] ==
           t_info->kp_thread_array[threadNumber][count - 1])) {
#ifdef OUT_DEBUG
    TSF("Find the loop to many times! now it's skip time!");
    TSF("How about the counter? counter: %d", count);
#endif
    return;
  }
  thread_action_count[threadNumber]++;
  t_info->kp_thread_array[threadNumber][count] = (int32_t)loc;
  // TSF("----------loc: %d-------------------", loc);

  /* A real interesting fact is the thread set as schd_deadline is not allowed
   * by linux kernel team to use fork or pthread_create! WHY!!! This confused me
   * a lot!*/

  /* I'm sry!, we just give up the main thread, which will also be mentioned in
   * paper i guss.*/

  /* The reason is also real simple, other way can't feed us! The SCHED_FIFO
   * SCHED_RR... Can't be the chioce! */

  if (Ischedule == true) {
#ifdef NDEBUG
    /* Disabel check part temporarily.*/
    u16 loc_temp = t_info->kp_action[threadNumber][count];
    // assert(loc_temp == (u16)loc);
#endif
    int32_t kp_time_loc = thread_care.find(threadNumber) != thread_care.end()
                              ? thread_care[threadNumber]
                              : -1;
    if (kp_time_loc == -1)
      return;

    if (kp_time_loc > THREAD_CARE) {
      FATAL("[Error]: Can't find the right number for  threadNumber: %d "
            ",kp_time_loc: %d",
            threadNumber, kp_time_loc);
    }
    int yield_times = t_info->kp_times[kp_time_loc][count];
#ifdef MAIN_WAITING
    if (threadNumber == mainTid && !main_sch_start) {
      if (yield_times > 0) {
        usleep(1000); // Make sure that the main thread will wait for others.
        main_sch_start = 1;
      }
    }
#endif
    while (1) {
      // TSF("Here facing mytid:%d, count:%d, and yield_times: %d", kp_time_loc,
      //     count, yield_times);
      if (yield_times <= 0) {
        break;
      }
      /* minus before the sched_yied */
      yield_times--;
      sched_yield();
    }
  }
}