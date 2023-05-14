#include "helper.h"
#include "config.h"
#include "debug.h"
#include "llvm_mode/Currency-instr.h"
#include "readxml.h"
#include "types.h"
#include <algorithm>
#include <assert.h>
#include <boost/container_hash/hash_fwd.hpp>
#include <boost/functional/hash.hpp>
#include <cassert>
#include <cerrno>
#include <cmath>
#include <complex>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <fstream>
#include <functional>
#include <iostream>
#include <iterator>
#include <map>
#include <memory>
#include <ostream>
#include <queue>
#include <set>
#include <sstream>
#include <stack>
#include <stddef.h>
#include <stdexcept>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <sys/types.h>
#include <system_error>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace {
#define OUT_DEBUG
#define ASSERT_DEBUG
using CK_SUM = u_int16_t;
using CK_SUM_32 = u_int32_t;
using CK_SUM_CFG = CK_SUM;
using VAR_TYPE = size_t;
using ACTION_NUMBER = size_t;
using OP_TYPE = size_t;
using tid_ac_v = std::vector<std::pair<size_t, OP_TYPE>>;
using PrefixSumHash =
    std::pair<std::pair<CK_SUM, size_t>, std::pair<CK_SUM, size_t>>;
template <typename ELEMENT> using double_set = std::set<std::set<ELEMENT>>;
using ck_size = std::pair<CK_SUM, size_t>;
using cks_un_set = std::unordered_set<ck_size>;
using cks_set = std::set<ck_size>;
using cks_ptr = std::shared_ptr<std::set<std::pair<CK_SUM, size_t>>>;
void clear_helper();
void used_finished(size_t number);

// NEXT: plan analysis program input

class WindowSearchMap;
class Working_info;
class SingleThread;

/* C(m+n, m, n)*/
u128 ride_by_self(int in_int) {
  if (in_int == 1)
    return in_int;
  u128 res = ride_by_self(in_int - 1) * in_int;
  if (res > 2000)
    return (2000);
  return res;
}

u128 factorial(int m, int n) {
  return ride_by_self(m + n) / (ride_by_self(m) * ride_by_self(n));
}

/* R(X) in help.cc*/
inline static int32_t Rand(u16 core) { return rand() % core; }

/* NOTION: the set is insidely Ordered*/
u32 hash32_set(std::set<CK_SUM> ck_set) {
  /* NOTE: hash_range is sensitive to the order of the elements so it wouldn't
   * be appropriate to use this with an unordered container.*/
  return boost::hash_range(ck_set.begin(), ck_set.end());
}

class WindowSearchMap {
  using search_result = bool;
  using search_map_key = std::unordered_set<OP_TYPE>;

public:
  search_result search_by_pair(std::pair<size_t, size_t> in, OP_TYPE op);
  search_result search_by_pair_v(std::pair<size_t, size_t> in,
                                 const std::vector<OP_TYPE> &ops);
  void add_element(const std::pair<size_t, size_t> &terminal, OP_TYPE op);
  WindowSearchMap() : out_map({}), store_map_v({}){};
  void clear();

private:
  std::unordered_map<OP_TYPE, size_t> out_map;
  std::vector<std::map<OP_TYPE, search_map_key>> store_map_v;
};

WindowSearchMap::search_result
WindowSearchMap::search_by_pair(std::pair<size_t, size_t> in, OP_TYPE op) {
  if (out_map.find(in.first) != out_map.end()) {
    size_t loc = out_map[in.first];
    if (store_map_v[loc].find(in.second) != store_map_v[loc].end()) {
      auto &search_item = store_map_v[loc][in.second];
      if (search_item.find(op) != search_item.end())
        return true;
    }
  }
  return false;
}

WindowSearchMap::search_result
WindowSearchMap::search_by_pair_v(std::pair<size_t, size_t> in,
                                  const std::vector<OP_TYPE> &ops) {
  if (out_map.find(in.first) != out_map.end()) {
    size_t loc = out_map[in.first];
    if (store_map_v[loc].find(in.second) != store_map_v[loc].end()) {
      auto &search_item = store_map_v[loc][in.second];
      for (const auto &op : ops)
        if (search_item.find(op) != search_item.end())
          return true;
    }
  }
  return false;
}

void WindowSearchMap::add_element(const std::pair<size_t, size_t> &terminal,
                                  OP_TYPE op) {
  if (out_map.find(terminal.first) == out_map.end()) {
    store_map_v.push_back({});
    out_map.insert({terminal.first, store_map_v.size() - 1});
  }
  size_t loc = out_map[terminal.first];
  if (store_map_v[loc].find(terminal.second) == store_map_v[loc].end()) {
    store_map_v[loc].insert({terminal.second, {}});
  }
  auto set = store_map_v[loc];
  store_map_v[loc][terminal.second].insert(op);
  return;
}

void WindowSearchMap::clear() {
  out_map.clear();
  store_map_v.clear();
}

class Que_entry_cpp {
public:
  struct queue_entry *q_entry_ptr;
  bool meaningful;
  size_t windows_size;
  size_t cfg_size;
  size_t number;
  cks_set windows_cks;
  cks_set retire_windows;
  cks_set cfg_cks;
  cks_set retire_cfg;
  std::set<CK_SUM> trace_mem;
  Que_entry_cpp()
      : q_entry_ptr(nullptr), meaningful(false), windows_cks({}), cfg_cks({}),
        number(INT32_MAX), windows_size(0), cfg_size(0){};
  size_t get_wSet_size() { return windows_cks.size(); };
  size_t get_cSet_size() { return cfg_cks.size(); };
  void add_w_cks(ck_size a) { windows_cks.insert(a); };
  void add_c_cks(ck_size a) { cfg_cks.insert(a); };
  void add_trace(ck_size a) { trace_mem.insert(a.first); };
  void retire_w(ck_size a);
  void retire_c(ck_size a);
  void finish_c(ck_size a);
};

void Que_entry_cpp::finish_c(ck_size a) {
  cfg_cks.erase(a);
  cfg_size--;
}

void Que_entry_cpp::retire_w(ck_size a) {
  windows_cks.erase(a);
  windows_size--;
  retire_windows.insert(a);
}

void Que_entry_cpp::retire_c(ck_size a) {
  cfg_cks.erase(a);
  cfg_size--;
  retire_cfg.insert(a);
}

std::shared_ptr<Que_entry_cpp> init_que_entry(struct queue_entry *q_entry_ptr,
                                              size_t number) {
  std::shared_ptr<Que_entry_cpp> res = std::make_shared<Que_entry_cpp>();
  res->q_entry_ptr = q_entry_ptr;
  res->number = number;
  res->meaningful = true;
  return res;
}

static std::shared_ptr<Que_entry_cpp> useless_que_entry_cpp =
    std::shared_ptr<Que_entry_cpp>();

using Cks_to_qEntryPtrN = std::map<ck_size, size_t>;
using Cks_to_qEntryPtr = std::map<ck_size, struct queue_entry *>;
using Cks32_to_qEntryPtr =
    std::map<std::pair<CK_SUM_32, size_t>, struct queue_entry *>;
using Cks_to_setQEntryPtr =
    std::map<std::pair<CK_SUM, size_t>, std::set<struct queue_entry *>>;

using Ck_to_setQEntryPtr = std::map<CK_SUM, std::set<struct queue_entry *>>;

class FilterInfo {
public:
  std::unordered_map<size_t, CK_SUM> count_to_ckSum;
  std::map<std::pair<CK_SUM, size_t>, size_t> ck_sum_map;
  WindowSearchMap close_map;
  std::set<std::pair<CK_SUM_32, size_t>>
      con_ck_sum_set; /* Just like virgin bits.*/
  std::map<size_t, std::shared_ptr<Que_entry_cpp>> q_entry_info_map;
  Cks_to_qEntryPtr cks_q_w;
  Cks_to_qEntryPtr cks_q_c;
  Cks32_to_qEntryPtr exesum_q;
  Cks_to_setQEntryPtr q_w_mem; /* Use to remmeber how many seed shared one ck.*/
  Ck_to_setQEntryPtr q_t_mem;
  Ck_to_setQEntryPtr q_c_mem;
  std::set<CK_SUM> finish_cfg;
  u64 exe_times;
  FilterInfo()
      : count_to_ckSum({}), ck_sum_map({}), close_map({}), con_ck_sum_set({}),
        q_entry_info_map({}), cks_q_w({}), cks_q_c({}), exesum_q({}),
        exe_times(0), q_w_mem({}), q_c_mem({}), q_t_mem({}) {}
  inline CK_SUM get_cksum(size_t n);
  bool is_interesting_ckSum(CK_SUM ck_sum_in, size_t offset);
  void add_count_to_ckSum(size_t a, CK_SUM b) { count_to_ckSum.insert({a, b}); }

  void add_close_map_by_pair(const std::pair<size_t, size_t> &terminal,
                             OP_TYPE op) {
    close_map.add_element(terminal, op);
  }

  bool search_close_map_by_pair(std::pair<size_t, size_t> in, OP_TYPE op) {
    return close_map.search_by_pair(in, op);
  };

  bool search_close_map_by_pair_v(std::pair<size_t, size_t> in,
                                  std::vector<OP_TYPE> &ops) {
    return close_map.search_by_pair_v(in, ops);
  }

  void add_element_to_ck_sum_map(CK_SUM ck_sum_in, size_t len);

  void dump_ctc_info();

  void clear();

  bool search_con_set(std::pair<CK_SUM_32, size_t> &element) {
    return con_ck_sum_set.find(element) == con_ck_sum_set.end() ? false : true;
  }

  bool search_con_set_or_add(std::pair<CK_SUM_32, size_t> &element);

  static CK_SUM hash_two(CK_SUM first, CK_SUM second);

  CK_SUM get_cksum_by_two(ACTION_NUMBER a, ACTION_NUMBER b);

  void add_q_entry(size_t loc, std::shared_ptr<Que_entry_cpp> ptr);

  CK_SUM get_ck_sum_by_two_131(ACTION_NUMBER a, ACTION_NUMBER b);

  std::shared_ptr<Que_entry_cpp> search_qEntry_info(size_t number);

  struct queue_entry *search_q_by_w(ck_size cks);

  struct queue_entry *search_q_by_c(ck_size cks);

  bool find_q_by_w(ck_size cks);

  bool find_q_by_c(ck_size cks);

  void set_w_cks(struct queue_entry *q, ck_size ck);

  void set_c_cks(struct queue_entry *q, ck_size ck);

  bool is_there_exeQueEntry(u32 ck_sum, u32 count);

  int8_t add_q_exe_times(u32 ck_sum, u32 count);

  int8_t add_q_exe(u32 ck_sum, u32 count, struct queue_entry *q);

  int8_t add_q_toT(CK_SUM ck_sum, u32 count, struct queue_entry *q);

  int8_t remove_q_fromT(CK_SUM ck_sum, u32 count, struct queue_entry *q);

  int8_t add_q_toC(CK_SUM ck_sum, u32 count, struct queue_entry *q);

  int8_t remove_q_fromC(CK_SUM ck_sum, u32 count, struct queue_entry *q);

  int8_t add_q_toW(CK_SUM ck_sum, u32 count, struct queue_entry *q);

  int8_t remove_q_fromW(CK_SUM ck_sum, u32 count, struct queue_entry *q);

  int8_t cfg_finish(CK_SUM ck_sum, u32 count);

  int8_t find_finished_cfg(CK_SUM ck_sum, u32 count);

  size_t get_size_c_cks(ck_size ck);
};

CK_SUM FilterInfo::get_cksum(size_t n) {
  if (count_to_ckSum.find(n) == count_to_ckSum.end())
    count_to_ckSum[n] = random() % (MAP_SIZE);
  auto res = count_to_ckSum[n];
  return res;
}

int8_t FilterInfo::find_finished_cfg(CK_SUM ck_sum, u32 count) {
  return finish_cfg.find(ck_sum) != finish_cfg.end();
}

int8_t FilterInfo::add_q_toT(CK_SUM ck_sum, u32 count, struct queue_entry *q) {
  if (q_t_mem.find(ck_sum) == q_t_mem.end()) {
    q_t_mem[ck_sum] = {};
  }
  q_t_mem[ck_sum].insert(q);
  return 1;
}

int8_t FilterInfo::remove_q_fromT(CK_SUM ck_sum, u32 count,
                                  struct queue_entry *q) {
  if (q_t_mem.find(ck_sum) == q_t_mem.end()) {
    fprintf(stderr, "[ERROR] Using no-exist ck_sum for q_t_mem!\n");
    return 0;
  }
  q_t_mem[ck_sum].erase(q);
  return 1;
}

int8_t FilterInfo::add_q_toC(CK_SUM ck_sum, u32 count, struct queue_entry *q) {
  if (q_c_mem.find(ck_sum) == q_c_mem.end()) {
    q_c_mem[ck_sum] = {};
  }
  q_c_mem[ck_sum].insert(q);
  return 1;
}

int8_t FilterInfo::remove_q_fromC(CK_SUM ck_sum, u32 count,
                                  struct queue_entry *q) {
  if (q_c_mem.find(ck_sum) == q_c_mem.end()) {
    fprintf(stderr, "[ERROR] Using no-exist ck_sum for q_c_mem!\n");
    return 0;
  }
  q_c_mem[ck_sum].erase(q);
  return 1;
}

int8_t FilterInfo::add_q_toW(CK_SUM ck_sum, u32 count, struct queue_entry *q) {
  if (q_w_mem.find({ck_sum, count}) == q_w_mem.end()) {
    q_w_mem[{ck_sum, count}] = {};
  }
  q_w_mem[{ck_sum, count}].insert(q);
  return 1;
}

int8_t FilterInfo::remove_q_fromW(CK_SUM ck_sum, u32 count,
                                  struct queue_entry *q) {
  if (q_w_mem.find({ck_sum, count}) == q_w_mem.end()) {
    fprintf(stderr, "[ERROR] Using no-exist ck_sum for q_w_mem!\n");
    return 0;
  }
  q_w_mem[{ck_sum, count}].erase(q);
  return 1;
}

int8_t FilterInfo::cfg_finish(CK_SUM ck, u32 count) {
  if (find_finished_cfg(ck, count) && q_c_mem.find(ck) != q_c_mem.end()) {
    fprintf(stderr, "[ERROR] same time exist in finished and un-finished?\n");
    exit(100);
  }
  /* This is set for the same testFile in dry_run. */
  if (q_c_mem.find(ck) == q_c_mem.end()) {
    WARNF("Cfg_finish: Using no-exist ck_sum for q_c_mem!\n");
    finish_cfg.insert(ck);
    return 0;
  }
  for (auto &q : q_c_mem[ck]) {
    auto q_info = search_qEntry_info(q->sch_number);
    if (!q_info->meaningful) {
      fprintf(stderr, "[ERROR] Using meaningless q_entry_cpp!\n");
      return 0;
    }
    q_info->finish_c({ck, 2});
  }
  q_c_mem.erase(ck);
  cks_q_c.erase({ck, 2});
  finish_cfg.insert(ck);
  return 1;
}

int8_t FilterInfo::add_q_exe(u32 ck_sum, u32 count, struct queue_entry *q) {
  if (exesum_q.find({ck_sum, count}) != exesum_q.end()) {
    FATAL(
        "You are realy inserting a existing entry node! This is meaningless!");
  }
  exesum_q[{ck_sum, count}] = q;
  return 1;
}

bool FilterInfo::is_there_exeQueEntry(u32 ck_sum, u32 count) {
  return exesum_q.find({ck_sum, count}) != exesum_q.end();
}

