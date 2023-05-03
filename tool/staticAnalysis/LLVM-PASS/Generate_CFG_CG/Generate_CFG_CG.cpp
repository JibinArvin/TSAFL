/*
   american fuzzy lop - LLVM-mode instrumentation pass
   ---------------------------------------------------

   Written by Laszlo Szekeres <lszekeres@google.com> and
              Michal Zalewski <lcamtuf@google.com>

   LLVM integration design comes from Laszlo Szekeres. C bits copied-and-pasted
   from afl-as.c are Michal's fault.

   Copyright 2015, 2016 Google Inc. All rights reserved.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at:

     http://www.apache.org/licenses/LICENSE-2.0

   This library is plugged into LLVM when invoking clang through afl-clang-fast.
   It tells the compiler to add code roughly equivalent to the bits discussed
   in ../afl-as.h.

 */

#include <cstdint>
#define AFL_LLVM_PASS
#include "config.h"
#include "debug.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "llvm/ADT/Statistic.h"
#include "llvm/Analysis/CFGPrinter.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"

#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <list>
#include <set>
#include <sstream>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>

#if defined(LLVM34)
#include "llvm/DebugInfo.h"
#else
#include "llvm/IR/DebugInfo.h"
#endif

#if defined(LLVM34) || defined(LLVM35) || defined(LLVM36)
#define LLVM_OLD_DEBUG_API
#endif

// #define STATICANAMYSIS_DEBUG

using namespace llvm;

cl::opt<std::string>
    OutDirectory("outdir",
                 cl::desc("Output directory where two Graphs are generated"),
                 cl::value_desc("outdir"));

cl::opt<std::string> TargetLoctions("target",
                                    cl::desc("target loctions need be "),
                                    cl::value_desc("target"));

namespace {

class AFLGO_based : public ModulePass {

public:
  static char ID;
  AFLGO_based() : ModulePass(ID) {}

  bool runOnModule(Module &M) override;
};

} // namespace

char AFLGO_based::ID = 0;

namespace llvm {

template <> struct DOTGraphTraits<Function *> : public DefaultDOTGraphTraits {
  DOTGraphTraits(bool isSimple = true) : DefaultDOTGraphTraits(isSimple) {}

  static std::string getGraphName(Function *F) {
    return "CFG for '" + F->getName().str() + "' function";
  }

  std::string getNodeLabel(BasicBlock *Node, Function *Graph) {
    if (!Node->getName().empty()) {
      return Node->getName().str();
    }

    std::string Str;
    raw_string_ostream OS(Str);

    Node->printAsOperand(OS, false);
    return OS.str();
  }
};

} // namespace llvm

static void getDebugLoc(const Instruction *I, std::string &Filename,
                        unsigned &Line) {
#ifdef LLVM_OLD_DEBUG_API
  DebugLoc Loc = I->getDebugLoc();
  if (!Loc.isUnknown()) {
    DILocation cDILoc(Loc.getAsMDNode(M.getContext()));
    DILocation oDILoc = cDILoc.getOrigLocation();

    Line = oDILoc.getLineNumber();
    Filename = oDILoc.getFilename().str();

    if (filename.empty()) {
      Line = cDILoc.getLineNumber();
      Filename = cDILoc.getFilename().str();
    }
  }
#else
  if (DILocation *Loc = I->getDebugLoc()) {
    Line = Loc->getLine();
    Filename = Loc->getFilename().str();

    if (Filename.empty()) {
      DILocation *oDILoc = Loc->getInlinedAt();
      if (oDILoc) {
        Line = oDILoc->getLine();
        Filename = oDILoc->getFilename().str();
      }
    }
  }
#endif /* LLVM_OLD_DEBUG_API */
}

// FIXME: add more possible option
static bool isBlacklisted(const Function *F) {
  static const SmallVector<std::string, 8> Blacklist = {
      "asan.", "llvm.",  "sancov.", "__ubsan_handle_",
      "free",  "malloc", "calloc",  "realloc"};

  for (auto const &BlacklistFunc : Blacklist) {
    if (F->getName().startswith(BlacklistFunc)) {
      return true;
    }
  }

  return false;
}

