#define LLVM_11
#define GROUP_DEBUG1

#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/Analysis/CallGraph.h"
#include "llvm/Analysis/PostDominators.h"
#include "llvm/IR/Constant.h"
#include "llvm/IR/DebugInfo.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Pass.h"
#include "llvm/Support/Compiler.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include "llvm/Transforms/Utils/Local.h" // for FindDbgAddrUses
#include <algorithm>
#include <boost/algorithm/string.hpp>
#include <cassert>
#include <chrono>
#include <cstdlib>
#include <ctime>
#include <cxxabi.h>
#include <dlfcn.h>
#include <fstream>
#include <iostream>
#include <llvm/Analysis/CFG.h>      // for CFG
#include <llvm/IR/BasicBlock.h>     // for BasicBlock
#include <llvm/IR/CFG.h>            // for CFG
#include <llvm/IR/Function.h>       // for Function
#include <llvm/IR/GlobalVariable.h> // for GlobalVariable
#include <llvm/IR/InstIterator.h>   // for inst iteration
#include <llvm/IR/InstrTypes.h>     // for TerminatorInst
#include <llvm/IR/IntrinsicInst.h>  // for intrinsic instruction
#include <llvm/IR/Module.h>         // for Module
#include <llvm/IR/User.h>
#include <llvm/IRReader/IRReader.h> // for isIRFiler isBitcode
#include <llvm/Pass.h>
#include <llvm/Support/CommandLine.h>
#include <map>
#include <set>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <sys/resource.h> // increase stack size
#include <unistd.h>
#include <vector>

using namespace llvm;
using namespace llvm;
using namespace std::chrono;
using namespace std;

// Lock analysis
#define LockFunctionNum 6
char LockFunctionString[][25] = {
    "Lock",    "mutex_lock", "task_lock", "PR_RWLock_Rlock", "PR_RWLock_Wlock",
    "PR_Lock",
};
char UnlockFunctionString[][25] = {"Unlock",      "mutex_unlock",
                                   "task_unlock", "PR_RWLock_Unlock",
                                   "PR_Unlock",   "UK"};

// Manually Marked Interesting Functions
#define MarkedInterestingNum 1
char MarkedInterestingString[][30] = {"js_ClearContextThread", "UK"};

#define DEBUG_TYPE "GROUPKEY"

namespace {
struct GroupKey : public ModulePass {

  static char ID; // Pass ID, replacement for typeid

  GroupKey() : ModulePass(ID) {}

  bool runOnModule(Module &M) override;

  StringRef getPassName() const override { return "GroupKey"; }
};
} // namespace

inline const char *get_funcname(const char *src) {
  int status = 99;
  const char *f = abi::__cxa_demangle(src, nullptr, nullptr, &status);
  return f == nullptr ? src : f;
}

std::string getSourceLoc(const Value *val) {
  if (val == NULL)
    return "empty val";

  std::string str;
  raw_string_ostream rawstr(str);
  if (const Instruction *inst = dyn_cast<Instruction>(val)) {
    if (isa<AllocaInst>(inst)) {
#ifdef LLVM_4 /* LLVM 4.0 */
      DbgDeclareInst *DDI =
          llvm::FindAllocaDbgDeclare(const_cast<Instruction *>(inst));
      if (DDI) {
        DIVariable *DIVar = cast<DIVariable>(DDI->getVariable());
        rawstr << DIVar->getLine();
      }
#else /* LLVM 6.0+ */
      for (DbgInfoIntrinsic *DII :
           FindDbgAddrUses(const_cast<Instruction *>(inst))) {
        if (DbgDeclareInst *DDI = dyn_cast<DbgDeclareInst>(DII)) {
          DIVariable *DIVar = cast<DIVariable>(DDI->getVariable());
          rawstr << DIVar->getLine();
          break;
        }
      }
#endif
    } else if (MDNode *N =
                   inst->getMetadata("dbg")) { // Here I is an LLVM instruction
      DILocation *Loc = cast<DILocation>(N);   // DILocation is in DebugInfo.h
      unsigned Line = Loc->getLine();
      StringRef File = Loc->getFilename();
      rawstr << File << ":" << Line;
      // StringRef Dir = Loc->getDirectory();
      // rawstr << Dir << "/" << File << ":" << Line;
    }
  } else {
    // Can only get source location for instruction, argument, global var or
    // function.
    rawstr << "N.A.";
  }
  return rawstr.str();
}

uint16_t string_hash_encoding(const char *str) {
  uint16_t encodingNum = 26;
  for (int k = 0; k < strlen(str); k++) {
    encodingNum *= int(str[k]);
    encodingNum = encodingNum + int(str[k]);
  }
  return encodingNum;
}