int8_t FilterInfo::add_q_exe_times(u32 ck_sum, u32 count) {
  ck_size init = {ck_sum, count};
  if (exesum_q.find(init) == exesum_q.end()) {
    fprintf(stderr, "[ERROR] Can't find the queue_entry looking for!\n");
#ifdef DEBUG
    exit(100);
#endif
    return 0;
  }
  if (exesum_q[init]->exec_time < UINT64_MAX)
    exesum_q[init]->exec_time++;
  return 1;
}

void FilterInfo::set_c_cks(struct queue_entry *q, ck_size ck) {
  cks_q_c[ck] = q;
}

void FilterInfo::set_w_cks(struct queue_entry *q, ck_size ck) {
  cks_q_w[ck] = q;
}

/* Return 1 means find successfully.*/
bool FilterInfo::find_q_by_w(ck_size cks) {
  return cks_q_w.find(cks) != cks_q_w.end();
}

/* Return 1 means find successfully.*/
bool FilterInfo::find_q_by_c(ck_size cks) {
  return cks_q_c.find(cks) != cks_q_c.end();
}

struct queue_entry *FilterInfo::search_q_by_w(ck_size cks) {
  if (cks_q_w.find(cks) == cks_q_w.end()) {
    fprintf(stderr, "[ERROR] Can't find cks in cks_q_w!");
#ifdef DEBUG
    exit(100);
#endif
    return nullptr;
  }
  return cks_q_w[cks];
}

struct queue_entry *FilterInfo::search_q_by_c(ck_size cks) {
  if (cks_q_c.find(cks) == cks_q_c.end()) {
    fprintf(stderr, "[ERROR] Can't find cks in cks_q_w!");
#ifdef DEBUG
    exit(100);
#endif
    return nullptr;
  }
  return cks_q_c[cks];
}

std::shared_ptr<Que_entry_cpp> FilterInfo::search_qEntry_info(size_t number) {
  if (q_entry_info_map.find(number) == q_entry_info_map.end()) {
    fprintf(stderr, "[ERROR] Out of range!");
#ifdef DEBUG
    exit(100);
#endif
    return nullptr;
  }
  return q_entry_info_map[number];
}

CK_SUM FilterInfo::get_ck_sum_by_two_131(ACTION_NUMBER a, ACTION_NUMBER b) {
  CK_SUM init = 131;
  init = hash_two(init, get_cksum(a));
  init = hash_two(init, get_cksum(b));
  return init;
}

void FilterInfo::add_q_entry(size_t loc, std::shared_ptr<Que_entry_cpp> ptr) {
  if (q_entry_info_map.find(loc) != q_entry_info_map.end()) {
    fprintf(stderr, "[ERROR] Repeated number!\n");
    return;
  }
  q_entry_info_map[loc] = ptr;
  return;
}

CK_SUM FilterInfo::get_cksum_by_two(ACTION_NUMBER a, ACTION_NUMBER b) {
  CK_SUM a_hash = get_cksum(a);
  CK_SUM b_hash = get_cksum(b);
  return hash_two(a, b);
}

void FilterInfo::add_element_to_ck_sum_map(CK_SUM ck_sum_in, size_t len) {
  if (ck_sum_map.find({ck_sum_in, len}) == ck_sum_map.end()) {
    ck_sum_map.insert({{ck_sum_in, len}, 1});
  }
}

void FilterInfo::dump_ctc_info() {
  std::cout << "[DEBUG] FilterInfo ctc info size: " << count_to_ckSum.size()
            << std::endl;
#ifdef OUT_DEBUG_1
  for (auto &item : count_to_ckSum) {
    std::cout << item.first << "  " << item.second << std::endl;
  }
#endif
}

bool FilterInfo::is_interesting_ckSum(CK_SUM ck_sum_in, size_t len) {
  if (ck_sum_map.find({ck_sum_in, len}) == ck_sum_map.end()) {
    // ck_sum_map.insert({{ck_sum_in, len}, 1}); NEXT: add to new!
    return true;
  }
  return false;
}

CK_SUM FilterInfo::hash_two(CK_SUM first, CK_SUM second) {
  return (first * 31) ^ second;
}

bool FilterInfo::search_con_set_or_add(std::pair<CK_SUM_32, size_t> &element) {
  if (search_con_set(element)) {
    return true;
  } else {
    con_ck_sum_set.insert(element);
    return false;
  }
};

void FilterInfo::clear() {
  count_to_ckSum.clear();
  ck_sum_map.clear();
  close_map.clear();
  con_ck_sum_set.clear();
  q_entry_info_map.clear();
  cks_q_w.clear();
  cks_q_c.clear();
  exesum_q.clear();
  q_w_mem.clear();
  q_c_mem.clear();
  q_t_mem.clear();
  exe_times = 0;
}

// TODO:
void build_g_filter_info_ctc() {}

static FilterInfo g_filter_info = FilterInfo();

/* A common function to get concurrent feature of t_info. */
std::pair<CK_SUM_32, size_t>
get_concurHash_from_tInfo(struct Thread_info *t_info) {
  std::set<CK_SUM> mem;
  for (size_t i = 0; i < t_info->final_thread_number; i++) {
    CK_SUM tmp_hash = 131;
    size_t j = 0;
    for (; j < SHARE_MEMORY_ARRAY_SIZE; j++) {
      auto action_now = t_info->kp_thread_array[i][j];
      if (action_now == -1) {
        break;
      }
      mem.insert(g_filter_info.get_cksum(action_now));
    }
  }
  return {hash32_set(mem), mem.size()};
}

/* JIBINNEXT:*/
CK_SUM create_cksum(const tid_ac_v &v, size_t start, size_t end) {
  CK_SUM res = 131;
  assert(end < v.size());
  for (size_t i = start; i <= end; i++) {
    res = (res * 31) ^ g_filter_info.get_cksum(v[i].second);
  }
  return res;
}

/* */
inline bool is_interesting(CK_SUM ck_sum, size_t len,
                           std::set<std::pair<CK_SUM, size_t>> &mem_cksum) {
  bool set_flag = false;
  if (mem_cksum.find({ck_sum, len}) == mem_cksum.end()) {
    set_flag = true;
    mem_cksum.insert({ck_sum, len});
  }
  return g_filter_info.is_interesting_ckSum(ck_sum, len) && set_flag;
}

class ActionType {
public:
  std::set<size_t> var;
  size_t counter;
  std::string function_name;
  action_type type;
  std::unordered_map<size_t, std::vector<size_t>> distance;
  bool is_group_head;
  std::vector<size_t> group_member;
  ActionType(size_t number)
      : counter(number), var({}), function_name({}), distance({}),
        is_group_head(false), group_member({}){};
  bool is_sperate(const ActionType &a);
  bool find_same_var(size_t number) const;
  inline void add_var(size_t item) { var.insert(item); };
  std::vector<size_t> get_distance_byEnd(size_t second);
  inline std::set<VAR_TYPE> get_var() { return var; }
  void add_group_members(size_t size_member, int *list);
  action_type get_lock_type();
  bool is_groupKey();
};

bool ActionType::is_groupKey() { return is_group_head; }

/* Get the action_type. */
action_type ActionType::get_lock_type() { return type; }

void ActionType::add_group_members(size_t size_member, int *list) {
  if (size_member > OP_MAX_COUNT || size_member == 0)
    return;
  for (size_t i = 0; i < size_member; i++) {
    group_member.push_back(list[i]);
  }
}

std::vector<size_t> ActionType::get_distance_byEnd(size_t key) {
  if (distance.find(key) == distance.end()) {
    return {};
  }
  return distance.at(key);
}

bool ActionType::find_same_var(size_t number) const {
  if (var.find(number) == var.end()) {
    return false;
  }
  return true;
}

/* Figure out if coule be treat two as single one*/
bool ActionType::is_sperate(const ActionType &a) {
  for (auto &item : var) {
    if (a.find_same_var(item)) {
      return false;
    }
  }
  return true;
}

/* Use for store infomation in single time! */

class SingleTimeInfo {
public:
  std::vector<std::shared_ptr<std::vector<std::pair<size_t, size_t>>>> result;
  std::set<std::vector<std::pair<size_t, size_t>>> useds;
  std::set<std::pair<size_t, size_t>> res_set;
  std::shared_ptr<Working_info> w_info;
  std::vector<std::shared_ptr<SingleThread>> st_que;
  std::set<std::pair<CK_SUM, size_t>> single_wCks;
  std::set<std::pair<CK_SUM, size_t>> thread_cks;
  struct cfg_info_token *cfg;
  struct cfg_info_token *cfg_finish;
  bool bad_plan_flag; /* Use to notify if there had a bad plan! */

  SingleTimeInfo()
      : result({}), useds({}), res_set(), w_info(), st_que(), single_wCks({}),
        thread_cks({}), cfg(nullptr), cfg_finish(nullptr) {}

  void remove_useless_var_in_set(std::set<VAR_TYPE> &set) const;

  void add_info(std::set<std::pair<size_t, size_t>> &res_set,
                std::shared_ptr<Working_info> w_info,
                std::vector<std::shared_ptr<SingleThread>> &st_que);

  void out_info(std::set<std::pair<size_t, size_t>> &res_set,
                std::shared_ptr<Working_info> &w_info,
                std::vector<std::shared_ptr<SingleThread>> &st_que);

  void clear();

  void clean_unfinished_info() { result.clear(); }

  void add_used(const tid_ac_v &used) { useds.insert(used); };

  bool find_used(const tid_ac_v &v) const {
    return useds.find(v) != useds.end() ? true : false;
  }

  void move_and_build_result(std::vector<tid_ac_v> &re);

  bool get_bad_flag() { return bad_plan_flag; }

  void add_cks(size_t plan_number); /* Add window cks here. */

  void add_cks(std::shared_ptr<tid_ac_v> plan);

  void add_cks(const tid_ac_v &plan);

  void add_thread_cks(const std::pair<CK_SUM, size_t> &ck);

  bool find_thread_ck(const std::pair<CK_SUM, size_t> &ck);

  virtual ~SingleTimeInfo();
};

SingleTimeInfo::~SingleTimeInfo() { clear(); }

bool SingleTimeInfo::find_thread_ck(const std::pair<CK_SUM, size_t> &ck) {
  return thread_cks.find(ck) != thread_cks.end();
}

void SingleTimeInfo::add_thread_cks(const std::pair<CK_SUM, size_t> &ck) {
  thread_cks.insert(ck);
}

/* This function don't care about the threadid type.*/
cks_ptr generate_wCks_from_plan(const tid_ac_v &plan) {
  size_t plan_size = plan.size();
  cks_ptr res = std::make_shared<std::set<std::pair<CK_SUM, size_t>>>();
  for (size_t i = 0; i < plan_size - 1; i++) {
    size_t threadid = plan[i].first;
    if (threadid == plan[i + 1].first)
      continue;
    for (int32_t j = i + 2; j < plan_size; j++) {
      if (plan[j].first == threadid) {
        std::vector<OP_TYPE> tmp;
        for (int32_t a = i + 1; a < j; a++) {
          tmp.push_back(plan[a].second);
        }
        CK_SUM init = g_filter_info.hash_two(
            131, g_filter_info.get_cksum(plan[i].second));
        init = g_filter_info.hash_two(init,
                                      g_filter_info.get_cksum(plan[j].second));
        for (auto &item : tmp) {
          init = g_filter_info.hash_two(init, g_filter_info.get_cksum(item));
        }
        res->insert({init, tmp.size() + 2});
        break;
      }
    }
  }
  return res;
}

/* Use generate_cks_from_plan. */
void SingleTimeInfo::add_cks(const tid_ac_v &plan) {
  auto res = generate_wCks_from_plan(plan);
  for (const auto &item : *res) {
    single_wCks.insert({item.first, item.second});
  }
}

void SingleTimeInfo::add_cks(std::shared_ptr<tid_ac_v> plan) {
  size_t plan_size = plan->size();
  auto cks_ptr = generate_wCks_from_plan(*plan);
  for (const auto &item : *cks_ptr) {
    single_wCks.insert({item.first, item.second});
  }
}

void SingleTimeInfo::add_cks(size_t plan_number) {
  std::shared_ptr<tid_ac_v> plan = result.at(plan_number);
  add_cks(plan);
}

void SingleTimeInfo::clear() {

  result.clear();
  useds.clear();
  st_que.clear();
  w_info = {};
  st_que.clear();
  single_wCks.clear();
  thread_cks.clear();
  bad_plan_flag = false;
  free(cfg);
  if (cfg_finish != nullptr) {
    free(cfg_finish);
  }
  cfg = nullptr;
  cfg_finish = nullptr;
}

/* NOTE: There happend belong move. */
void SingleTimeInfo::move_and_build_result(std::vector<tid_ac_v> &re) {
  for (size_t i = 0; i < re.size(); i++) {
    std::shared_ptr<tid_ac_v> ptr = std::make_shared<tid_ac_v>(re[i]);
    result.push_back(ptr);
  }
}

void SingleTimeInfo::add_info(
    std::set<std::pair<size_t, size_t>> &res_set_in,
    std::shared_ptr<Working_info> w_info_in,
    std::vector<std::shared_ptr<SingleThread>> &st_que_in) {

  res_set = std::set<std::pair<size_t, size_t>>(res_set_in);
  w_info = w_info_in;
  st_que.resize(st_que_in.size());
  for (size_t i = 0; i < st_que_in.size(); i++) {
    st_que[i] = std::move(st_que_in[i]);
  }
}

void SingleTimeInfo::out_info(
    std::set<std::pair<size_t, size_t>> &res_set_IN,
    std::shared_ptr<Working_info> &w_info_IN,
    std::vector<std::shared_ptr<SingleThread>> &st_que_IN) {
  res_set_IN = res_set;
  w_info_IN = w_info;
  for (auto &item : st_que) {
    st_que_IN.push_back(std::move(item));
  }
  st_que.clear();
}

static SingleTimeInfo g_single_time_info = SingleTimeInfo();

class ActionInfo {
public:
  std::vector<ActionType> list;
  std::unordered_map<size_t, size_t> numberToListmap;
  std::unordered_map<size_t, std::set<size_t>> mem_for_isSeparete_search;
  ActionInfo() : list({}), numberToListmap({}), mem_for_isSeparete_search({}){};
  bool is_sperate(size_t a, size_t b);
  void add_action(size_t action_n, size_t var);
  void add_action(size_t action);
  u32 get_max_count();
  std::vector<size_t> get_distance_byPair(std::pair<size_t, size_t> search);

  std::set<VAR_TYPE> search_vars_by_action(ACTION_NUMBER a);
  std::vector<VAR_TYPE> get_shared_vars(ACTION_NUMBER a, ACTION_NUMBER b);
  bool ifVarInAction(ACTION_NUMBER a, VAR_TYPE v);
  void clear();
  int8_t is_action_lock(size_t action_number);
  bool is_groupKey(size_t n);
  void dump();
};

void ActionInfo::dump() {
  std::cout << "-------ACTION DUMP-------";
  for (auto ac : list) {
    std::cout << "ACTION NUMBER: " << ac.counter << std::endl;
    for (auto var : ac.get_var()) {
      std::cout << "var: " << var << "  ";
    }
    std::cout << std::endl;
  }
  std::cout << "-------ACTION DUNP END-------";
}

bool ActionInfo::is_groupKey(size_t n) {
  auto loc = numberToListmap[n];
  return list[loc].is_groupKey();
}

