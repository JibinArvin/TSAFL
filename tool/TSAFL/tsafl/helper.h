#pragma once

#ifndef __AFL_HELPER_H__
#define __AFL_HELPER_H__

#include "../llvm_mode/Currency-instr.h"
#include "../readxml.h"
#include "../types.h"
#include <stddef.h>
#include <stdint.h>
#define AFL_LLVM_PASS
#define MAX_TARGET_SP 8

struct queue_entry {

  u8 *fname; /* File name for the test case      */
  u32 len;   /* Input length                     */

  u8 cal_failed,    /* Calibration failed?              */
      trim_done,    /* Trimmed?                         */
      passed_det,   /* Deterministic stages passed?     */
      has_new_cov,  /* Triggers new coverage?           */
      var_behavior, /* Variable behavior?               */
      favored,      /* Currently favored?               */
      fs_redundant; /* Marked as redundant in the fs?   */
  u8 tr_intersting;
  u32 bitmap_size, /* Number of bits set in bitmap     */
      exec_cksum,  /* Checksum of the execution trace  */
      fuzz_level;  /* Number of fuzz                   */

  u64 exec_us,  /* Execution time (us)              */
      handicap, /* Number of queue cycles behind    */
      depth;    /* Path depth                       */

  u8 *trace_mini; /* Trace bytes, if kept             */
  u32 tc_ref;     /* Trace bytes ref count            */

  struct queue_entry *next, /* Next element, if any             */
      *next_100;            /* 100 elements ahead               */
  /* JIBIN add. */
  u32 sch_interesting;
  u8 schedule_var_be; /* Work as before schedule?       */
  u64 exec_time;      /* Time has been fuzzing iterate. */
  u32 sch_exec_cksum;
  u32 sch_exec_size;
};

struct single_info_token {
  unsigned int start; /* start and end represent the thread period
                         indentification just used in helper...*/
  unsigned int end;
  unsigned int action_size;
  unsigned int tid;
  int32_t *actions;
  int8_t is_main;
  int32_t to_main[2];
  /* This array with only two elements represents the operating
                       range of the current thread corresponding to the main
                       thread.*/
};

struct thread_info_scheduel_token {
  size_t size;
  struct single_info_token list[];
};

/**/
struct thread_need_care {
  size_t size;
  int64_t tid_list[];
};

#define CONSIDER_THREAD_NUMBER_HELPER 2
struct scheduel_result {
  /* just include the interesting location. */
  int64_t thread_same_period[MY_PTHREAD_CREATE_MAX];
  u_int64_t result_seq[CONSIDER_THREAD_NUMBER_HELPER];
  u32 entry_size[CONSIDER_THREAD_NUMBER_HELPER];
  u32 stop_times[];
};

struct scheduel_result_list {
  u64 size;
  struct scheduel_result *list[];
};

/* INFO you need for single time. */
struct single_t_info {
  int16_t thread_number;
  int32_t *thread_s_number;
};

struct cfg_info_token {
  u32 size;
  u_int16_t cksums[];
};

static struct queue_entry *queue, /* Fuzzing queue (linked list)      */
    *queue_cur,                   /* Current offset within the queue  */
    *queue_top,                   /* Top of the list                  */
    *q_prev100;                   /* Previous 100 marker              */

static u8 state_energy;     /* state determine which kind of power mode! */
static u32 init_cfg_number, /* the cfg index finished by init queue entry.    */
    init_window;            /* the window index finished by init queue entry. */
static u64 whole_running_time = 86400; /* 24H */

static u16 target_splice_ok;
static u64 que_meaningful_size;
static u64 que_meaningful_size_old;
static int8_t Test_lock = 0; /* Actually it born as 0*/

