/**
 * copyright
 * created by wcventure
 * changed by jibin
 **/
#include <llvm/Analysis/InlineCost.h>
#include <llvm/IR/Attributes.h>
#define LLVM_11

#define AFL_LLVM_PASS
#include "../config.h"
#include "../debug.h"
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
#include "llvm/Transforms/Utils/Cloning.h"
#include "llvm/Transforms/Utils/Local.h" // for FindDbgAddrUses
#include <algorithm>
#include <boost/algorithm/string.hpp>
#include <cassert>
#include <chrono>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <iostream>
#include <llvm/Analysis/CFG.h>      // for CFG
#include <llvm/IR/BasicBlock.h>     // for BasicBlock
#include <llvm/IR/CFG.h>            // for CFG
#include <llvm/IR/Function.h>       // for Function
#include <llvm/IR/GlobalVariable.h> // for GlobalVariable
#include <llvm/IR/InstIterator.h>   // for inst iteration
#include <llvm/IR/InstrTypes.h>     // for TerminatorInst
#include <llvm/IR/Instructions.h>   // for Instructions
#include <llvm/IR/IntrinsicInst.h>  // for intrinsic instruction
#include <llvm/IR/Module.h>         // for Module
#include <llvm/IR/Type.h>
#include <llvm/IR/User.h>
#include <llvm/IRReader/IRReader.h> // for isIRFiler isBitcode
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

#ifdef LLVM_11
#include <llvm/Pass.h>
#else
#include "llvm/IR/CallSite.h"
#include <llvm/PassAnalysisSupport.h>
#endif

using namespace llvm;
using namespace llvm;
using namespace std::chrono;
using namespace std;
using std::vector;

#define DEBUG_TYPE "DBDSPass"

namespace {

class CURPass : public ModulePass {

public:
  static char ID;
  CURPass() : ModulePass(ID) {}

  bool runOnModule(Module &M) override;

  StringRef getPassName() const override { return "CUR Instrumentation"; }
};

} // namespace

char CURPass::ID = 0;

uint16_t string_hash_encoding(const char *str) {
  uint16_t encodingNum = 26;
  for (int k = 0; k < strlen(str); k++) {
    encodingNum *= int(str[k]);
    encodingNum = encodingNum + int(str[k]);
  }
  return encodingNum;
}