int8_t ActionInfo::is_action_lock(size_t action_number) {
  if (numberToListmap.find(action_number) == numberToListmap.end())
    printf("[helper] There is no such action;%d", action_number);
  size_t loc_in_list = numberToListmap.at(action_number);
  auto action_t = list.at(loc_in_list).get_lock_type();
  if (action_t == lock_action)
    return 1;
  else if (action_t == unlock_action)
    return -1;
  return 0;
}

std::vector<size_t>
ActionInfo::get_distance_byPair(std::pair<size_t, size_t> search) {
  auto first = search.first;
  auto second = search.second;
  if (numberToListmap.find(first) == numberToListmap.end()) {
    return {};
  }
  return list[numberToListmap.at(first)].get_distance_byEnd(second);
}

u32 ActionInfo::get_max_count() {
  u32 max = 0;
  for (const auto &item : list) {
    if (item.counter > max)
      max = item.counter;
  }
  return max;
}

bool ActionInfo::ifVarInAction(ACTION_NUMBER a, VAR_TYPE v) {
  return search_vars_by_action(a).count(v) > 0;
}

std::vector<VAR_TYPE> ActionInfo::get_shared_vars(ACTION_NUMBER a,
                                                  ACTION_NUMBER b) {
  std::set<VAR_TYPE> a_set = search_vars_by_action(a);
  std::set<VAR_TYPE> b_set = search_vars_by_action(b);
  std::vector<VAR_TYPE> res;
  for (VAR_TYPE item : a_set) {
    if (b_set.find(item) != b_set.end()) {
      res.push_back(item);
    }
  }
  return res;
}

void ActionInfo::clear() {
  list.clear();
  numberToListmap.clear();
  mem_for_isSeparete_search.clear();
}

std::set<VAR_TYPE> ActionInfo::search_vars_by_action(ACTION_NUMBER a) {
  return list[numberToListmap[a]].get_var();
}

void ActionInfo::add_action(size_t action_n, size_t var) {
  if (numberToListmap.find(action_n) == numberToListmap.end()) {
    ActionType tmp = {action_n};
    list.push_back(tmp);
    numberToListmap.insert({action_n, list.size() - 1});
  }
  size_t loc = numberToListmap[action_n];
  list.back().add_var(var);
};

void ActionInfo::add_action(size_t action_n) {
  if (numberToListmap.find(action_n) == numberToListmap.end()) {
    ActionType tmp = {action_n};
    list.push_back(tmp);
    numberToListmap.insert({action_n, list.size() - 1});
  }
}

bool ActionInfo::is_sperate(size_t a, size_t b) {
  if (mem_for_isSeparete_search.find(a) != mem_for_isSeparete_search.end()) {
    if (mem_for_isSeparete_search[a].find(b) !=
        mem_for_isSeparete_search[a].end()) {
      return false;
    }
  } else {
    mem_for_isSeparete_search[a] = {};
  }
  size_t loca = numberToListmap[a];
  size_t locb = numberToListmap[b];
  if (list[loca].is_sperate(list[locb])) {
    return true;
  } else {
    mem_for_isSeparete_search[a].insert(b);
    return false;
  }
  return true;
}

static ActionInfo g_action_info = {};

bool is_groupKey(size_t number) { return g_action_info.is_groupKey(number); }

void build_actionInfo(struct function_array *f_array,
                      struct variable_array *v_array,
                      struct distance_array *d_array,
                      struct groupKey_array *g_array, ActionInfo &aI) {
  assert(f_array != NULL && v_array != NULL && d_array != NULL &&
         g_array != NULL);
  std::map<size_t, std::shared_ptr<ActionType>> mem;
  /* The f_array include the info of (func -> action). */
  // printf(
  //     "------build_actionInfo, f_arrary-size: %d, v: %d, d: %d, g:
  //     %d-------", f_array->size, v_array->size, d_array->size,
  //     g_array->size);
  for (size_t i = 0; i < f_array->size; i++) {
    struct function_container *func_c = f_array->list[i];
    for (size_t j = 0; j < func_c->size; j++) {
      std::string function_name(f_array->list[i]->name);
      size_t op_size = func_c->size;
      for (size_t k = 0; k < op_size; k++) {
        struct op_node *op = &func_c->op_list[k];
        if (mem.find(op->count) == mem.end()) {
          /* The info from v_array doesn't fit! */
          auto action_ptr = std::make_shared<ActionType>(op->count);

          if (!strcmp(op->actionType, "read"))
            action_ptr->type = read_action;
          else if (!strcmp(op->actionType, "write"))
            action_ptr->type = write_action;
          else if (!strcmp(op->actionType, "unlock"))
            action_ptr->type = unlock_action;
          else if (!strcmp(op->actionType, "lock"))
            action_ptr->type = lock_action;
          else
            action_ptr->type = default_action;

          mem[op->count] = action_ptr;
        }
        mem[op->count]->function_name = function_name;
      }
    }
  }
  /* The v_array include the info of (var -> action). */
  for (size_t i = 0; i < v_array->size; i++) {
    variable_container *item = v_array->list[i];
    for (size_t j = 0; j < item->size; j++) {
      struct op_node *a = &(item->op_list[j]);
      if (mem.find(a->count) == mem.end()) {
        auto action_ptr = std::make_shared<ActionType>(a->count);
        /* Actually the actiontype is meaningless.... ->_<-*/
        if (!strcmp(a->actionType, "read"))
          action_ptr->type = read_action;
        else if (!strcmp(a->actionType, "write"))
          action_ptr->type = write_action;
        else if (!strcmp(a->actionType, "unlock"))
          action_ptr->type = unlock_action;
        else if (!strcmp(a->actionType, "lock"))
          action_ptr->type = lock_action;
        else
          action_ptr->type = default_action;
        /* There are a lot imformation not included. */
        mem[a->count] = action_ptr;
      }
      /* set action's vars. */
      mem[a->count]->add_var(item->v_name);
    }
  }

  /* The f_array include the info of potential path! . */
  for (size_t i = 0; i < d_array->size; i++) {
    struct distance_container *d_c = d_array->list[i];
    /* The d_array contain action doesn't exist in f and v. */
    if (mem.find(d_c->owner) == mem.end()) {
      FATAL("[ERROR] Can't find the op from f_arrary!.");
      continue;
    }
    size_t owner = d_c->owner;
    for (size_t j = 0; j < d_c->size; j++) {
      size_t next = d_c->nexter[j];
      struct distance_node_info *dni = d_c->path[j];
      std::vector<size_t> tmp(dni->node_size);
      /* distance_node_info value is useless for now! */
      for (size_t k = 0; k < dni->node_size; k++) {
        tmp.push_back(dni->node[i]);
      }
      mem[owner]->distance[next] = tmp;
    }
  }

  for (size_t i = 0; i < g_array->size; i++) {
    struct group_key_container *g_key = g_array->list[i];
    for (size_t j = 0; j < g_key->size; j++) {
      size_t size_mem_list = g_key->size;
      size_t key = g_key->key;
      if (mem.find(key) == mem.end()) {
        FATAL("[ERROR] Can't find the op from f_arrary!.");
        continue;
      }
      mem[key]->is_group_head = true;
      mem[key]->add_group_members(size_mem_list, g_key->list);
    }
  }
  /* NOW finish the mem part! ^_^*/
  /* The f**k function is bs. >_<*/
  for (auto &item : mem) {
    aI.add_action(item.first);
    size_t loc = aI.numberToListmap[item.first];
    aI.list[loc] = *item.second;
  }
};

class SingleThread {
public:
  SingleThread(size_t start, size_t end, size_t size, size_t tid)
      : start(start), end(end), tid(tid), size(size), loc(0), list({}),
        is_main(false), to_main(), signal_list({}) {}
  SingleThread() : start(0), end(0), size(0), loc(0), list({}) {}
  void add_list(int32_t action);
  void dump();
  int32_t into_squence(std::vector<std::pair<size_t, size_t>> &que);
  inline void back_one_step(size_t len);
  void remove_element_fromLlist(std::vector<size_t> &locs);
  void init_signal_list_byV(const std::vector<int32_t> &v);
  void switch_signal_list(std::vector<int32_t> &v);
  std::vector<size_t> list;
  std::vector<int32_t> signal_list;
  size_t start;
  size_t end;
  size_t tid;
  const size_t size;
  int32_t loc;
  bool is_main;
  std::pair<size_t, size_t> to_main;
};

void SingleThread::switch_signal_list(std::vector<int32_t> &v) {
  /* Some check! */
  assert(v.size() == signal_list.size());
  auto res = std::move(signal_list);
  signal_list = std::move(v);
  v = std::move(res);
}

void SingleThread::init_signal_list_byV(const std::vector<int32_t> &v) {
  /* Some check! */
  assert(v.size() == list.size());
  assert(v.size() == size);
  signal_list = v;
}

void SingleThread::remove_element_fromLlist(std::vector<size_t> &locs) {
  std::sort(locs.begin(), locs.end());
  std::vector<size_t> res;
  for (size_t i = 0, j = 0; i < list.size(); i++) {
    if (i == locs[j]) {
      j++;
      continue;
    }
    res.push_back(list[i]);
  }
  list = res;
  return;
}

inline CK_SUM hash_two_number(CK_SUM a, CK_SUM b) { return a ^ b; }

void fill_single_que_in_set_signal_list(const std::vector<size_t> &single,
                                        std::vector<size_t> &res,
                                        std::set<CK_SUM> &mem_set) {

  assert(single.size() > 0 && res.size() == single.size());

  std::vector<std::pair<VAR_TYPE, size_t>> judge_mem;
  for (size_t i = 0; i < single.size(); i++) {
    if (single[i] != 0)
      judge_mem.push_back({single[i], i});
  }
  CK_SUM first = g_filter_info.get_cksum(judge_mem[0].first);
  CK_SUM second = 0;
  for (size_t i = 1; i < judge_mem.size(); i++) {
    second = g_filter_info.get_cksum(judge_mem[i].first);
    CK_SUM tmp = hash_two_number(first, second);
    if (mem_set.find(tmp) == mem_set.end()) {
      mem_set.insert(tmp);
      res[judge_mem[i - 1].second] += 1;
      res[judge_mem[i].second] += 1;
    }
    first = second;
  }
}

int32_t
SingleThread::into_squence(std::vector<std::pair<size_t, size_t>> &que) {
  if (loc > size) {
    fprintf(stderr, "[ERROR] Single_thread::into_squence -> OVER the range.\n");
  }
#ifdef OUT_DEBUG_1
  std::cout << "[DEBUG] Single_thread::into_squence -> once. " << std::endl;
#endif
  size_t res = 0;
  if (loc == (int32_t)size) {
    return -1;
  }
  do {
    que.push_back(std::make_pair(tid, list[loc]));
    loc++;
    res += 1;
  } while (loc < size && signal_list[loc] != 1);
  return res;
}

void SingleThread::back_one_step(size_t len) {
  if (loc < 0) {
    fprintf(
        stderr,
        "[ERROR] Single_thread::back_one_step() -> there is no way to back.\n");
    exit(1);
  }
  loc -= len;
}

void SingleThread::add_list(int32_t action) { list.push_back(action); }

void SingleThread::dump() { std::cout << tid << std::endl; }

class Working_info {
public:
  std::unordered_map<size_t, std::unordered_set<size_t>>
      interesting_relation; /* Represent as tid! This include the interesting
                               pair!*/
  std::unordered_set<size_t> interesting; /* Represent as tid! */
  /* Represent as tid! */
  std::unordered_map<size_t, std::unordered_set<size_t>> start_same_time;
  Working_info()
      : interesting_relation({}), interesting({}), start_same_time({}){};
  /* Extra thread need care from t_n_c*/
  void generate_interesting(const struct thread_need_care *t_n_c);
  /* make sure the schedueled thread is in same time space!*/
  void rebuild_relation_map(
      const std::vector<std::shared_ptr<SingleThread>> &st_que);
  void generate_interesting_pair(std::set<std::pair<size_t, size_t>> &res_set);
  void generate_start_same_time(
      const std::vector<std::shared_ptr<SingleThread>> &st_que);
};

void Working_info::generate_interesting_pair(
    std::set<std::pair<size_t, size_t>> &res_set) {
  std::cout << "[DEBUG] interesting size: " << interesting.size() << std::endl;
  for (const auto &start : interesting) {
    /* only care about the thread in same time space! */
    for (const auto &end : interesting_relation[start]) {
      if (res_set.find({end, start}) == res_set.end()) {
        res_set.insert({start, end});
      }
    }
  }
#ifdef OUT_DEBUG
  std::cout << "[DEBUG] Working_info::generate_interesting_pair res size: ";
  std::cout << res_set.size() << std::endl;
#endif
}

void Working_info::generate_interesting(const struct thread_need_care *t_n_c) {
  for (size_t i = 0; i < t_n_c->size; i++) {
    interesting.insert(t_n_c->tid_list[i]);
  }
#ifdef OUT_DEBUG
  std::cout << "[DEBUG] Working_info::generate_interesting finished! size:"
            << interesting.size() << std::endl;
#endif
}

void Working_info::rebuild_relation_map(
    const std::vector<std::shared_ptr<SingleThread>> &st_que) {
  for (size_t i = 0; i < st_que.size(); i++) {
    if (st_que[i]->is_main)
      continue;
    interesting_relation[st_que[i]->tid] = {};
    for (size_t j = i + 1; j < st_que.size(); j++) {
      if (st_que[j]->is_main)
        continue;
      if (std::max(st_que[i]->start, st_que[j]->start) <
          std::min(st_que[i]->end, st_que[j]->end)) {
        interesting_relation[st_que[i]->tid].insert(st_que[j]->tid);
      }
    }
  }
#ifdef OUT_DEBUG
  std::cout << "[DEBUG] Working_info::rebuild_relation_map finished! size:"
            << interesting_relation.size() << std::endl;
#endif
}

void Working_info::generate_start_same_time(
    const std::vector<std::shared_ptr<SingleThread>> &st_que) {
#ifdef OUT_DEBUG
  std::cout << "[DEBUG] Working_info::generate_start_same_time start!"
            << std::endl;
#endif
  for (size_t i = 0; i < st_que.size(); i++) {
    interesting_relation[st_que[i]->tid] = {};
    for (size_t j = 0; j < st_que.size(); j++) {
      if (st_que[j]->tid == st_que[i]->tid)
        continue;
      if (st_que[i]->start == st_que[j]->start) {
        interesting_relation[st_que[i]->tid].insert(st_que[j]->tid);
      }
    }
  }
}

std::unique_ptr<SingleThread>
build_from_token(const struct single_info_token *token) {
  size_t start = token->start;
  size_t end = token->end;

  auto res = std::make_unique<SingleThread>(start, end, token->action_size,
                                            token->tid);
  for (int32_t i = 0; i < (int32_t)token->action_size; i++) {
    res->add_list(token->actions[i]);
  }
  res->is_main = token->is_main;
  return res;
}

size_t
get_max_size_from_ques(const std::vector<std::shared_ptr<SingleThread>> &st_que,
                       const std::vector<size_t> &loc) {
  size_t res = 0;
  std::for_each(loc.begin(), loc.end(),
                [&res, &st_que](const unsigned long &number) {
                  res += st_que[number]->size;
                });
#ifdef OUT_DEBUG
  std::cout << "[DEBUG] get_max_size_from_ques->res:  " << res << std::endl;
#endif
  return res;
}