bool GroupKey::runOnModule(Module &M) {

  // std::vector<std::string> locs_v;
  std::set<std::string> locs_set;
  std::map<std::string, std::set<std::string>> locs_map;

  /* read config.txt */

  char *ConfigTxt = NULL;
  std::string ConFileName;
  int ConcurrencyInsteresting = 0;

  ConfigTxt = getenv("ConfigTXT");
  if (ConfigTxt == NULL || strcmp(ConfigTxt, " ") == 0) {
    ConcurrencyInsteresting = 0;
    outs() << "[DEBUG] There is no ConfigTxt!";
    return false;
  }

  if (ConfigTxt != NULL && ConfigTxt[0] != '.' && ConfigTxt[0] != ' ') {
    ConFileName = std::string(ConfigTxt);
  } else {
    outs() << "[DEBUG] Please provide right ConfigTxt name"
           << "\n";
    return false;
  }

#ifdef GROUP_DEBUG
  outs() << "ConfigTxt: " << ConfigTxt << "ConFileName" << ConFileName << '\n';
#endif
  ifstream Confile{};
  Confile.open(ConFileName, ios::binary);
  if (Confile.fail()) {
    outs() << "[DEBUG] Failed to open ConfigTxt. Config:" << ConFileName
           << "\n";
    return false;
  } else {
    ConcurrencyInsteresting = 1;
  }

  if (ConcurrencyInsteresting == 1) {
    string line{};
    int i = 0;
    for (; getline(Confile, line); i++) {
      if (i % 2 == 0) {
        locs_set.insert(line);
      }
    }
    if (i % 2 != 0) {
      outs() << "[DEBUG] Wrong lineNumber in configTxt";
      return false;
    }
  }
  // char* Group_debug = getenv("GROUP_DEBUG");
  // if(strcmp(Group_debug, "YES")) {
  //   outs() << "GROUP_DEBUG";
  //   for(auto item : locs_set) {
  //     outs() << item << '\n';
  //   }
  // }
#ifdef GROUP_DEBUG
  outs() << "from config.txt\n";
  for (auto item : locs_set) {
    outs() << item << "\n";
  }
#endif

  /* read config.txt end */

  /* Scan every Instruction */
  for (auto &F : M) {
    for (auto &BB : F) {
      bool flag_found = false;
      std::string loc_one_BB{};
      for (BasicBlock::iterator instruction = BB.begin(), IEnd = BB.end();
           instruction != IEnd; instruction++) {
        if (Instruction *inst = dyn_cast<Instruction>(instruction)) {

          /* deal with SourceLoc */
          std::string SourceLoc = getSourceLoc(inst);
          if (SourceLoc.size() == 0)
            continue;
          if (SourceLoc.find(":") == std::string::npos)
            continue;
          std::string tmpstring("./");
          if (SourceLoc.compare(0, tmpstring.length(), tmpstring) == 0) {
            SourceLoc.erase(0, 2);
          }
          /* Get the sourceloc */

          /* Find right location */
          bool flag_same = false;
          if (locs_set.find(SourceLoc) != locs_set.end()) {
            flag_same = true;
          }
          if (flag_same && !flag_found) {
            flag_found = true;
            loc_one_BB = SourceLoc;
            if (locs_map.find(SourceLoc) == locs_map.end()) {
              locs_map[SourceLoc] = std::set<std::string>{};
            } 
            // else {
            //   outs() << "[DEBUG] Same loc more one time.\n" << SourceLoc << '\n';
            //   return false;
            // }
          } else if (flag_same && flag_found) {
            /* Skip same as key. */
            if(loc_one_BB == SourceLoc) {
              continue;
            }
            locs_map[loc_one_BB].insert(SourceLoc);
          }
        }
      }
    }
  }

  /* Make sure there is anyone being left. */
  std::set<std::string> check_set{};
  for (auto item : locs_map) {
    // if (locs_set.find(item.first) == locs_set.end()) {
    //   outs() << "[DEBUG] there is someone being left\n" << item.first <<
    //   '\n';
    // }
    check_set.insert(item.first);
    for (auto ss : item.second) {
      // if (locs_set.find(ss) == locs_set.end()) {
      //   outs() << "[DEBUG] there is someone being left\n" << ss << '\n';
      // }
      check_set.insert(ss);
    }
  }
  if (check_set.size() != locs_set.size()) {
    outs() << "[DEBUG] there is someone being left\n";
    for (auto item : check_set) {
      if (locs_set.find(item) == locs_set.end()) {
        outs() << item << '\n';
      }
    }
    return false;
  }

  /* Dump final result. */
  for (auto item : locs_map) {
    outs() << "[KEY] " << item.first << '\n';
    for (auto ss : item.second) {
      outs() << "[VALUE] " << ss << "\n";
    }
  }
  return true;
}

char GroupKey::ID = 0;

static RegisterPass<GroupKey> X("so", "GroupKey");