#ifdef __cplusplus
extern "C" {
#endif
/* init g_action_info, g_filter_info. */
void init_helper(struct function_array *f_array, struct variable_array *v_array,
                 struct distance_array *d_array,
                 struct groupKey_array *g_array);

void build_g_actionInfo(struct function_array *f_array,
                        struct variable_array *v_array,
                        struct distance_array *d_array,
                        struct groupKey_array *g_array);
void test_helper();
u32 get_max_count_action();
void build_g_filter(int64_t number);
void build_action_info(const struct variable_array *v_array);
void build_g_filter_ctc();
void helper_free_scheduel_result_list(struct scheduel_result_list *space);
struct thread_need_care *get_t_n_c(struct Thread_info *t_info,
                                   struct single_t_info *s_t);
struct single_t_info *get_s_t(struct Thread_info *t_info);
struct thread_info_scheduel_token *
get_tis_token(const struct Thread_info *const t_info,
              struct single_t_info *s_t);
void free_s_t_i(struct single_t_info *f);
void free_thread_info_scheduel_token(struct thread_info_scheduel_token *f);
u8 check_t_info(struct Thread_info *t_info);
void generate_que(struct scheduel_result_list **result_ptr,
                  struct thread_info_scheduel_token *token,
                  struct thread_need_care *t_n_c);
void build_g_actionInfo(struct function_array *f_array,
                        struct variable_array *v_array,
                        struct distance_array *d_array,
                        struct groupKey_array *g_array);
/* return 1 means just as plan! */
int8_t check_plan_result(struct Thread_info *info, struct scheduel_result *plan,
                         size_t number);
int8_t if_good_plan_now(size_t number);
void finish_one_plan_success(size_t number, struct Thread_info *t_info,
                             size_t kp_size, struct thread_need_care *t_care);
void finish_one_scheduel_run(struct cfg_info_token *cfg_token);
int8_t store_info_preSchedule(struct Thread_info *t_info);
int8_t check_after_schedule(struct Thread_info *t_info);
struct cfg_info_token *generate_cfg_token(struct thread_info_scheduel_token *t);
struct cfg_info_token *
generate_finished_cfg(struct thread_info_scheduel_token *t);
void put_cfg_token_toG(struct cfg_info_token *cfg_token);
void push_finish_cfg_toG(struct cfg_info_token *cfg_token);
int8_t update_q_cfg(struct queue_entry *q);
int8_t update_q_window(struct queue_entry *q);
int8_t finish_cfg(u16 ck_sum);
int8_t is_finished_cfg(u16 ck_sum);
int8_t minus_one_q(struct queue_entry *q);
// int8_t clean_que(struct queue_entry *que); /* This is useless right now,
// which is replaced by new cull_que. */
void after_trim_q(struct queue_entry *q, u8 *trace_bit, int8_t live);
void set_init_cAndW();
float calculate_score_factor_sch(struct queue_entry *q);
u32 get_target_splicing_limit();
int64_t get_splicing_targets(u32 targets_size, struct queue_entry **targets);
void fill_que_sch_exeInfo(struct Thread_info *t_info, struct queue_entry *q);
u8 have_new_concurrent_per(struct queue_entry *q);
void add_to_queEntry_sInfo_map(struct queue_entry *q);
void update_new_scheQ_to_map(struct queue_entry *q_in,
                             struct Thread_info *t_info);
struct queue_entry *get_same_sch_exe_q(struct queue_entry *q_in);
int8_t update_queEntry_sInfo(struct queue_entry *q,
                             struct cfg_info_token *cfg_tInfo_token);
int8_t update_queEntry_sInfo_with_another(struct queue_entry *q,
                                          struct queue_entry *another);
int8_t set_queueEntry_interesting_WandC();
void fill_aflTraceMap(struct queue_entry *q, u8 *trace_bits);
void init_q_infos(struct queue_entry *q);
void add_qPtr_with_execksum_count(struct queue_entry *q, u32 exec_cksum,
                                  u32 count);
void update_q_exe_time(struct queue_entry *q);
#ifdef __cplusplus
}
#endif

#endif