/*return 0 means there is no unallowed_situation. */
/* If user choose no search_map, the function will take g_filter_info one. */
bool is_have_unallowed_situation(tid_ac_v &que,
                                 WindowSearchMap *search_map = nullptr) {
  for (size_t i = 0; i < que.size() - 1; i++) {
    size_t thread_name_loc = que[i].first;
    if (thread_name_loc == que[i + 1].first)
      continue;
    for (size_t j = i + 2; j < que.size(); j++) {
      if (que[j].first == thread_name_loc) {
        std::vector<OP_TYPE> tran;
        for (size_t a = i + 1; a < j; a++) {
          tran.push_back(que[a].second);
        }
        if (search_map == nullptr) {
          if (g_filter_info.search_close_map_by_pair_v(
                  {que[i].second, que[j].second}, tran)) {
            return true;
          }
        } else {
          if (search_map->search_by_pair_v({que[i].second, que[j].second},
                                           tran)) {
            return true;
          }
        }
        break;
      }
    }
  }
  return false;
}

/* Take first v as main one, return value -1 means that the two v is same.
 * Otherwise, any number bigger than -1 means the differ happend at this
 * location. Normaly, take the no-template v as first v, but it depends on
 * different situation. NOTE: The repeated action should be eliminated
 * during running test.
 */
std::pair<int32_t, int32_t> cmp_tid_ac_v(const tid_ac_v *const first,
                                         const tid_ac_v *const second) {
#ifdef ASSERT_DEBUG
  assert(first->size() == second->size());
#endif
  size_t loc_first = 0, loc_second = 0;
  while (loc_first < first->size() && loc_second < second->size()) {
    if ((*first)[loc_first].second == (*second)[loc_second].second) {
      loc_first += 1;
      loc_second += 1;
      continue;
    } else {
      return {loc_first, loc_second};
    }
  }
  return {-1, -1};
}

// TODO:  1. make sure if there are repeated actions be remembered.
// 2. filter for ques.
bool check_if_obey(struct Thread_info *t_info,
                   const std::shared_ptr<tid_ac_v> &cmp,
                   SingleTimeInfo &single_time_info,
                   std::pair<size_t, size_t> &test_out, OP_TYPE &test_op_out) {
  const struct Kp_action_info *kp_mem = t_info->kp_mem;
  const u64 kp_mem_size = t_info->kp_mem_size;
  /* Store the running info. */
  tid_ac_v info_in(kp_mem_size);
  for (size_t i = 0; i < kp_mem_size; i++) {
    info_in[i] = {kp_mem[i].threadid,
                  kp_mem[i].loc}; /* TODO: change the infomation search
                                     system.(In the testd program.) */
  }
  /* Cmp and get result! */
  std::pair<int32_t, int32_t> loc = cmp_tid_ac_v(&info_in, cmp.get());
  if (loc.second == -1)
    return true;

  /* The same thread replacing(means that the op being the wrong loc from same
   * thread) have not been considered. (Just return true now.) BY the way, the
   * situation that length of two vector is different have not been taken into
   * consideration. */
  if (cmp->at(loc.second).first == info_in.at(loc.second).first)
    return true;
  size_t thread_name_loc = cmp->at(loc.second).first,
         tmp = thread_name_loc; /* Work on template sequence. */
  int32_t front = -1, end = -1;
  for (tmp = tmp - 1; tmp >= 0; tmp--) {
    if ((*cmp)[tmp].first != tmp) {
      front = tmp;
    }
  }

  for (tmp = thread_name_loc + 1; tmp < cmp->size(); tmp++) {
    if ((*cmp)[tmp].first != tmp) {
      end = tmp;
    }
  }

  if (front == -1 || end == -1) /* Do nothing. */
    return true;
  test_out = {(*cmp)[front].second, (*cmp)[end].second};
  test_op_out = (*cmp)[thread_name_loc].second;
  g_filter_info.add_close_map_by_pair(test_out, test_op_out);
  return false;
}

// TODO:
/* Used to figure out which var is not used between program. */
void generate_useless_var(
    const std::vector<std::unique_ptr<SingleThread>> &st_que,
    std::vector<VAR_TYPE> &store_v) {
  std::map<VAR_TYPE, size_t> counter_map;
  /* TODO: Sorry for using triple loop. Need changing. */
  for (const auto &st : st_que) {
    std::set<VAR_TYPE> mem_var_set;
    for (const auto &p : st->list) {
      for (auto &var : g_action_info.search_vars_by_action(p)) {
        mem_var_set.insert(var);
      }
    }
    for (const auto &var : mem_var_set) {
      if (counter_map.find(var) == counter_map.end()) {
        counter_map.insert({var, 0});
      }
      counter_map[var] += 1;
    }
  }
  for (auto &item : counter_map) {
    if (item.second <= 1) {
      store_v.push_back(item.first);
    }
  }
}

/* Used to reset the info don't need. */
void leave_single_times() { g_single_time_info.clear(); }

/* Used for generate the basic info just for single times. */
void generate_basic_info(struct thread_info_scheduel_token *token,
                         std::shared_ptr<Working_info> w_info,
                         std::vector<std::shared_ptr<SingleThread>> &st_que,
                         const struct thread_need_care *t_n_c) {
  for (size_t i = 0; i < token->size; i++) {
    st_que.push_back(build_from_token(&token->list[i]));
  }
#ifdef OUT_DEBUG
  std::cout << "[DEBUG] generate_basic_info add st_que finished! size:"
            << st_que.size() << std::endl;
#endif
  g_single_time_info.clear();
  // TODO: generate info
  w_info->generate_interesting(t_n_c);
  w_info->rebuild_relation_map(st_que);
  w_info->generate_start_same_time(st_que);
  return;
}

/* After one que is running finished. */
void finish_one_que(size_t number) {
  const std::shared_ptr<tid_ac_v> v =
      g_single_time_info.result.at(number); // check range plz
  size_t len_v = v->size();
  std::vector<size_t> mem;
  for (size_t i = 0; i < len_v; i++) {
    if ((*v)[i].first != (*v)[i + 1].first) {
      mem.push_back(i);
    }
  }

#ifdef OUT_DEBUG
  std::cout << "[DEBUG] finish_one_que  before g_filter_info.ck_sum_map.size: "
            << g_filter_info.ck_sum_map.size() << std::endl;
#endif

  for (size_t i = 0; i < mem.size() - 1; i++) {
    CK_SUM ck_sum = create_cksum(*v, mem[i], mem[i + 1]);
    g_filter_info.add_element_to_ck_sum_map(ck_sum, mem[i + 1] - mem[i] + 1);
  }
#ifdef OUT_DEBUG
  std::cout << "[DEBUG] finish_one_que after g_filter_info.ck_sum_map.size: "
            << g_filter_info.ck_sum_map.size() << std::endl;
#endif
}

/* Put info into result. */
void put_Scheduel_que_into_result(
    struct scheduel_result_list **resutl_list_ptr,
    const std::vector<std::vector<std::pair<size_t, size_t>>> &final_list,
    std::shared_ptr<Working_info> w_info,
    std::shared_ptr<std::unordered_map<int64_t, size_t>> tid_number_map) {

  if ((*resutl_list_ptr) != NULL) {
    FATAL("[ERROR] put_Scheduel_que_into_result trans is not NULL");
  }

  u32 list_size = final_list.size();
  (*resutl_list_ptr) = (struct scheduel_result_list *)malloc(
      sizeof(struct scheduel_result_list) +
      (list_size) * sizeof(struct scheduel_result *));
  struct scheduel_result_list *result_list = (*resutl_list_ptr);
  result_list->size = 0;
  memset(result_list->list, (int)NULL,
         sizeof(struct scheduel_result *) * list_size);
  /* Start fill the scheduel_results. */
  for (const auto &v : final_list) {
    /* v is a vector, item:v -> item is (tid, number to represent action). */
    size_t list_size_single = v.size();
    struct scheduel_result *tmp_re = (struct scheduel_result *)malloc(
        sizeof(struct scheduel_result) + list_size_single * sizeof(u32));
    memset(tmp_re->thread_same_period, -1,
           MY_PTHREAD_CREATE_MAX * sizeof(int64_t));
    result_list->list[result_list->size++] = tmp_re;
    /* tid_to_number: {tid, (location in result sequence, size of the block)}*/
    std::unordered_map<size_t, std::pair<size_t, size_t>> tid_to_number;
    size_t counter = 0;
    for (const auto &item : v) {
      /* item std::pair(tid, counter pre set). */
      if (tid_to_number.find(item.first) == tid_to_number.end()) {
        tid_to_number[item.first] = {counter, 0};
        if (counter > 1) {
          FATAL("Wrong counter for create tid_to_number here.");
        }
        counter += 1;
      }
      tid_to_number[item.first].second += 1;
    }
    /* Check tid_to_number if OK >_<. */
    if (tid_to_number.size() != 2) {
      FATAL("[ERROR] put_Scheduel_que_into_result -> TOO MUCH entry in "
            "tid_to_number.\n");
    }

    /* Fill the struct scheduel_result entry_size and create_sequence. */
    for (auto &item : tid_to_number) {
      auto tid = item.first;
      auto entry_size = item.second.second;
      auto seq = item.second.first;
      tmp_re->entry_size[seq] = entry_size;
      /* Add to g_single_time_info at the begin of generate_que*/
      tmp_re->result_seq[seq] = tid_number_map->at(tid);
    }

    /* Fill the thread_same_period by w_info. */
    std::set<int32_t> tids_same_period; /* Inside is tid!!!*/
    int64_t early_time_tid = INT64_MAX;
    for (auto &item : tid_to_number) {
      auto tid = item.first;
      tids_same_period.insert(tid);
      if (tid < early_time_tid) {
        early_time_tid = tid;
      }
    }

    auto search_res = w_info->start_same_time[early_time_tid];
    tids_same_period.insert(search_res.begin(), search_res.end());
    assert(tids_same_period.size() < MY_PTHREAD_CREATE_MAX);
    counter = 0;
    for (auto &tid : tids_same_period) {
      // NEXT:
      tmp_re->thread_same_period[counter] = tid_number_map->at(tid);
      counter++;
    }
    counter = 0;
    /* Generate delay times. */
    std::vector<size_t> numbers(v.size(), 0);
    for (size_t i = 1; i < v.size(); i++) {
      if (v[i].first == v[i - 1].first) {
        numbers[i] = 0;
      } else {
        numbers[i] = 1;
      }
    }

    std::vector<std::vector<size_t>> stop_times(tid_to_number.size(),
                                                std::vector<size_t>());
    for (size_t i = 0; i < numbers.size(); i++) {
      /* v[i].first --> tid! */
      auto tid = v[i].first;
      size_t loc_result_stop_times = tid_to_number[tid].first;
      stop_times[loc_result_stop_times].push_back(numbers[i]);
    }
    /* Fill the result->stop_times. */
    int counter_stop = 0;
    for (auto &stop_times_single : stop_times) {
      for (auto &stop_time : stop_times_single) {
        tmp_re->stop_times[counter_stop] = stop_time;
        counter_stop++;
      }
    }
    counter_stop = 0;

    /* Add some check! */
    size_t stop_times_tid = 0;
    for (auto &item : tid_to_number) {
      stop_times_tid += item.second.second;
    }
    assert(stop_times_tid = v.size());
    size_t stop_times_v = 0;
    for (auto &stop_times_single : stop_times) {
      stop_times_v += stop_times_single.size();
    }
    assert(stop_times_v == v.size());

    if (v.size() > 0) {
      int32_t tid_to_number_zero_tid = -1;
      for (auto &item : tid_to_number) {
        if (item.second.first == 0)
          tid_to_number_zero_tid = item.first;
      }
      assert(tid_to_number_zero_tid == v[0].first);
    }

    for (auto &item : tid_to_number) {
      auto entry_size = item.second.second;
      auto seq = item.second.first;
      if (stop_times[seq].size() != entry_size)
        FATAL(" stop_times[seq].size() == entry_size ");
    }

  } /* for (const auto &v : final_list) end. */
}

void build_action_info_for_test() {
  std::vector<std::pair<size_t, size_t>> pair_v = {
      {1, 2}, {2, 2}, {3, 2}, {4, 2},  {5, 2}, {6, 2},
      {7, 2}, {8, 2}, {9, 2}, {10, 2}, {10, 1}};
  for (auto &item : pair_v) {
    g_action_info.add_action(item.first, item.second);
  }
}

void clear_helper() {
  g_action_info.clear();
  g_single_time_info.clear();
  g_filter_info.clear();
}

/* This function is used to check how many interesting window exist in plan, And
 * if the number is zero, it will also add it into g_single_info.cks. */
size_t
get_meaning_couple_from_v(const tid_ac_v &v,
                          std::set<std::pair<CK_SUM, size_t>> &mem_cksum) {
  size_t re = 0;
  size_t len_v = v.size();
  cks_ptr res = generate_wCks_from_plan(v);
  for (auto item : *res) {
    re += is_interesting(item.first, item.second, mem_cksum) ? 1 : 0;
  }
  /* 如果没有进入实际运行阶段，still put it into cks. */
  if (re == 0) {
    for (auto item : *res) {
      g_single_time_info.single_wCks.insert({item.first, item.second});
    }
  }
  return re;
}

/* NOTE: This is only useful under two thread!!! ╮（╯▽╰）╭*/
bool is_offense_main(tid_ac_v &que,
                     const std::vector<std::unique_ptr<SingleThread>> &st_que,
                     const std::vector<size_t> &loc) {
  auto main_1 = st_que[loc[0]]->is_main;
  auto main_2 = st_que[loc[0]]->is_main;
  if (main_1 != true && main_2 != true)
    return false;
  size_t loc_not_main = main_1 == true ? main_2 : main_1;
  size_t loc_main = main_1 == true ? main_1 : main_2;
  assert(loc_main != loc_not_main);
  size_t end_loc_not_main = st_que[loc_not_main]->to_main.second;
  if (std::abs((int32_t)(st_que[loc_main]->size - end_loc_not_main)) <= 1) {
    return false;
  }
  int32_t mainTid = st_que[loc_main]->tid;
  /* Euuu... We must change this! */
  if ((que.back().first) != mainTid)
    return true;
}

void create_que(const std::vector<std::shared_ptr<SingleThread>> &st_que,
                const std::vector<size_t> &loc,
                std::vector<std::vector<std::pair<size_t, size_t>>> &final_list,
                tid_ac_v &res, size_t max_size, size_t size,
                std::set<std::pair<CK_SUM, size_t>> &mem_cksum) {
  if (size == max_size) {
    /* The sequence can't be changed! */
    /* Plans unused woule be add to g_single_info.cks by
     * get_meaning_couple_from_v*/
    if (!is_have_unallowed_situation(res) &&
        get_meaning_couple_from_v(res, mem_cksum) > 0) {
      final_list.push_back(res);
    }
    return;
  }
  for (const auto &item : loc) {
    int32_t len_plus = st_que[item]->into_squence(res);
    if (len_plus > 0) {
      create_que(st_que, loc, final_list, res, max_size, size + len_plus,
                 mem_cksum);
      for (size_t i = 0; i < len_plus; i++) {
        res.pop_back();
      }
      st_que[item]->back_one_step(len_plus);
    }
  }
}

} // namespace