bool AFLGO_based::runOnModule(Module &M) {
  std::string outDirectory{};
  if (OutDirectory.empty()) {
    BADF("Cannot find outDirectory for Graphs, use ./ to replace");
  } else {
    outDirectory = "./";
  }

  SAYF("[-]CGF and CG generation started\n");

  if (TargetLoctions.empty()) {
    BADF("Cannot find targetDir for Graphs, which means the analysis is "
         "meaningless");
  }
  std::set<std::string> targets;
  std::ifstream targetsfile(TargetLoctions);
  std::string line;
#ifdef STATICANAMYSIS_DEBUG
  std::cout << "lines\n";
#endif
  while (std::getline(targetsfile, line)) {
    int loc = line.find(" ");
    line = line.substr(0, loc);
    if (likely(line != ""))
      targets.insert(line);
#ifdef STATICANAMYSIS_DEBUG
    std::cout << line << " -->line for debug\n";
#endif
  }
  targetsfile.close();

  /* Create dot-files directory */
  std::string dotfiles(OutDirectory + "/dot-files");
  if (sys::fs::create_directory(dotfiles)) {
    FATAL("Could not create directory %s.", dotfiles.c_str());
  }

  for (auto &F : M) {

    bool has_BBs = false;
    std::string funcName = F.getName().str();

#ifdef STATICANAMYSIS_DEBUG
    std::cout << "running on " << funcName << std::endl;
#endif

    /* Black list of function names */
    if (isBlacklisted(&F)) {
      continue;
    }

    for (auto &BB : F) {
      std::string bb_name("");
      std::string filename;
      unsigned line;
      bool is_contianed_target = false;
      bool is_need_add = true;
      std::string last_ii_name{""};
      for (auto &I : BB) {
        std::string bbcall_name("");
        getDebugLoc(&I, filename, line);

        std::string i_name{""};

        /* Don't worry about external libs */
        static const std::string Xlibs("/usr/");
        if (filename.empty() || line == 0 ||
            !filename.compare(0, Xlibs.size(), Xlibs))
          continue;

        // TODO: add format
        std::size_t found = filename.find_last_of("/\\");
        if (found != std::string::npos)
          filename = filename.substr(found + 1);
        i_name = filename + ":" + std::to_string(line);

        // skip same i_name
        if (unlikely(last_ii_name != i_name)) {
          last_ii_name = i_name;
          is_need_add = true;
        } else {
          is_need_add = false;
        }

        if (bb_name.empty()) {
          bb_name = "line:" + i_name;
#ifdef STATICANAMYSIS_DEBUG
          errs() << bb_name << "  --> bb_name, when init\n";
#endif
        } else {
          if (is_contianed_target == 0 &&
              targets.find(i_name) != targets.end()) {
            bb_name = "line:" + i_name;
            is_contianed_target = true;
          } else if (is_contianed_target == true &&
                     targets.find(i_name) != targets.end() &&
                     is_need_add == true) {
            bb_name += ";line:" + i_name;
          }
        }

        if (auto *c = dyn_cast<CallInst>(&I)) {

          std::size_t found = filename.find_last_of("/\\");
          if (found != std::string::npos)
            filename = filename.substr(found + 1);

          if (auto *CalledF = c->getCalledFunction()) {
            if (!isBlacklisted(CalledF)) {
              bbcall_name = CalledF->getName().str();
              bb_name += ";Call:" + bbcall_name;
            }
          }
        }
      }

      if (!bb_name.empty()) {

        BB.setName(bb_name);
        if (!BB.hasName()) {
          std::string newname = bb_name;
          Twine t(newname);
          SmallString<256> NameData;
          StringRef NameRef = t.toStringRef(NameData);
          MallocAllocator Allocator;
          BB.setValueName(ValueName::Create(NameRef, Allocator));
        }
        has_BBs = true;
      }
    }
    if (has_BBs) {
      /* Print CFG */
      std::string cfgFileName = dotfiles + "/cfg." + funcName + ".dot";
      std::error_code EC;
      raw_fd_ostream cfgFile(cfgFileName, EC, sys::fs::F_None);
      if (!EC) {
        WriteGraph(cfgFile, &F, true);
      }
    }
  }
  return true;
}
static RegisterPass<AFLGO_based> X("so", "AFLGO_based Pass");