int CountLines(char *filename) {
  ifstream ReadFile;
  int n = 0;
  string line;
  ReadFile.open(filename, ios::in);
  if (ReadFile.fail()) {
    return 0;
  } else // file exit
  {
    while (getline(ReadFile, line, '\n')) {
      if (line.size() == 0 || line.empty())
        continue;
      else
        n++;
    }
    ReadFile.close();
    return n;
  }
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

bool isNum(string str) {
  stringstream sin(str);
  double d;
  char c;
  if (!(sin >> d))
    return false;
  if (sin >> c)
    return false;
  return true;
}

// TODO: add file reading
// --------------------------------------------------------------------------------------

bool CURPass::runOnModule(Module &M) {
  LLVMContext &C = M.getContext();
  IntegerType *Int8Ty = IntegerType::getInt8Ty(C);
  IntegerType *Int32Ty = IntegerType::getInt32Ty(C);
  // IntegerType *Int64Ty = IntegerType::getInt64Ty(C);
  /* Show a banner */
  outs() << "Using CUR-LLVM-PASS\n";

  /* Show a banner */

  char be_quiet = 0;

  if (isatty(2) && !getenv("AFL_QUIET")) {

    SAYF(cCYA "TSAFL " cBRI VERSION cRST " by <jibindong96@gmail.com>\n");

  } else
    be_quiet = 1;

  // 添加插桩函数的声明
  llvm::LLVMContext &context = M.getContext();
  llvm::IRBuilder<> builder(context);

  // Function instr_Call()
  std::vector<Type *> argTypesCall;
  argTypesCall.push_back(builder.getInt8PtrTy());
  ArrayRef<Type *> argTypesRefCall(argTypesCall);
  llvm::FunctionType *funcCallType =
      llvm::FunctionType::get(builder.getVoidTy(), argTypesRefCall, false);
  llvm::Function *instr_CallFunc = llvm::Function::Create(
      funcCallType, llvm::Function::ExternalLinkage, "_Z10instr_CallPv", &M);

  // Function instr_PthreadCall()
  std::vector<Type *> argTypesPthreadCall;
  argTypesPthreadCall.push_back(builder.getInt8PtrTy());
  ArrayRef<Type *> argTypesRefPthreadCall(argTypesPthreadCall);
  llvm::FunctionType *funcPthreadCallType = llvm::FunctionType::get(
      builder.getVoidTy(), argTypesRefPthreadCall, false);
  llvm::Function *instr_PthreadCallFunc = llvm::Function::Create(
      funcPthreadCallType, llvm::Function::ExternalLinkage,
      "_Z17instr_PthreadCallPv", &M);

  // Function instr_create()
  std::vector<Type *> argTypesCreate;
  argTypesCreate.push_back(Type::getInt64PtrTy(C));
  argTypesCreate.push_back(builder.getInt8PtrTy());
  ArrayRef<Type *> argTypesRefCreate(argTypesCreate);
  llvm::FunctionType *funcCreateType =
      llvm::FunctionType::get(builder.getVoidTy(), argTypesRefCreate, false);
  llvm::Function *instr_CreateFunc =
      llvm::Function::Create(funcCreateType, llvm::Function::ExternalLinkage,
                             "_Z20instr_pthread_createPmPv", &M);

  // Function instr_Join
  std::vector<Type *> argTypesJoin;
  argTypesJoin.push_back(builder.getInt64Ty());
  ArrayRef<Type *> argTypesRefJoin(argTypesJoin);
  llvm::FunctionType *funcJoinType =
      FunctionType::get(builder.getVoidTy(), argTypesRefJoin, false);
  llvm::Function *instr_JoinFunc =
      Function::Create(funcJoinType, llvm::Function::ExternalLinkage,
                       "_Z18instr_pthread_joinm", &M);

  // Function instr_LOC
  std::vector<Type *> argTypesLOC;
  argTypesLOC.push_back(builder.getInt8PtrTy());
  argTypesLOC.push_back(builder.getInt32Ty());
  argTypesLOC.push_back(builder.getInt32Ty());
  ArrayRef<Type *> argTypesRefLOC(argTypesLOC);
  llvm::FunctionType *funcLOCType =
      FunctionType::get(builder.getVoidTy(), argTypesRefLOC, false);
  llvm::Function *instr_LOCFunc = Function::Create(
      funcLOCType, llvm::Function::ExternalLinkage, "_Z9instr_LOCPvjj", &M);

  // Function instr_SLOC
  std::vector<Type *> argTypesSLOC;
  argTypesSLOC.push_back(builder.getInt8PtrTy());
  argTypesSLOC.push_back(builder.getInt32Ty());
  ArrayRef<Type *> argTypesRefSLOC(argTypesSLOC);
  llvm::FunctionType *funcSLOCType =
      FunctionType::get(builder.getVoidTy(), argTypesRefSLOC, false);
  llvm::Function *instr_SLOCFunc =
      Function::Create(funcSLOCType, llvm::Function::ExternalLinkage,
                       "_Z16instr_LOC_stringPvj", &M);

  // TODO: delete before

  // Function traceEnd
  llvm::FunctionType *functraceEndType =
      llvm::FunctionType::get(builder.getVoidTy(), false);
  llvm::Function *instr_traceEndFunc = llvm::Function::Create(
      functraceEndType, llvm::Function::ExternalLinkage, "_Z8traceEndv", &M);

  /* add confiles */
  /*There are template file in the  */
  const char *Con_PATH = NULL;
  char *ConFileName;
  int linesNum = 0;
  int ConcurrencyInsteresting = 0;
  Con_PATH = getenv("ConFile_PATH");

  /* Used to describe the loc info */
  std::map<std::string, int> loc_to_number;

  if (Con_PATH == NULL || strcmp(Con_PATH, "") == 0) {
    ConcurrencyInsteresting = 0;
    outs() << "[DEBUG] Will use no schedule instru\n";
    outs() << "[DEBUG] MEANINGLESS. \n";
  }

  if (Con_PATH != NULL && Con_PATH[0] != '.' && Con_PATH[0] != ' ') {
    ConFileName = (char *)malloc(strlen(Con_PATH) + 1);
    if (ConFileName == NULL) {
      return false;
    }
    strcpy(ConFileName, Con_PATH);
    linesNum = CountLines(ConFileName);
  } else {
    outs() << "[DEBUG] Please export Con_PATH"
           << "\n";
  }

  if (linesNum % 2 != 0) {
    outs() << "[DEBUG] LLVM_PASS -> real wrong number(not even number) for "
              "lineNumber: "
           << linesNum << "\n";
    return false;
  }

  ifstream read_file;
  read_file.open(ConFileName, ios::binary);

  if (read_file.fail()) {
    outs() << "[DEBUG] Failed to open ConConfig file."
           << "\n";
    outs() << "[DEBUG] The program quit without execution."
           << "\n";
    ConcurrencyInsteresting = 0;
    return false;
  } else {
    ConcurrencyInsteresting = 1;
    outs() << "[DEBUG] The ConConfig file was successfully opened.\n[DEBUG] "
              "The file is "
           << ConFileName << "\n";
  }

  if (ConcurrencyInsteresting == 1) {
    std::vector<std::string> ConLine;
    string line;
    int line_co = 0;
    for (int i = 0; getline(read_file, line); i++) {
      if (i % 2 == 0) {
        ConLine.push_back("");
        if (!line.empty() && line[line.size() - 1] == '\r') {
          if (line.size() == 1)
            ConLine[line_co] = "N.A.";
          else {
            ConLine[line_co] = line.erase(line.size() - 1);
            int spaceIndex = ConLine[i].find(" ");
            if (spaceIndex >= 0)
              ConLine[line_co] = ConLine[i].substr(0, spaceIndex);
          }
        } else if (line.size() == 0 || line.empty()) {
          outs() << "[DEBUG] LLVM_PASS -> block line in CONFILE_CONFIG: "
                 << ConFileName << "\n";
          i++;
        } else {
          ConLine[line_co] = line;
          int spaceIndex = ConLine[line_co].find(" ");
          if (spaceIndex >= 0)
            ConLine[line_co] = ConLine[i].substr(0, spaceIndex);
        }
        line_co++;
      } else {
        std::string ss;
        if (!line.empty() && line[line.size() - 1] == '\r') {
          ss = line.erase(line.size() - 1);
          int spaceIndex = ss.find(" ");
          if (spaceIndex >= 0) {
            ss = ss.substr(0, spaceIndex);
          }
        } else if (line.size() == 0 || line.empty()) {
          outs() << "[DEBUG] LLVM_PASS -> block line in CONFILE_CONFIG: "
                 << ConFileName << "\n";
          i++;
        } else {
          ss = line;
          int spaceIndex = ss.find(" ");
          if (spaceIndex >= 0) {
            ss = ss.substr(0, spaceIndex);
          }
        }
        if (!isNum(ss)) {
          outs() << "[DEBUG] LLVM-PASS -> line not number of CON_CONFIG LINE: "
                 << i << "(zero start)\n";
          return false;
        }
        int loc = stoi(ss);
        std::string s_loc = ConLine.back();
        loc_to_number[s_loc] = loc;
      }
    }

#ifdef DEBUG
    outs() << "[DEBUG] All the Concurent insteresting line are shown as follow:"
           << "\n";
    for (auto item : loc_to_number) {
      out << "loc string: " << item.first << "number: " << item.second << " \n";
    }
#endif
    free(ConFileName);
    read_file.close();
    outs() << "[DEBUG] start insert.\n";
    // TODO: change pthread_create function
    std::set<llvm::Function *> pthreadCallFunction; // 找pthread_create的参数
    std::set<llvm::Function *>::iterator setit; // 定义前向集合迭代器
    int Only_Instru_pthread_create =
        0; // 创建线程的函数是否只考虑pthread_create
    for (Module::iterator function = M.begin(), FEnd = M.end();
         function != FEnd; function++) {
      if (function->isDeclaration() || function->size() == 0) {
        continue;
      }

      for (Function::iterator basicblock = function->begin(),
                              BBEnd = function->end();
           basicblock != BBEnd; basicblock++) {
        for (BasicBlock::iterator instruction = basicblock->begin(),
                                  IEnd = basicblock->end();
             instruction != IEnd; instruction++) {
          if (Instruction *inst = dyn_cast<Instruction>(instruction)) {
            if (inst->getOpcode() == Instruction::Call) {
              std::string instr_create = "pthread_create";
              if (inst->getNumOperands() == 5) { // 操作数大于5
                if (instr_create ==
                    std::string(
                        inst->getOperand(4)->getName())) { // 找pthread_create
                  Function *pthread_task = dyn_cast<Function>(
                      inst->getOperand(2)->stripPointerCasts());
                  if (pthread_task != NULL) {
                    pthreadCallFunction.insert(pthread_task);
                    Only_Instru_pthread_create = 1;
                  }
                }
              }
            }
          }
        }
      }
    }

    if (Only_Instru_pthread_create ==
        0) { // 找不到pthread_create函数，那就可能有用户封装了平pthread_create
      for (Module::iterator function = M.begin(), FEnd = M.end();
           function != FEnd; function++) {
        if (function->isDeclaration() || function->size() == 0) {
          continue;
        }
        for (Function::iterator basicblock = function->begin(),
                                BBEnd = function->end();
             basicblock != BBEnd; basicblock++) { // 遍历每一个Basic Block

          for (BasicBlock::iterator instruction = basicblock->begin(),
                                    IEnd = basicblock->end();
               instruction != IEnd; instruction++) {

            if (Instruction *inst = dyn_cast<Instruction>(instruction)) {
              if (inst->getOpcode() == Instruction::Call) {
                std::string instr_create2 = "PR_CreateThread";
                if (inst->getNumOperands() >= 5) { // 操作数大于5

                  int find_thread_flag = 0;
                  int arg_id = 0;

                  if (Only_Instru_pthread_create == 0 &&
                      inst->getNumOperands() == 8 &&
                      instr_create2 ==
                          std::string(inst->getOperand(7)->getName())) {
                    find_thread_flag = 1;
                    arg_id = 1;
                  }

                  if (find_thread_flag == 1) {
                    Function *pthread_task = dyn_cast<Function>(
                        inst->getOperand(arg_id)->stripPointerCasts());
                    pthreadCallFunction.insert(pthread_task);
                  }
                }

                std::string instr_create3 = "create_pthread";
                if (inst->getNumOperands() >= 6) { // 操作数大于6

                  int find_thread_flag = 0;
                  int arg_id = 0;

                  if (Only_Instru_pthread_create == 0 &&
                      inst->getNumOperands() == 6 &&
                      instr_create3 ==
                          std::string(inst->getOperand(5)->getName())) {
                    find_thread_flag = 1;
                    arg_id = 3;
                  }

                  if (find_thread_flag == 1) {
                    Function *pthread_task = dyn_cast<Function>(
                        inst->getOperand(arg_id)->stripPointerCasts());

                    pthreadCallFunction.insert(pthread_task);
                  }
                }
              }
            }
          }
        }
      }
    }

    GlobalVariable *AFLMapPtr =
        new GlobalVariable(M, PointerType::get(Int8Ty, 0), false,
                           GlobalValue::ExternalLinkage, 0, "__afl_area_ptr");

    GlobalVariable *AFLPrevLoc = new GlobalVariable(
        M, Int32Ty, false, GlobalValue::ExternalLinkage, 0, "__afl_prev_loc", 0,
        GlobalVariable::GeneralDynamicTLSModel, 0, false);

    int inst_blocks = 0;
    /* Insert trace piont. */
    /* change!*/
    for (auto &FF : M) {
      std::string LastLocString = "";
      llvm::InlineFunctionInfo ifi;
      srand(time(0));
      for (auto &BB : FF) {
        bool static_interesting = false;
        for (BasicBlock::iterator instruction = BB.begin(), IEnd = BB.end();
             instruction != IEnd; instruction++) {
          if (Instruction *inst = dyn_cast<Instruction>(instruction)) {
            std::string SourceLoc = getSourceLoc(inst);

            if (LastLocString == SourceLoc) // 跟上一条指令的行号一样，略过
              continue;
            LastLocString = SourceLoc;

            if (SourceLoc.size() ==
                0) // SourceLoc为空则不用比较了，直接continue
              continue;

            if (SourceLoc.find(":") ==
                std::string::npos) // SourceLoc可能取得有问题
              continue;
            std::string tmpstring("./");
            if (SourceLoc.compare(0, tmpstring.length(), tmpstring) ==
                0) { // 如果SourceLoc前两个字符为"./", 则删除掉
              SourceLoc.erase(0, 2);
            }
            // TODO: change the thing in the confile
            for (int i = 0; i < ConLine.size(); i++) {
              if (inst->getOpcode() == Instruction::Ret) {
                continue;
              }
              if (boost::algorithm::ends_with(ConLine[i], SourceLoc)) {
                static_interesting = true;
              }
            }
          }
        }

        /* Close to afl-llvm part! */
        if (INST_RATIO > 100 || INST_RATIO < 1)
          FATAL("Bad value of INST_RATIO (must be between 1 and 100)");
        if (static_interesting || AFL_R(100) <= INST_RATIO) {
          BasicBlock::iterator IP = BB.getFirstInsertionPt();
          IRBuilder<> IRB(&(*IP));

          /* Make up cur_loc */

          unsigned int cur_loc = AFL_R(MAP_SIZE);

          ConstantInt *CurLoc = ConstantInt::get(Int32Ty, cur_loc);

          /* Load prev_loc */

          LoadInst *PrevLoc = IRB.CreateLoad(AFLPrevLoc);
          PrevLoc->setMetadata(M.getMDKindID("nosanitize"),
                               MDNode::get(C, None));
          Value *PrevLocCasted = IRB.CreateZExt(PrevLoc, IRB.getInt32Ty());

          /* Load SHM pointer */

          LoadInst *MapPtr = IRB.CreateLoad(AFLMapPtr);
          MapPtr->setMetadata(M.getMDKindID("nosanitize"),
                              MDNode::get(C, None));
          Value *MapPtrIdx =
              IRB.CreateGEP(MapPtr, IRB.CreateXor(PrevLocCasted, CurLoc));

          /* Update bitmap */

          LoadInst *Counter = IRB.CreateLoad(MapPtrIdx);
          Counter->setMetadata(M.getMDKindID("nosanitize"),
                               MDNode::get(C, None));
          Value *Incr = IRB.CreateAdd(Counter, ConstantInt::get(Int8Ty, 1));
          IRB.CreateStore(Incr, MapPtrIdx)
              ->setMetadata(M.getMDKindID("nosanitize"), MDNode::get(C, None));

          /* Set prev_loc to cur_loc >> 1 */

          StoreInst *Store = IRB.CreateStore(
              ConstantInt::get(Int32Ty, cur_loc >> 1), AFLPrevLoc);
          Store->setMetadata(M.getMDKindID("nosanitize"), MDNode::get(C, None));

          inst_blocks++;
        }
      }
    }

    // instr func time
    for (Module::iterator function = M.begin(), FEnd = M.end();
         function != FEnd; function++) {

      if (function->isDeclaration() || function->size() == 0) {
        continue;
      }

      BasicBlock::iterator FIP = function->begin()->getFirstInsertionPt();
      SmallVector<Value *, 1> retArg;
      Value *CurFuncVa =
          ConstantExpr::getBitCast(&(*function), Type::getInt8PtrTy(C));
      retArg.push_back(CurFuncVa);
      IRBuilder<> IRB(&(*FIP));
      IRB.CreateCall(instr_CallFunc, retArg);
      if (function->getName() == "main") {
        // continue;
      }

      std::string LastLocString = "";

      for (Function::iterator basicblock = function->begin(),
                              BBEnd = function->end();
           basicblock != BBEnd; basicblock++) {

        // std::string LastLocString = "";

        for (BasicBlock::iterator instruction = basicblock->begin(),
                                  IEnd = basicblock->end();
             instruction != IEnd; instruction++) {

          if (Instruction *inst = dyn_cast<Instruction>(instruction)) {

            IRBuilder<> IRBuild(
                &(*(instruction))); // 插桩的位置，插在该指令的后面

            if (inst->getOpcode() == Instruction::Call) {

              std::string instr_exit = "_exit";
              if (inst->getNumOperands() == 2) { // 操作数大于2
                if (instr_exit == std::string(inst->getOperand(1)->getName())) {
                  // 在exit前方插
                  outs() << "[DEBUG] insert instr_traceEndFunc.\n";
                  IRBuild.CreateCall(instr_traceEndFunc);
                }
              }

              std::string instr_create = "pthread_create";
              std::string instr_create2 = "PR_CreateThread";
              std::string instr_create3 = "create_pthread";
              std::string instr_join = "pthread_join";
              if (inst->getNumOperands() >= 5) { // 操作数大于5

                if (Only_Instru_pthread_create == 1 &&
                    inst->getNumOperands() == 5 &&
                    instr_create ==
                        std::string(inst->getOperand(4)
                                        ->getName())) { // 找pthread_create

                  // 在pthread_create后方插
                  SmallVector<Value *, 2> createArg; // 用一个Vector存放参数
                  Function *pthread_task = dyn_cast<Function>(
                      inst->getOperand(2)->stripPointerCasts());
                  Value *OpFuncV = ConstantExpr::getBitCast(
                      pthread_task, Type::getInt8PtrTy(C));
                  createArg.push_back(inst->getOperand(0));
                  createArg.push_back(OpFuncV);
                  BasicBlock::iterator tmptier = instruction;
                  tmptier++;
                  IRBuilder<> IRNEXT_Build(&(*(tmptier)));
                  IRNEXT_Build.CreateCall(instr_CreateFunc, createArg);

                } else if (Only_Instru_pthread_create == 0 &&
                           inst->getNumOperands() == 8 &&
                           instr_create2 ==
                               std::string(
                                   inst->getOperand(7)
                                       ->getName())) { // PR_CreateThread

                  SmallVector<Value *, 2> createArg; // 用一个Vector存放参数
                  Function *pthread_task = dyn_cast<Function>(
                      inst->getOperand(3)->stripPointerCasts());
                  Value *OpFuncV = ConstantExpr::getBitCast(
                      pthread_task, Type::getInt8PtrTy(C));
                  createArg.push_back(inst->getOperand(0));
                  createArg.push_back(OpFuncV);
                  BasicBlock::iterator tmptier = instruction;
                  tmptier++;
                  IRBuilder<> IRNEXT_Build(&(*(tmptier)));
                  IRNEXT_Build.CreateCall(instr_CreateFunc, createArg);

                } else if (Only_Instru_pthread_create == 0 &&
                           inst->getNumOperands() == 6 &&
                           instr_create3 ==
                               std::string(inst->getOperand(5)
                                               ->getName())) { // create_pthread

                  SmallVector<Value *, 2> createArg; // 用一个Vector存放参数
                  Function *pthread_task = dyn_cast<Function>(
                      inst->getOperand(3)->stripPointerCasts());
                  Value *OpFuncV = ConstantExpr::getBitCast(
                      pthread_task, Type::getInt8PtrTy(C));
                  createArg.push_back(inst->getOperand(0));
                  createArg.push_back(OpFuncV);
                  BasicBlock::iterator tmptier = instruction;
                  tmptier++;
                  IRBuilder<> IRNEXT_Build(&(*(tmptier)));
                  IRNEXT_Build.CreateCall(instr_CreateFunc, createArg);
                }

              } else if (inst->getNumOperands() >= 3) { // 操作数大于3

                if ((inst->getNumOperands() == 3 &&
                     instr_join ==
                         std::string(
                             inst->getOperand(2)->getName())) // 找pthread_join
                ) {
                  SmallVector<Value *, 1> JoinArg; // 用一个Vector存放参数
                  JoinArg.push_back(inst->getOperand(0));
                  BasicBlock::iterator tmptier = instruction;
                  tmptier++;
                  IRBuilder<> IRNEXT_Build(&(*(tmptier)));
                  IRNEXT_Build.CreateCall(instr_JoinFunc, JoinArg);
                }
              }
            }

            // 指令对应的代码行号
            std::string SourceLoc = getSourceLoc(inst);

            // 过滤掉自己插桩的instr_Call和instr_PthreadCallFunc
            if (inst->getOpcode() == Instruction::Call) {
              if (inst->getNumOperands() == 2 &&
                  (inst->getOperand(1) == instr_PthreadCallFunc ||
                   inst->getOperand(1) == instr_CallFunc)) {
                continue;
              }
            }

            if (LastLocString == SourceLoc) // 跟上一条指令的行号一样，略过
              continue;
            LastLocString = SourceLoc;

            if (SourceLoc.size() ==
                0) // SourceLoc为空则不用比较了，直接continue
              continue;

            if (SourceLoc.find(":") ==
                std::string::npos) // SourceLoc可能取得有问题
              continue;

            std::string tmpstring("./");
            if (SourceLoc.compare(0, tmpstring.length(), tmpstring) ==
                0) { // 如果SourceLoc前两个字符为"./", 则删除掉
              SourceLoc.erase(0, 2);
            }
            // TODO: change the thing in the confile
            for (int i = 0; i < ConLine.size(); i++) {
              if (inst->getOpcode() == Instruction::Ret) {
                continue;
              }
              if (boost::algorithm::ends_with(
                      ConLine[i],
                      SourceLoc)) { // rather than ConLine[i] == SourceLoc

                outs() << "[DEBUG] Instrumentation for " << SourceLoc << "\n";

                SmallVector<Value *, 2> slocArg;

                int64_t loc_info_temp = loc_to_number[ConLine[i]];
                Value *Sloc = ConstantInt::get(Int32Ty, loc_info_temp);
                Value *CurFuncVa = ConstantExpr::getBitCast(
                    &(*function), Type::getInt8PtrTy(C));
                slocArg.push_back(CurFuncVa);
                slocArg.push_back(Sloc);
                IRBuild.CreateCall(instr_SLOCFunc, slocArg);

                /* Insert the instr_LOCFunc */
                std::string strLocNum =
                    SourceLoc.substr(SourceLoc.find(":") + 1);
                outs() << "strLocNum" << strLocNum << "\n";
                unsigned int LocNum = std::stoi(strLocNum);
                BasicBlock::iterator ntmptier = instruction;
                IRBuilder<> IRNEXT_Build(&(*(ntmptier)));
                SmallVector<Value *, 3> locArg; // 用一个Vector存放参数
                Value *conInt = ConstantInt::get(Int32Ty, LocNum);
                Value *CurFuncV = ConstantExpr::getBitCast(
                    &(*function), Type::getInt8PtrTy(C));
                locArg.push_back(CurFuncV);
                locArg.push_back(conInt);
                locArg.push_back(Sloc); /*Loction imformation! */
                IRNEXT_Build.CreateCall(instr_LOCFunc, locArg);
                break;
              }
            }
          }
        }
      }
    }
  }
  return true;
}

static void registerCurPass(const PassManagerBuilder &,
                            legacy::PassManagerBase &PM) {
  PM.add(new CURPass());
}

static RegisterStandardPasses
    RegisterCurPass(PassManagerBuilder::EP_ModuleOptimizerEarly,
                    registerCurPass);

static RegisterStandardPasses
    RegisterCurPass0(PassManagerBuilder::EP_EnabledOnOptLevel0,
                     registerCurPass);