namespace HELPER__CC_TEST {

/* Create test_WindowSearchMap. */
WindowSearchMap get_test_WindowSearchMap() {
  WindowSearchMap test_map = {};
  test_map.add_element({1, 2}, 3);
  test_map.add_element({5, 6}, 4);
  return test_map;
}

void test_WindowSearchMap() {
  WindowSearchMap test_map = {};
  test_map.add_element({1, 2}, 100);
  test_map.add_element({500, 600}, 200);
  bool first = test_map.search_by_pair({1, 1}, 100);
  bool second = test_map.search_by_pair({1, 2}, 100);
  bool third = test_map.search_by_pair({500, 600}, 200);
  std::cout << "[TEST] test_WindowSearchMap\n";
  assert(first == false && second == true && third == true);
  std::cout << "[TEST] test_WindowSearchMap succesed" << std::endl;
}

// NEXT: add normal port.
void build_build_g_filter_info_ctc() {}

void build_g_filter_info_ctc_for_test1(size_t number) {
  for (size_t i = 0; i < number; i++) {
    g_filter_info.add_count_to_ckSum(i, R(1 << 16));
  }
#ifdef OUT_DEBUG
  g_filter_info.dump_ctc_info();
#endif
}

void test_check_if_obey1() {
  struct Thread_info *t_info =
      (struct Thread_info *)malloc(sizeof(struct Thread_info));
  t_info->kp_mem_size = 5;
  const std::shared_ptr<tid_ac_v> cmp = std::make_shared<tid_ac_v>();
  tid_ac_v item1 = {{1, 2}, {1, 3}}; // NEXT:
}

void test_is_have_unallowed_situation() {
  auto search_map = get_test_WindowSearchMap();
  tid_ac_v que = {{5, 10}, {5, 1}, {6, 7}, {6, 8}, {6, 3}, {5, 2}};
  tid_ac_v que1 = {{5, 10}, {5, 1}, {6, 7}, {6, 8}, {6, 3}, {5, 7}, {5, 2}};
  tid_ac_v que2 = {{5, 10}, {5, 1}, {6, 7}, {6, 8}, {6, 3}, {5, 7}, {5, 2}};
  bool flag = is_have_unallowed_situation(que, &search_map);
  bool flag1 = is_have_unallowed_situation(que1, &search_map);
  std::cout << "[TEST] test_is_have_unallowed_situation.\n";
  assert(flag == true);
  assert(flag1 == false);
  std::cout << "[TEST] test_is_have_unallowed_situation finish." << std::endl;
}

} // namespace HELPER__CC_TEST

/* Used to search if the action is lock or unlock.
 * @return_value: 0 means it's not lock or unlock.
 * @return_value: 1 means it's lock.
 * @return_value: -1 means it's unlock.
 */
int8_t is_lock(size_t aciotn_number) {
  return g_action_info.is_action_lock(aciotn_number);
};

/* Generate interesting que and init it according to struct
 * thread_info_scheduel_token token!*/
using Ptr_i_que = std::shared_ptr<std::vector<std::vector<int32_t>>>;
void generate_and_init_Iq(Ptr_i_que que,
                          struct thread_info_scheduel_token *token) {
#ifdef OUT_DEBUG
  std::cout << "generate_and_init_Iq" << std::endl;
#endif
  for (size_t i = 0; i < token->size; i++) {
    auto size_l = token->list[i].action_size;
    que->push_back(std::vector<int32_t>(size_l, 1));
  }
#ifdef OUT_DEBUG
  std::cout << "generate_and_init_Iq finish!" << std::endl;
#endif
}

void compare_two_sequence(struct single_info_token *s_one,
                          struct single_info_token *s_two, size_t loc_one,
                          size_t loc_two, Ptr_i_que interesting_ques) {
  /* Can't use the same sequence! */
  if (loc_one == loc_two)
    return;
  /* Find the same part! */
  size_t i = 0;
  for (; i < s_one->action_size && i < s_two->action_size; i++) {
    if (s_one->actions[i] != s_two->actions[i])
      break;
  }
  /* Color the first one !*/
  if (i == 0)
    return;
  for (size_t j = 0; j < i; j++) {
    /* Color as not interesting! */
    (*interesting_ques)[loc_one][j] = -1;
  };
#define WILD_MODE
/* It's too wild for me, I think....*/
#ifdef WILD_MODE
  std::set<std::pair<size_t, size_t>> mem;
  for (size_t i = 0; i < s_two->action_size - 1; i++) {
    mem.insert({s_two->actions[i], s_two->actions[i + 1]});
  }
  for (size_t i = 0; i < s_one->action_size - 1; i++) {
    if (mem.find({s_one->actions[i], s_one->actions[i + 1]}) != mem.end())
      (*interesting_ques)[loc_one][i] = -1;
  }
#endif
}

/* JIBIN PS: I do think that extra this double for into a macro function.
 * But ... waiting...ing...*/
/* First of all, do the thread sequence! color it! */
void color_que_with_thread_similarity(
    Ptr_i_que interesting_ques, struct thread_info_scheduel_token *token) {
  if (interesting_ques->size() == 0)
    FATAL("Plz init the interesting_que before use.");
  /* Plz init the interesting_que before use.*/
  for (size_t i = 0; i < token->size; i++) {
    struct single_info_token *s_one = &token->list[i];
    for (size_t j = 0; j < token->size; j++) {
      if (i == j)
        continue;
      struct single_info_token *s_two = &token->list[j];
      compare_two_sequence(s_one, s_two, i, j, interesting_ques);
    }
  }
}

/* Now we need worry about the order of color the interesting list*/
/* Find the repeated window in single thread! */
void color_que_with_window(Ptr_i_que interesting_ques,
                           struct thread_info_scheduel_token *token) {
  for (size_t i = 0; i < token->size; i++) {
    struct single_info_token *s_one = &token->list[i];
    if (s_one->is_main) /* We dont't color the main thread sequence here. */
      continue;
    std::set<std::pair<int32_t, int32_t>> mem;
    for (int32_t j = 0; s_one->action_size > 1 && j < s_one->action_size - 1;
         j++) {
      assert((*interesting_ques)[i].size() == s_one->action_size);
      int32_t a_a = s_one->actions[j];
      int32_t a_b = s_one->actions[j + 1];
      std::pair<int32_t, int32_t> p = {a_a, a_b};
      if (mem.find(p) == mem.end()) {
        mem.insert(p);
      } else {
        (*interesting_ques).at(i).at(j) = -1;
      }
    }
  }
}

/* Find action protected by lock! */
void color_que_with_lock_initiative(Ptr_i_que interesting_ques,
                                    struct thread_info_scheduel_token *token){};

/* Find action protected by lock! */
void color_que_with_lock_default(
    Ptr_i_que interesting_ques,
    const std::vector<std::unique_ptr<SingleThread>> &st_que) {

  for (size_t i = 0; i < st_que.size(); i++) {
    std::stack<size_t> s_lock = {};
    size_t j = 0;
    for (auto item : st_que[i]->list) {
      auto action_t = is_lock(item);
      /* According to the action_t to determine s_lock action! */
      if (action_t == 1) {
        s_lock.push(item);
      } else if (action_t == -1) {
        if (s_lock.empty()) {
          printf("There are a lock infor collection error! The lock number is "
                 "less than unlock!");
        } else
          s_lock.pop();
      }

      /* Determine interesting_que state based on s_lock*/

      if (!s_lock.empty()) {
        (*interesting_ques)[i][j] = -1;
      }
      j++;
    }
  }
}

/* Value a interesting que. */
int32_t get_value_interesting_que(std::vector<int32_t> &v) {
  int32_t counter = 0;
  for (auto status : v) {
    if (status == 1) {
      counter += 1;
    }
  }
  return counter + 1 < 0 ? 0 : counter + 1;
}

/**/
void color_unconnected_action(Ptr_i_que interesting_ques,
                              struct thread_info_scheduel_token *token) {
  for (size_t i = 0; i < token->size; i++) {
    struct single_info_token *s = &token->list[i];
    if (s->action_size == 0)
      continue;
    for (int32_t j = 0; j < (int32_t)s->action_size - 1; j++) {
      auto old_set = g_action_info.search_vars_by_action(s->actions[j]);
      if (old_set.empty())
        continue;
      auto new_set = g_action_info.search_vars_by_action(s->actions[j + 1]);
      bool find_same = false;
      for (auto item : new_set) {
        if (old_set.find(item) != old_set.end()) {
          find_same = true;
          break;
        }
      }
      if (find_same == false)
        (*interesting_ques)[i][j] = -1;
    }
  }
}

/* Plz take main thread as a special funtion!*/
Ptr_i_que
color_que_first(struct thread_info_scheduel_token *token,
                const std::vector<std::shared_ptr<SingleThread>> &st_que) {
  /* Init the I_q firstly. */

  Ptr_i_que I_q = std::make_shared<std::vector<std::vector<int32_t>>>();
  generate_and_init_Iq(I_q, token);
  /* Color interesting que with thread_similarity, which will uninterest the
   * content shared by more than one thread.*/
  color_que_with_thread_similarity(I_q, token);

  // if (!Test_lock)
  //   color_que_with_lock_default(I_q, st_que);
  // else /* */
  //   color_que_with_lock_initiative(I_q, token);
  color_unconnected_action(I_q, token);
  color_que_with_window(I_q, token);
#ifdef OUT_DEBUG
  std::cout << "[DEBUG] init and color the que first step!." << std::endl;
#endif
  return I_q;
}

void color_mainQue_useless_part(struct thread_info_scheduel_token *token,
                                std::vector<int32_t> &I_1,
                                std::vector<int32_t> &I_2, size_t my_tid1,
                                size_t my_tid2) {
  struct single_info_token *s_one = &token->list[my_tid1];
  struct single_info_token *s_two = &token->list[my_tid2];
  bool have_main = false;
  int32_t loc_un_main = -1;
  struct single_info_token *s_main = NULL;
  if (my_tid1 == my_tid2) {
    FATAL("color_mainQue_useless_part with two same thread acount! ");
  }
  if (token->list[my_tid1].is_main == 1) {
    have_main = true;
    s_main = s_one;
    loc_un_main = my_tid2;
  } else if (token->list[my_tid2].is_main == 1) {
    have_main = true;
    s_main = s_two;
    loc_un_main = my_tid1;
  }
  if (have_main == false)
    return;
  if (loc_un_main == -1)
    FATAL("There is two main thread here!");
  size_t loc_main1 = (size_t)token->list[loc_un_main].to_main[0];
  size_t loc_main2 = (size_t)token->list[loc_un_main].to_main[1];
  std::vector<int32_t> *ptr = NULL;
  if (my_tid1 != loc_un_main)
    ptr = &I_1;
  else if (my_tid2 != loc_un_main)
    ptr = &I_2;
  else
    FATAL("Can't find right main thread I_q");
  size_t ptr_size = ptr->size();
  for (size_t i = 0; (i < ptr_size) && (i < loc_main1) && (i >= 0); i++) {
    (*ptr)[i] = -1;
  }

  for (size_t i = loc_main2; (i < ptr_size) && (i > 0); i++) {
    (*ptr)[i] = -1;
  }
}

void color_que_useless_var(struct thread_info_scheduel_token *token,
                           std::vector<int32_t> &I_1, std::vector<int32_t> &I_2,
                           size_t my_tid1, size_t my_tid2) {
  /* First of all, make set for usefull*/
  struct single_info_token *s_one = &token->list[my_tid1];
  struct single_info_token *s_two = &token->list[my_tid2];
  std::set<VAR_TYPE> set_f;
  std::set<VAR_TYPE> set_use;
  std::unordered_map<VAR_TYPE, size_t> var_times;

  for (size_t i = 0; i < s_one->action_size; i++) {
    auto set = g_action_info.search_vars_by_action(s_one->actions[i]);
    if (set.empty())
      continue;
    for (auto item : set) {
      set_f.insert(item);
    }
  }
  /* Could take the two into single one! */
  for (size_t i = 0; i < s_two->action_size; i++) {
    auto set = g_action_info.search_vars_by_action(s_two->actions[i]);
    for (auto &item : set) {
      if (set_f.find(item) != set_f.end())
        set_use.insert(item);
    }
  }

  for (size_t i = 0; i < I_1.size(); i++) {
    if (unlikely(I_1[i] == -1))
      continue;
    auto set = g_action_info.search_vars_by_action(s_one->actions[i]);
    bool find_var = false;
    if (set.empty())
      find_var = true;

    for (auto var : set_use) {
      if (set.count(var) > 0) {
        find_var = true;
        break;
      }
    }
    if (find_var == false)
      I_1[i] = -1;
  }
  for (size_t i = 0; i < s_two->action_size; i++) {
    if (unlikely(I_2[i] == -1))
      continue;
    auto set = g_action_info.search_vars_by_action(s_two->actions[i]);
    bool find_var = false;
    if (set.empty())
      find_var = true;
    for (auto var : set_use) {
      if (set.count(var) > 0) {
        find_var = true;
        break;
      }
    }
    if (find_var == false)
      I_2[i] = -1;
  }
}

/* This function only use to color all location which is bot groupkey! So this
 * is a real rude way to decrease the complexity. I don't like but have no other
 * choice.*/
void color_notGroup_key(struct thread_info_scheduel_token *token,
                        std::vector<int32_t> &I_1, size_t my_tid) {
  /* Every one is same no other choice! */
  struct single_info_token *s_i_t = &token->list[my_tid];
  for (int32_t i = 0; i < s_i_t->action_size; i++) {
    if (!is_groupKey(s_i_t->actions[i])) {
      I_1[i] = -1;
    }
  }
}

/* The most dangerous way to decrease the complexity, which just give up all
 * reason just to make the plans real! There are a lot random part! This
 * function needs to come after the color_group!*/
/* size_n -> the size now.
 * size_w -> the size we want!
 */
void color_interesting_by_random(struct thread_info_scheduel_token *token,
                                 size_t loc, std::vector<int32_t> &I_1,
                                 size_t size_n, size_t size_w) {
  /* I mean it's so rude! This is too ungentlemanly. >_..*/
  if (loc > token->size) {
    printf("[helper] color_interesting_by_random: use loc beyond the range of "
           "token!");
    return;
  }

  int32_t rate = ((float)size_w / size_n) * 100;
  struct single_info_token *s_i = &token->list[loc];
  for (size_t i = 0; i < I_1.size(); i++) {
    if (I_1[i] == 1 && Rand(100) < rate) {
      I_1[i] = -1;
    }
  }
}

/* Check if you are currently eligible to generate the plan. */
std::pair<size_t, size_t> value_two_interesting_que(std::vector<int32_t> &I_1,
                                                    std::vector<int32_t> &I_2) {
  /* So rude!*/
  std::pair<size_t, size_t> res = {get_value_interesting_que(I_1),
                                   get_value_interesting_que(I_2)};
  if ((res.first > 5 || res.second > 5) && res.first + res.second > 30) {
    if (res.first > 8) {
      res.first = 8;
    }
    if (res.second > 8) {
      res.second = 8;
    }
  }
  auto value = factorial(res.first, res.second);
  /* NOTE: make sure there !*/
  while (value > PLAN_MAX && res.first > 0 && res.second > 0) {
    res.first--;
    res.second--;
    value = factorial(res.first, res.second);
  }
  return res;
}

/* Add initial plan to st_que! */
void add_initial_plan_st_que(std::vector<std::shared_ptr<SingleThread>> &st_que,
                             Ptr_i_que &I_q) {
  /* Just use in generate_que, be careful! */
  int counter = 0;
  for (const auto &v : *I_q) {
    st_que[counter]->init_signal_list_byV(v);
    counter++;
  }
  // std::cout << "st_ques.size()" << st_que.size() << std::endl;
  // for (size_t i = 0; i < st_que.size(); i++) {
  //   std::cout << "st_que[?]->size" << st_que[i]->size << std::endl;
  // }
}

/* Check the different data if follow the baic rule here. */
void check_if_obey_basic_rule(
    struct thread_info_scheduel_token *token, Ptr_i_que &I_q,
    std::vector<std::shared_ptr<SingleThread>> &st_que) {
  for (size_t i = 0; i < token->size; i++) {
    struct single_info_token *s_i_t = &token->list[i];
    size_t s_size = s_i_t->action_size;
    int64_t s_tid = s_i_t->tid;
    if (I_q->at(i).size() != s_size)
      FATAL("I_q don't fit the token info!");
    if (st_que[i]->size != s_size)
      FATAL("st_que[i]->size don't fit the token info!");
    if (st_que[i]->tid != s_tid)
      FATAL("st_que[i]->tid don't fit the token info!");
  }
}

/* Add several change to decrease the influence of path exploration! */
/* First of all, decrease the total same or part of same thread. Secondly,
 * analysis the lock part of thread que, maybe generate some interesting
 * part of that before the generate_que. Third, After decrease the useless
 * infomation in que, choose right stategy for final plan result. By the
 * way, We give each action in different color to determine if there a
 * potential window.*/

/* HEY! NOTE! We have remove main thread before generate_que by change t_n_c!*/
void generate_que(struct scheduel_result_list **result_ptr,
                  struct thread_info_scheduel_token *token,
                  struct thread_need_care *t_n_c) {
  std::shared_ptr<Working_info> w_info = std::make_shared<Working_info>();
  std::vector<std::shared_ptr<SingleThread>> st_que;
  // NEXT:
  size_t size_t_n_c = t_n_c->size;
  auto tid_number_map = std::make_shared<std::unordered_map<int64_t, size_t>>();
  int64_t j = 0;
  while (j < size_t_n_c) {
    tid_number_map->insert(std::make_pair(t_n_c->tid_list[j], j));
    j++;
  }
  generate_basic_info(token, w_info, st_que, t_n_c);

  std::unordered_map<size_t, size_t> st_map;
  size_t i = 0;
  for (auto const &st : st_que) {
    st_map.insert({st->tid, i++});
  }
#ifdef OUT_DEBUG
  std::cout << "[DEBUG] before color the ques." << std::endl;
#endif
  /* Do first color step here! */
  Ptr_i_que I_q = color_que_first(token, st_que);
  check_if_obey_basic_rule(token, I_q, st_que);
  add_initial_plan_st_que(st_que, I_q);
  /* Tons of structure! */
  std::set<std::pair<size_t, size_t>> res_set = {};
  w_info->generate_interesting_pair(res_set);
  std::set<std::pair<CK_SUM, size_t>> mem_cksum; /* Use to value plans. */
  std::vector<tid_ac_v> final_list;              /* Restore final plans.*/

#ifdef OUT_DEBUG
  std::cout << "[DEBUG] finish color the ques first step." << std::endl;
#endif

  for (const auto &p : res_set) {
    if (p.first == p.second)
      continue;
    std::vector<size_t> trans = {st_map[p.first], st_map[p.second]};
    auto first_loc = st_map[p.first]; /* Name is loc but the real meaning is loc
                                         in st_que or .. JIBIN: Wrong name! */
    auto second_loc = st_map[p.second];
    std::vector<std::pair<size_t, size_t>> temp = {};
    size_t max_size = get_max_size_from_ques(st_que, trans);
    auto I_q1 = (*I_q)[first_loc];
    auto I_q2 = (*I_q)[second_loc];

    color_mainQue_useless_part(token, I_q1, I_q2, first_loc, second_loc);
    color_que_useless_var(token, I_q1, I_q2, first_loc, second_loc);
    auto I_q_v1 = get_value_interesting_que(I_q1);
    auto I_q_v2 = get_value_interesting_que(I_q2);

    if ((I_q_v1 <= 0) || (I_q_v2 <= 0))
      continue;
    if (factorial(I_q_v1, I_q_v2) > 500) {
      if ((I_q_v1 + I_q_v2) > 15 || I_q_v1 > 10 || I_q_v2 > 10) {
        if (I_q_v1 > 8)
          color_notGroup_key(token, I_q1, first_loc);
        if (I_q_v2 > 8)
          color_notGroup_key(token, I_q2, second_loc);
      }
    }

    I_q_v1 = get_value_interesting_que(I_q1);
    I_q_v2 = get_value_interesting_que(I_q2);

    if (factorial(I_q_v1, I_q_v2) > 500) {
      auto new_pair = value_two_interesting_que(I_q1, I_q2);
      if (new_pair.first > I_q_v1 || new_pair.second > I_q_v2)
        FATAL("Something wrong within value_two_interesting_que! The new_pair "
              "numver is bigger!");
      if (new_pair.first != (unsigned long)I_q_v1) {
        color_interesting_by_random(token, first_loc, I_q1, I_q_v1,
                                    new_pair.first);
      }
      if (new_pair.second != (unsigned long)I_q_v2) {
        color_interesting_by_random(token, second_loc, I_q2, I_q_v2,
                                    new_pair.second);
      }
    }

    st_que[first_loc]->switch_signal_list(I_q1);
    st_que[second_loc]->switch_signal_list(I_q2);
    /* Finish one part of trash, It's another part now!（￣ε（#￣）*/
    create_que(st_que, trans, final_list, temp, max_size, 0, mem_cksum);
    st_que[first_loc]->switch_signal_list(I_q1);
    st_que[second_loc]->switch_signal_list(I_q2);
  }

  mem_cksum.clear();

  /* arrange and transfor the result*/
#ifdef OUT_DEBUG
  std::cout << "[DEBUG] final size: " << final_list.size() << std::endl;
#endif
  fprintf(stderr, "[JIBIN]: generate plan final size %ld", final_list.size());
  put_Scheduel_que_into_result(result_ptr, final_list, w_info, tid_number_map);
  g_single_time_info.move_and_build_result(final_list);
  g_single_time_info.add_info(res_set, w_info, st_que);
  final_list.clear();
#ifdef OUT_DEBUG_1
  std::cout << g_single_time_info.result.size() << std::endl;
  if (g_single_time_info.result.size() > 0) {
    std::cout << g_single_time_info.result[0]->size() << "\n";
    for (auto &item : *g_single_time_info.result[0]) {
      std::cout << item.first << std::endl;
    }
  }
#endif
}

void helper_free_scheduel_result_list(struct scheduel_result_list *space) {
  for (size_t i = 0; i < space->size; i++) {
    free(space->list[i]);
    space->list[i] = NULL;
  }
  free(space);
}

void build_action_info(const struct variable_array *const v_array) {
  for (size_t i = 0; i < v_array->size; i++) {
    const struct variable_container *const item = v_array->list[i];
    size_t v_name = (size_t)item->v_name;
    for (size_t j = 0; j < item->size; j++) {
      int op_count = item->op_list[j].count;
      g_action_info.add_action(op_count, v_name);
    }
  }
}

namespace HELPER__CC_TEST {
void test_cmp_tid_ac_v() {
  tid_ac_v first = {{1, 1}, {1, 2}, {2, 1}, {1, 3}, {2, 2}, {2, 3}};
  tid_ac_v second = {{1, 1}, {1, 2}, {2, 1}, {1, 3}, {2, 2}, {2, 3}};
  tid_ac_v third = {{1, 1}, {1, 2}, {2, 1}, {2, 2}, {1, 3}, {2, 3}};
  auto first_answer = cmp_tid_ac_v(&first, &second);
  auto second_answer = cmp_tid_ac_v(&first, &third);
  std::cout << "[TEST] test_cmp_tid_ac_v\n";
  assert(first_answer.second == -1);
  assert(second_answer.second == 3);
  std::cout << "[TEST] test_cmp_tid_ac_v success." << std::endl;
}

void build_token_for_test_1(struct thread_info_scheduel_token **token_ptr) {
  if ((*token_ptr) != NULL) {
    fprintf(stderr, "[ERROR] build_token_for_test error");
    exit(1);
  }
  static int32_t a1[] = {1, 2, 3, 4, 5};
  static int32_t a2[] = {5, 6, 7, 8, 9};
  (*token_ptr) = (struct thread_info_scheduel_token *)malloc(
      sizeof(struct thread_info_scheduel_token) +
      2 * sizeof(struct single_info_token));
  struct thread_info_scheduel_token *token = (*token_ptr);
  token->size = 2;
  token->list[0] = {
      .start = 0,
      .end = 1,
      .action_size = 5,
      .tid = 1,
      .actions = a1,
  };
  token->list[1] = {
      .start = 0,
      .end = 1,
      .action_size = 5,
      .tid = 2,
      .actions = a1,
  };
}

void test_check_if_obey2() {}

} // namespace HELPER__CC_TEST

namespace HELPER__CC_TEST {
void test_re_que() {
  std::cout << "[TEST] test_re_que\n";
  clear_helper();
  struct thread_info_scheduel_token *token = NULL;
  build_token_for_test_1(&token);
  build_g_filter_info_ctc_for_test1(12);
  build_action_info_for_test();
  struct scheduel_result_list *result = NULL;
  struct thread_need_care *t_n_c = (struct thread_need_care *)malloc(
      sizeof(struct thread_need_care) + 2 * sizeof(unsigned int));
  t_n_c->size = 2;
  t_n_c->tid_list[0] = 1;
  t_n_c->tid_list[1] = 2;
  generate_que(&result, token, t_n_c);
  std::cout << "[TEST] get final_list first time: "
            << g_single_time_info.result.size() << std::endl;
  ;
  helper_free_scheduel_result_list(result);
  result = NULL;
  for (size_t i = 0; i < 2 && i < g_single_time_info.result.size(); i++) {
    finish_one_que(i);
  }
}

void test_before_run() {
  struct Thread_info *t_info =
      (struct Thread_info *)malloc(sizeof(struct Thread_info));
  struct single_t_info *single_info = get_s_t(t_info);
  struct thread_info_scheduel_token *t_info_s_token =
      get_tis_token(t_info, single_info);
  free_s_t_i(single_info);
  free_thread_info_scheduel_token(t_info_s_token);
  free(t_info);
}

void init_g_filter_info_from_actions(ActionInfo &action_info) {}

} // namespace HELPER__CC_TEST

/* Check Thread_info if abey the basic rules! Return value Zero means OK, One
 * means wrong!*/
u8 check_t_info(struct Thread_info *t_info) {
  std::map<int64_t, size_t> thread_counter;
  for (size_t i = 0; i < MY_PTHREAD_CREATE_MAX; i++) {
    int64_t id = t_info->thread_count_to_tid[i];
    if (id == -1) {
      break;
    }
    thread_counter.insert({id, 0});
  }

  for (size_t i = 0; i < 4 * MY_PTHREAD_CREATE_MAX; i++) {
    int64_t id = t_info->thread_create_jion[i];
    if (id == 0)
      break;
    int64_t abs_thread_id = std::abs(id);
    if (thread_counter.find(abs_thread_id) == thread_counter.end()) {
      WARNF("[ERROR] check_t_info find one in thread_create_jion but not in "
            "thread_counter!\n");
      return 1;
    }
  }
  return 0;
}

/* Use when all the thing oncetime is done.*/
void exit_one_turn() {}

bool isNumber(std::string str) {
  std::stringstream sin(str);
  double d;
  char c;
  if (!(sin >> d))
    return false;

  if (sin >> c)
    return false;
  return true;
}

void build_g_filter_ctc() {
  char *hash_number = nullptr;
  hash_number = getenv("HASHNUMBER");
  if (!isNumber(std::string(hash_number)))
    FATAL("ERROR: WRONG! HASHNUMBER NOT NUMBER");
  int64_t hash_number_int = std::stoi(hash_number);

  for (size_t i = 0; i <= hash_number_int; i++) {
    g_filter_info.add_count_to_ckSum(i, R(i));
  }

#ifdef OUT_DEBUG
  g_filter_info.dump_ctc_info();
#endif
}

void build_g_filter(int64_t number) {
  srand(time(0));
  for (size_t i = 0; i < number; i++) {
    g_filter_info.add_count_to_ckSum(i, random() % (MAP_SIZE));
  }
}

void build_g_filter(int number, u_int16_t *code) {
  for (int i = 0; i < number; i++) {
    g_filter_info.add_count_to_ckSum(i, code[i]);
  }
#ifdef OUT_DEBUG
  g_filter_info.dump_ctc_info();
#endif
};

void test_helper() {
  build_action_info_for_test();
  int a1[] = {1, 2, 3, 4, 1, 2, 3, 4, 1};
  int a2[] = {4, 5, 6, 7};
  int a3[] = {7, 8, 9, 1, 2, 3};
  int a4[] = {8, 10, 6};
  struct thread_info_scheduel_token *token =
      (struct thread_info_scheduel_token *)malloc(
          sizeof(struct thread_info_scheduel_token) +
          4 * sizeof(struct single_info_token));
  token->size = 4;
  token->list[0] = {
      .start = 2,
      .end = 4,
      .action_size = 9,
      .tid = 102,
      .actions = a1,
  };
  token->list[1] = {
      .start = 5,
      .end = 6,
      .action_size = 4,
      .tid = 101,
      .actions = a2,
  };
  token->list[2] = {
      .start = 0,
      .end = 6,
      .action_size = 6,
      .tid = 200,
      .actions = a3,
  };
  token->list[3] = {
      .start = 0,
      .end = 6,
      .action_size = 3,
      .tid = 300,
      .actions = a4,
  };
  const unsigned int t_n_c_size = 4;
  struct thread_need_care *t_n_c = (struct thread_need_care *)malloc(
      sizeof(struct thread_need_care) + t_n_c_size * sizeof(unsigned int));
  struct scheduel_result_list *result = NULL;
  // g_filter_info.add_close_map_by_pair({1, 2}, 7);
  std::cout << "test_helper" << std::endl;
  g_filter_info.add_close_map_by_pair({1, 2}, 1);
  t_n_c->size = 4;
  t_n_c->tid_list[0] = 101;
  t_n_c->tid_list[1] = 102;
  t_n_c->tid_list[2] = 200;
  t_n_c->tid_list[3] = 300;
  HELPER__CC_TEST::build_g_filter_info_ctc_for_test1(30);
  generate_que(&result, token, t_n_c);
  helper_free_scheduel_result_list(result);
  result = NULL;
  generate_que(&result, token, t_n_c);
  helper_free_scheduel_result_list(result);
  result = NULL;
  leave_single_times();
  free(t_n_c);
  g_filter_info.clear();
  /* Test WindowSearchMap funtion. */
  HELPER__CC_TEST::test_WindowSearchMap();
  HELPER__CC_TEST::test_cmp_tid_ac_v();
  HELPER__CC_TEST::test_is_have_unallowed_situation();
  HELPER__CC_TEST::test_re_que();
}
/* 计算一列的hash值！ NOTE: the hascode still from g_filter_info! */
CK_SUM get_hash_seq() {
  CK_SUM res = 131;
  return res;
}

/* 0 means not find new thing! Share same function with fill_que_sch_exeInfo!
 * This can make sure that the que_entry without new cur will find the same que
 * 100%!
 */
u8 have_new_concurrent_per(struct Thread_info *t_info) {
  /* Used to store the imformation from t_info. */
  auto hash_result = get_concurHash_from_tInfo(t_info);
  return g_filter_info.search_con_set_or_add(hash_result) ? 0 : 1;
}

/* Generate the single_t_info for once time. */
struct single_t_info *get_s_t(struct Thread_info *t_info) {
  struct single_t_info *res = NULL;
  std::vector<size_t> thread_mem;
  size_t i = 0;
  for (; i < t_info->final_thread_number; i++) {
    size_t j = 0;
    for (; j < SHARE_MEMORY_ARRAY_SIZE; j++) {
      if (t_info->kp_thread_array[i][j] == -1)
        break;
    }
    thread_mem.push_back(j);
  }
  if (i == 0) {
    FATAL("[ERROR] get_s_t can't find the thread number from the t_info!\n");
  }
  if (thread_mem.size() != t_info->final_thread_number) {
    FATAL("[ERROR] get_s_t t_info->final_thread_number don't match! "
          "Thread_mem.size:%ld, t_info->t_info->final_thread_number:%ld",
          thread_mem.size(), t_info->final_thread_number);
  }
  i = 0;

  res = (struct single_t_info *)malloc(sizeof(struct single_t_info));
  res->thread_number = t_info->final_thread_number;
  res->thread_s_number =
      (int32_t *)malloc(sizeof(int32_t) * res->thread_number);
  int32_t *ptr_thread_action = res->thread_s_number;
  for (size_t k = 0; k < thread_mem.size(); k++) {
    ptr_thread_action[k] = thread_mem[k];
  }
  return res;
}
/* TODO: generate tnc from thread info! */
struct thread_need_care *get_t_n_c(struct Thread_info *t_info,
                                   struct single_t_info *s_t) {
  ;
  assert(s_t != NULL);
  struct thread_need_care *res = (struct thread_need_care *)malloc(
      sizeof(struct thread_need_care) +
      sizeof(unsigned int) * s_t->thread_number);
  res->size = s_t->thread_number;

  for (int32_t i = 0; i < s_t->thread_number; i++) {
    res->tid_list[i] = t_info->thread_count_to_tid[i];
  }
  return res;
}

/* There is always some guy don't use thread_jion, So the to main value is need
 * set by us!*/
int32_t fix_right_to_mian_end(struct thread_info_scheduel_token *token) {}

/* Generate struct thread_info_scheduel_token from t_info*/
struct thread_info_scheduel_token *
get_tis_token(const struct Thread_info *const t_info,
              struct single_t_info *s_t) {
  assert(s_t != NULL);
  std::vector<int32_t *> action_mem;
  std::vector<int64_t> thread_count_tid_mem;
  std::map<int64_t, size_t> tid_count;

  std::vector<std::pair<size_t, size_t>> to_main_loc;
  for (int16_t i = 0; i < s_t->thread_number; i++) {
    int32_t *temp_array =
        (int32_t *)malloc(sizeof(int32_t) * s_t->thread_s_number[i]);
    for (size_t j = 0; j < s_t->thread_s_number[i]; j++) {
      temp_array[j] = t_info->kp_thread_array[i][j];
    }
    action_mem.push_back(temp_array);
    int64_t temp_tid = t_info->thread_count_to_tid[i];
    thread_count_tid_mem.push_back(temp_tid);
    tid_count.insert({temp_tid, i});
    to_main_loc.push_back({t_info->thread_main_eara[i][0] == -1
                               ? 0
                               : t_info->thread_main_eara[i][0],
                           t_info->thread_main_eara[i][1] == -1
                               ? 0
                               : t_info->thread_main_eara[i][1]});
  }

  struct thread_info_scheduel_token *res =
      (struct thread_info_scheduel_token *)malloc(
          sizeof(struct thread_info_scheduel_token) +
          sizeof(struct single_info_token) * s_t->thread_number);
  res->size = s_t->thread_number;

  for (size_t i = 0; i < res->size; i++) {
    res->list[i].action_size = s_t->thread_s_number[i];
    res->list[i].tid = thread_count_tid_mem[i];
    res->list[i].actions = action_mem[i];
    /* Add main and to_main info. */
    if (thread_count_tid_mem[i] == t_info->mainid) {
      res->list[i].is_main = 1;
      /* The maintid don't need this!*/
      res->list[i].to_main[0] = 0;
      res->list[i].to_main[1] = 0;
    } else {
      res->list[i].is_main = 0;
      res->list[i].to_main[0] = to_main_loc[i].first;
      res->list[i].to_main[1] = to_main_loc[i].second;
    }
  }

  std::vector<int64_t> create_jion_mem;
  for (size_t i = 0; i < 4 * MY_PTHREAD_CREATE_MAX; i++) {
    if (unlikely(t_info->thread_create_jion[i] == 0))
      break;
    create_jion_mem.push_back(t_info->thread_create_jion[i]);
  }
  int64_t main_tid = t_info->mainid;
  /* This is useless right now, since we changed the traceEnd function.*/
  // if (create_jion_mem.back() != -1 * main_tid) {
  //   create_jion_mem.push_back(-1 * main_tid);
  // }
  /* In case there is something wrong with the t_info*/
  if (create_jion_mem.size() < 1) {
    FATAL(
        "[ERROR] get_tis_token can't find enough value for create_jion_mem!\n");
  }

  /* Get the group! */
  u32 counter = 0;
  /* Special for 0 zero.*/
  int64_t zero_c_j_m = create_jion_mem[0];
  res->list[tid_count[zero_c_j_m]].start = counter;

  for (size_t i = 1; i < create_jion_mem.size(); i++) {
    int64_t tid_number = create_jion_mem[i];
    int8_t add = (tid_number ^ create_jion_mem[i - 1]) >= 0 ? 0 : 1;
    counter += add;
    size_t loc = tid_count[std::abs(tid_number)];
    if (tid_number > 0) {
      res->list[loc].start = counter;
    } else if (tid_number < 0) {
      res->list[loc].end = counter;
    }
  }

  int32_t count_main = -1;
  size_t token_size = res->size;
  for (size_t i = 0; i < token_size; i++) {
    struct single_info_token *s_i = &res->list[i];
    if (s_i->is_main)
      count_main = s_i->action_size;
  }
  if (count_main < 0)
    FATAL("count_main can't be smaller than zero!");
  size_t now = 0;
  while (now < token_size) {
    struct single_info_token *s_i = &res->list[now];
    if (!s_i->is_main && s_i->to_main[1] < s_i->to_main[0]) {
      s_i->to_main[1] = count_main;
      if (s_i->to_main[1] < s_i->to_main[0]) {
        FATAL("The compare failed, the to_main[1]:%d is smaller than "
              "to_main[0]:%d. "
              "This is unaccpeted! i: %d, token_size:%d",
              s_i->to_main[1], s_i->to_main[0], now, token_size);
      }
    }
    now++;
  }

  return res;
}

void free_thread_info_scheduel_token(struct thread_info_scheduel_token *f) {
  assert(f != NULL);
  assert(f->size <= MY_PTHREAD_CREATE_MAX);
  for (size_t i = 0; i < f->size; i++) {
    free(f->list[i].actions);
  }
  free(f);
}

void free_s_t_i(struct single_t_info *f) {
  free(f->thread_s_number);
  free(f);
}

void build_g_actionInfo(struct function_array *f_array,
                        struct variable_array *v_array,
                        struct distance_array *d_array,
                        struct groupKey_array *g_array) {
  g_action_info = {};
  build_actionInfo(f_array, v_array, d_array, g_array, g_action_info);
#ifdef OUT_DEBUG_DUMP
  g_action_info.dump();
#endif
}

struct cfg_info_token *
generate_finished_cfg(struct thread_info_scheduel_token *t) {
  std::set<CK_SUM> cks;
  for (size_t i = 0; i < t->size; i++) {
    struct single_info_token *s_t = &t->list[i];
    if (s_t->action_size == 0 && s_t->action_size == 1)
      continue;
    for (int32_t j = 0; s_t->action_size > 1 && j < s_t->action_size - 1; j++) {
      int32_t first = s_t->actions[j];
      int32_t second = s_t->actions[j + 1];
      auto hash_code = g_filter_info.get_ck_sum_by_two_131(first, second);
      cks.insert(hash_code);
    }
  }
  u32 cfg_token_size = cks.size();
  struct cfg_info_token *token = (struct cfg_info_token *)malloc(
      sizeof(struct cfg_info_token) + sizeof(u_int16_t) * cfg_token_size);
  token->size = cfg_token_size;
  size_t tmp_count = 0;
  for (auto item : cks) {
    token->cksums[tmp_count] = item;
    tmp_count++;
  }
  tmp_count = 0;
  return token;
}

/* predicted cfg result*/
struct cfg_info_token *
generate_cfg_token(struct thread_info_scheduel_token *t) {
  /*step1 : generate potential pair. */
  std::set<std::pair<size_t, size_t>> mem;
  for (size_t i = 0; i < t->size; i++) {
    struct single_info_token *si = &t->list[i];
    for (size_t j = 0; si->action_size > 1 && j < si->action_size - 1; j++) {
      mem.insert({si->actions[j], si->actions[j + 1]});
    }
  }

  /*step2 : find potential cfgs*/
  std::set<std::pair<size_t, size_t>> mem_potential;
  for (auto &item : mem) {
    auto v = g_action_info.get_distance_byPair(item);
    auto v_size = v.size();
    if (v_size == 0)
      continue;
    mem_potential.insert({item.first, v[0]});
    mem_potential.insert({v[v_size - 1], item.second});
    for (size_t i = 0; i < v.size() - 1; i++) {
      mem_potential.insert({v[i], v[i + 1]});
    }
  }
  /*step3 : get_result*/
  std::set<CK_SUM_CFG> cfg_cks;
  for (auto &item : mem_potential) {
    if (mem.find(item) != mem.end())
      continue;
    cfg_cks.insert(
        g_filter_info.get_ck_sum_by_two_131(item.first, item.second));
  }
  /*step4 : put result back */
  u32 cfg_token_size = cfg_cks.size();
  struct cfg_info_token *token = (struct cfg_info_token *)malloc(
      sizeof(struct cfg_info_token) + sizeof(u_int16_t) * cfg_token_size);
  token->size = cfg_token_size;
  size_t tmp_count = 0;
  for (auto item : cfg_cks) {
    token->cksums[tmp_count] = item;
    tmp_count++;
  }
  tmp_count = 0;
  return token;
}

/* return 1 means good plan! 0 means not. */
int8_t check_plan_good(const std::shared_ptr<tid_ac_v> plan, size_t number) {
  for (size_t i = 0; i < plan->size() - 1; i++) {
    if ((*plan)[i].first == (*plan)[i + 1].first)
      continue;
    size_t final_loc = i + 2;
    while (final_loc < plan->size()) {
      if ((*plan)[final_loc].first == (*plan)[i].first)
        break;
    }
    if (final_loc == plan->size())
      continue;
    std::vector<OP_TYPE> ops;
    for (size_t j = i + 1; j < final_loc; j++) {
      ops.push_back((*plan)[j].second);
    }
    if (g_filter_info.search_close_map_by_pair_v(
            {(*plan)[i].second, (*plan)[final_loc].second}, ops))
      return 0;
  }
  return 1;
}

/* return 1 means good plan! 0 means not.
  Used for avoid useless plan after some plan already failed.
*/
int8_t if_good_plan_now(size_t number) {
  if (g_single_time_info.get_bad_flag() != true)
    return 1;
  return check_plan_good(g_single_time_info.result.at(number), number);
}

/* Find the wrong place of the exe result. */
std::pair<std::pair<int64_t, int64_t>, OP_TYPE>
check_plan_resultc_CC(std::vector<struct Kp_action_info> mem, size_t number) {
  using res_type = std::pair<std::pair<int64_t, int64_t>, OP_TYPE>;
  res_type res = {{-1, -1}, 0};
  std::pair<size_t, size_t> loc = {};
  std::shared_ptr<tid_ac_v> plan = g_single_time_info.result.at(number);
  assert(mem.size() == plan->size());
  size_t wrong_loc = 0;
  while (wrong_loc < mem.size()) {
    if (mem[wrong_loc].loc == (*plan)[wrong_loc].second) {
      wrong_loc++;
    } else {
      break;
    }
  }
  if (wrong_loc == mem.size())
    return res;
  int32_t front = -1, end = -1;
  size_t thread_name_loc = (*plan)[wrong_loc].second, tmp = thread_name_loc;
  for (tmp = tmp - 1; tmp >= 0; tmp--) {
    if ((*plan)[tmp].first != tmp) {
      front = tmp;
    }
  }
  for (tmp = thread_name_loc + 1; tmp < plan->size(); tmp++) {
    if ((*plan)[tmp].first != tmp) {
      end = tmp;
    }
  }
  if (front == -1 || end == -1) /* Do nothing. */
    return res;
  return {{(*plan)[front].second, (*plan)[end].second},
          (*plan)[thread_name_loc].second};
}

int8_t check_plan_result(struct Thread_info *info, struct scheduel_result *plan,
                         size_t number) {
  std::vector<struct Kp_action_info> mem;
  for (size_t i = 0; i < info->kp_mem_size; i++) {
    mem.push_back(info->kp_mem[i]);
  }
  const std::shared_ptr<tid_ac_v> plan_v = g_single_time_info.result.at(number);
  if (mem.size() != plan->entry_size[0] + plan->entry_size[1] ||
      mem.size() != plan_v->size()) {
    fprintf(stderr,
            "[ERROR] check_plan_result runing result is absuloutely wrong!\n");
    fprintf(stderr, "mem.size %ld; plan.size %d", mem.size(),
            plan->entry_size[0] + plan->entry_size[1]);
    exit(100);
  }
  std::pair<std::pair<int64_t, int64_t>, OP_TYPE> check_result =
      check_plan_resultc_CC(mem, number);
  if (check_result.first.first == -1)
    return 1;
  g_filter_info.add_close_map_by_pair(check_result.first, check_result.second);
  g_single_time_info.bad_plan_flag = true;
  return 0;
}

void finish_one_plan_success(size_t number, struct Thread_info *t_info,
                             size_t kp_size) {
  auto kp = &t_info->kp_mem;
  std::shared_ptr<tid_ac_v> ptr = std::make_shared<tid_ac_v>();

  for (int32_t i = 0; i < t_info->kp_mem_size; i++) {
    u32 threadid = t_info->kp_mem[i].threadid;
    u32 loc = t_info->kp_mem->loc;
    ptr->push_back({threadid, loc});
  }
  g_single_time_info.add_cks(ptr);
}

/* generate cks by thread from a vector. */
std::pair<CK_SUM, size_t>
generate_thread_cks_v(const std::vector<int32_t> &mem) {
  CK_SUM init = 131;
  for (const auto &number : mem) {
    CK_SUM now = g_filter_info.get_cksum(number);
    init = g_filter_info.hash_two(init, now);
  }
  return {init, mem.size()};
}

/* store some info before schedule to check if the program run as same way!
   return 1 as work well.
*/
int8_t store_info_preSchedule(struct Thread_info *t_info) {
  std::vector<std::vector<int32_t>> mem;
  for (size_t i = 0; i < t_info->final_thread_number; i++) {
    std::vector<int32_t> mem_t;
    for (size_t j = 0; j < SHARE_MEMORY_ARRAY_SIZE; j++) {
      if (t_info->kp_thread_array[i][j] == -1)
        break;
      mem_t.push_back(t_info->kp_thread_array[i][j]);
    }
    mem.emplace_back(mem_t);
  }
  for (const auto &v : mem) {
    g_single_time_info.add_thread_cks(generate_thread_cks_v(v));
  }
  return 1;
}

/* Init the same queEntry */
int8_t fill_queEntry_sInfo_with_another(struct queue_entry *q,
                                        struct queue_entry *another) {
  auto q_same_info = g_filter_info.search_qEntry_info(another->sch_number);
  int64_t loc = 1;
  if (q->sch_number == UINT32_MAX) {
    loc = g_filter_info.q_entry_info_map.size();
    q->sch_number = loc;
  }
  /* If q_same_info is tatolly useless. */
  if (q_same_info == useless_que_entry_cpp) {
    g_filter_info.add_q_entry(loc, useless_que_entry_cpp);
    return 0;
  }

  auto ptr = init_que_entry(q, loc);
  ptr->q_entry_ptr = q;
  ptr->number = loc;
  ptr->meaningful = q_same_info->meaningful;
  ptr->windows_cks = q_same_info->windows_cks;
  ptr->cfg_size = q_same_info->cfg_size;
  ptr->windows_size = q_same_info->windows_size;
  ptr->cfg_cks = q_same_info->cfg_cks;
  ptr->trace_mem = q_same_info->trace_mem;
  ptr->retire_windows = q_same_info->retire_windows;
  ptr->retire_cfg = q_same_info->retire_cfg;
  g_filter_info.add_q_entry(loc, ptr);
  return 1;
}

/* update queEntry scheduel info!*/
int8_t fill_queEntry_sInfo(struct queue_entry *q,
                           struct cfg_info_token *cfg_tInfo_token) {
  int64_t loc = -1;
  if (q->sch_number == UINT32_MAX) {
    loc = g_filter_info.q_entry_info_map.size();
    q->sch_number = loc;
  } else {
    fprintf(stderr, "[ERROR] There is already a number for q!\n");
    exit(100);
  }
  if (loc == -1)
    return 0;
  auto ptr = init_que_entry(q, loc);
  /* add windows!*/
  for (const auto &item : g_single_time_info.single_wCks) {
    ptr->add_w_cks(item);
  }
  ptr->windows_size = ptr->windows_cks.size();
  /* add cfg info*/

  for (size_t i = 0; i < cfg_tInfo_token->size; i++) {
    if (is_finished_cfg(cfg_tInfo_token->cksums[i]))
      continue;
    ptr->add_c_cks({cfg_tInfo_token->cksums[i], 2});
  }
  ptr->cfg_size = ptr->cfg_cks.size();
  g_filter_info.add_q_entry(loc, ptr);
  return 1;
}

/* */
int8_t is_finished_cfg(u16 ck_sum) {
  return g_filter_info.find_finished_cfg(ck_sum, 2);
}
/*check program if work in same way as before schedule.
  return 1 as ok!
 */
int8_t check_after_schedule(struct Thread_info *t_info) {
  std::unordered_map<int32_t, size_t> map_mem;
  std::vector<std::vector<int32_t>> mem;
  struct Kp_action_info *ptr = t_info->kp_mem;
  for (size_t i = 0; i < t_info->kp_mem_size; i++) {
    if (map_mem.find(ptr[i].threadid) == map_mem.end()) {
      map_mem[ptr[i].threadid] = mem.size();
      mem.push_back({});
    }
    mem[map_mem[ptr[i].threadid]].push_back(ptr[i].loc);
  }
  for (auto &v : mem) {
    auto res = generate_thread_cks_v(v);
    if (!g_single_time_info.find_thread_ck(res))
      return 0;
  }
  return 1;
}

void put_cfg_token_toG(struct cfg_info_token *cfg_token) {
  if (g_single_time_info.cfg != nullptr) {
    fprintf(stderr, "[ERROR] There is already cfg!\n");
    exit(100);
  }
  g_single_time_info.cfg = cfg_token;
}

void push_finish_cfg_toG(struct cfg_info_token *cfg_token) {
  g_single_time_info.cfg_finish = cfg_token;
}

void search_q_entry_cpp(size_t number) {
  if (!g_filter_info.q_entry_info_map.count(number)) {
    fprintf(stderr, "[ERROR] ");
    exit(100);
  }
}

std::shared_ptr<Que_entry_cpp> search_g_qEntry_info(struct queue_entry *q) {
  return g_filter_info.search_qEntry_info(q->sch_number);
}

int8_t update_q_window(struct queue_entry *q) {
  u64 fav_factor = q->exec_us * q->len;
  auto q_info = search_g_qEntry_info(q);
  if (q_info->meaningful == false) {
    fprintf(stderr, "[ERROR] transfer useless q info!\n");
#ifdef DEBUG
    exit(100);
#endif
    return 0;
  }
  std::vector<ck_size> mem;
  for (const auto &item : q_info->windows_cks) {
    if (g_filter_info.find_q_by_w(item)) {

      struct queue_entry *here_ptr = g_filter_info.search_q_by_w(item);
      if (fav_factor > here_ptr->exec_us * here_ptr->len) {
        mem.push_back(item);
        continue;
      }

      auto q_temp_info = search_g_qEntry_info(here_ptr);

      q_temp_info->retire_w(item);
    }
    g_filter_info.set_w_cks(q, item);
  }
  for (auto &item : mem) {
    q_info->retire_w(item);
  }
  return 1;
}

int8_t update_q_cfg(struct queue_entry *q) {
  u64 fav_factor = q->exec_us * q->len;
  auto q_info = search_g_qEntry_info(q);
  if (q_info->meaningful == false) {
    fprintf(stderr, "[ERROR] transfer useless q info!\n");
#ifdef DEBUG
    exit(100);
#endif
    return 0;
  }
  std::vector<ck_size> mem;
  for (const auto &ck : q_info->cfg_cks) {
    if (g_filter_info.find_q_by_c(ck)) {

      struct queue_entry *here_ptr = g_filter_info.search_q_by_c(ck);
      if (fav_factor > here_ptr->exec_us * here_ptr->len) {
        mem.push_back(ck);
        continue;
      }

      auto q_tmp = search_g_qEntry_info(here_ptr);
      q_tmp->retire_c(ck); /* USE retire function! */
    }
    g_filter_info.set_c_cks(q, ck);
  }
  for (auto &ck : mem) {
    q_info->retire_c(ck);
  }
  return 1;
}

/* Finish one scheduel here!*/
void finish_one_scheduel_run(struct cfg_info_token *cfg_token) {
  auto ptr_finished_cfg = cfg_token;
  for (size_t i = 0; i < ptr_finished_cfg->size; i++) {
    finish_cfg(ptr_finished_cfg->cksums[i]);
  }
  g_single_time_info.clear();
}

/* If not a new que, (JIBIN:) don someting! */
int8_t update_q_exe_time(u32 ck_sum, u32 count) {
  if (!g_filter_info.is_there_exeQueEntry(ck_sum, count)) {
#ifdef DEBUG
    FATAL("[ERROR] There is no que you looking for! Something happened to "
          "virgins");
#endif
    return 0;
  }
  g_filter_info.add_q_exe_times(ck_sum, count);
  g_filter_info.exe_times += 1;
  return 1;
}

/* Init q info in different set! */
int8_t insert_q_info_set(struct queue_entry *q, u8 *trace_bits) {
  if (q->sch_interesting == 0 && q->trace_mini == 0)
    return 0;
  if (q->have_insert_q_info_cpp == 1) {
    return 0;
  }
  auto q_info = search_g_qEntry_info(q);
  if (q_info->meaningful == false) {
    fprintf(stderr, "[ERROR] Use meanless q_info! \n");
#ifdef DEBUG
    exit(100);
#endif
    return 0;
  }
  u32 i;
  for (i = 0; i < MAP_SIZE; i++) {
    if (trace_bits[i]) {
      g_filter_info.add_q_toT(i, 2, q);
    }
    q_info->add_trace({i, 2});
  }
  for (const auto &item : q_info->cfg_cks) {
    g_filter_info.add_q_toC(item.first, item.second, q);
  }
  for (const auto &item : q_info->retire_cfg) {
    g_filter_info.add_q_toC(item.first, item.second, q);
  }
  for (const auto &item : q_info->windows_cks) {
    g_filter_info.add_q_toW(item.first, item.second, q);
  }
  for (const auto &item : q_info->retire_windows) {
    g_filter_info.add_q_toW(item.first, item.second, q);
  }
  q->have_insert_q_info_cpp = 1;
  return 1;
}

int8_t insert_q_exe(u32 ck_sum, u32 count, struct queue_entry *q) {
  if (g_filter_info.is_there_exeQueEntry(ck_sum, count)) {
    WARNF("You are inserting a existing entry node!");
#ifdef DEBUG
    exit(100);
#endif
    return 0;
  }
  g_filter_info.add_q_exe(ck_sum, count, q);
  return 1;
}

int8_t finish_cfg(u16 ck_sum) { return g_filter_info.cfg_finish(ck_sum, 2); }

/* When we find a q become meaningless the counter in
 */
/* The number of window and trace_bit never decrease!*/
int8_t minus_one_q(struct queue_entry *q) {
  if (!(q->sch_interesting == 0 && q->tr_intersting == 0)) {
    return 0;
  }
  auto q_info = search_g_qEntry_info(q);
  if (q_info->meaningful == false) {
    fprintf(stderr, "[ERROR] Using meaningless q_info in minus_one_q \n");
    return 0;
  }
  for (const auto &item : q_info->cfg_cks) {
    g_filter_info.remove_q_fromC(item.first, item.second, q);
  }
  for (const auto &item : q_info->retire_cfg) {
    g_filter_info.remove_q_fromC(item.first, item.second, q);
  }
  for (const auto &item : q_info->windows_cks) {
    g_filter_info.remove_q_fromW(item.first, item.second, q);
  }
  for (const auto &item : q_info->retire_windows) {
    g_filter_info.remove_q_fromW(item.first, item.second, q);
  }
  for (auto &item : q_info->trace_mem) {
    g_filter_info.remove_q_fromT(item, 2, q);
  }
  q_info->meaningful = false;

#ifdef TSAFL_LESS_MEMORY
  q_info = useless_que_entry_cpp;
#endif
  return 1;
}

/* WAITING. */
void after_trim_q(struct queue_entry *q, u8 *trace_bit, int8_t live) {
  if (!live)
    return;
  auto q_info = search_g_qEntry_info(q);
  if (q_info->meaningful == false) {
    insert_q_info_set(q, trace_bit);
  }
  return;
}

int8_t set_queueEntry_interesting_WandC() {
  for (const auto &pair : g_filter_info.cks_q_w) {
    pair.second->sch_interesting = 1;
  }
  for (const auto &pair : g_filter_info.cks_q_c) {
    pair.second->sch_interesting = 1;
  }
}

/* unfinish clean que!*/
int8_t clean_que(struct queue_entry *que) {
  auto q = que;
  while (q) {
    q = q->next;
    if (q->sch_interesting == 0 && q->tr_intersting == 0) {
      minus_one_q(q);
    }
  }
  return 1;
}

void set_init_cAndW() {
  init_cfg_number = g_filter_info.finish_cfg.size();
  init_window = g_filter_info.q_w_mem.size();
}

float calculate_score_factor_sch(struct queue_entry *q) {
  float r_cfg, r_t, r_w = 1.0;
  float cfg_size = g_filter_info.q_c_mem.size() +
                   g_filter_info.finish_cfg.size() - init_cfg_number;
  float t_size = g_filter_info.q_t_mem.size();
  float w_size = g_filter_info.q_w_mem.size();
  r_cfg = w_size / cfg_size * cfg_size;
  r_t = w_size / t_size * t_size;
  r_w = r_w / w_size;
  float t_score, w_score, c_score;
  auto q_info = search_g_qEntry_info(q);
  if (!q_info->meaningful) {
    fprintf(stderr, "[ERROR] Useless q_info in calculate_score_factor_sch. \n");
    exit(100);
  }
  c_score = 0;
  for (auto item : q_info->cfg_cks) {
    if (g_filter_info.q_c_mem.count(item.first) == 0) {
      fprintf(stderr, "[ERROR] find uncoverd ck in cfg_cks\n");
      exit(100);
    }
    auto size_tmp = g_filter_info.q_c_mem[item.first].size();
    c_score += 1.0f / size_tmp;
  }

  t_score = 0;
  for (auto &item : q_info->trace_mem) {
    if (g_filter_info.q_t_mem.count(item) == 0) {
      fprintf(stderr, "[ERROR] find uncoverd ck in q_t_mem\n");
      exit(100);
    }
    auto size_tmp = g_filter_info.q_t_mem[item].size();
    t_score += 1.0f / size_tmp;
  }

  w_score = 0;
  for (auto &item : q_info->windows_cks) {
    if (g_filter_info.q_w_mem.count(item) == 0) {
      fprintf(stderr, "[ERROR] find uncoverd ck in q_w_mem\n");
      exit(100);
    }
    auto size_tmp = g_filter_info.q_w_mem[item].size();
    w_score += 1.0f / size_tmp;
  }
  return t_score * r_t + w_score * r_w + c_score * r_cfg;
}

u32 get_target_splicing_limit() {
  u32 res = 0;
  if (que_meaningful_size <= 2)
    return que_meaningful_size;
  res = que_meaningful_size / 2;
  if (res > MAX_TARGET_SP)
    res = MAX_TARGET_SP;
  return res;
}

std::tuple<size_t, size_t, size_t> comaper_two_q_info(struct queue_entry *q1,
                                                      struct queue_entry *q2) {
  if (q1 == q2)
    return {-1, -1, -1};
  auto q1_info = search_g_qEntry_info(q1);
  auto q2_info = search_g_qEntry_info(q2);
  size_t number_t = 0;
  for (const auto &item : q1_info->trace_mem) {
    if (q2_info->trace_mem.count(item) == 0)
      number_t++;
  }
  size_t number_c = 0;
  for (const auto &item : q1_info->cfg_cks) {
    if (q2_info->cfg_cks.count(item) == 0)
      number_c++;
  }
  size_t number_w = 0;
  for (const auto &item : q1_info->windows_cks) {
    if (q2_info->windows_cks.count(item) == 0)
      number_w++;
  }
  return {number_t, number_w, number_c};
}

int64_t get_score_splicing(struct queue_entry *q1, struct queue_entry *q2,
                           float r_t, float r_w, float r_cfg) {
  if (q1 == q2)
    return -1;
  size_t number_t, number_c, number_w;
  std::tie(number_t, number_w, number_c) = comaper_two_q_info(q1, q2);
  return r_t * number_t + r_cfg * number_c + r_w * number_w;
  return 1;
}

using score_quePtr = std::pair<float, struct queue_entry *>;

int64_t get_splicing_targets(u32 targets_size, struct queue_entry **targets) {
  u32 final_size;
  float r_cfg, r_t, r_w = 1.0;
  float cfg_size = g_filter_info.q_c_mem.size() +
                   g_filter_info.finish_cfg.size() - init_cfg_number;
  float t_size = g_filter_info.q_t_mem.size();
  float w_size = g_filter_info.q_w_mem.size();
  r_cfg = w_size / cfg_size;
  r_t = w_size / targets_size;
  int counter = 0;
  struct cmpare_pair {
    bool operator()(score_quePtr a, score_quePtr b) {
      return a.first > b.first;
    }
  };
  std::priority_queue<score_quePtr, std::vector<score_quePtr>, cmpare_pair> com;
  struct queue_entry *q = queue;
  while (q) {
    if (q == queue_cur) {
      q = q->next;
      continue;
    }
    score_quePtr ptr;
    ptr.first = get_score_splicing(q, queue_cur, r_t, r_w, r_cfg);
    ptr.second = q;
    if (counter < final_size) {
      com.push(ptr);
    } else {
      if (ptr.first < com.top().first) {
        q = q->next;
        continue;
      } else {
        com.pop();
        com.push(ptr);
      }
    }
    counter++;
    q = q->next;
  }

  /* some time it just happened. */
  if (com.size() < targets_size) {
    final_size = com.size();
  }
  for (size_t i = 0; i < final_size; i++) {
    targets[i] = com.top().second;
    com.pop();
  }
  return final_size;
}

u32 get_max_count_action() { return g_action_info.get_max_count(); }

/* Set q->sch_exec_cksum and q->sch_exec_size by t_info. */
void fill_que_sch_exeInfo(struct Thread_info *t_info, struct queue_entry *q) {
  std::set<CK_SUM> set;
  auto res = get_concurHash_from_tInfo(t_info);
  q->sch_exec_cksum = res.first;
  q->sch_exec_size = res.second;
}