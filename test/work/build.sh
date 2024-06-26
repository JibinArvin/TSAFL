#!/bin/bash

# For Mac
if [ $(command uname) == "Darwin" ]; then
	if ! [ -x "$(command -v greadlink)"  ]; then
		brew install coreutils
	fi
	BIN_PATH=$(greadlink -f "$0")
	ROOT_DIR=$(dirname $(dirname $(dirname $BIN_PATH)))
# For Linux
else
	BIN_PATH=$(readlink -f "$0")
	ROOT_DIR=$(dirname $(dirname $(dirname $BIN_PATH)))
fi

export ROOT_DIR=${ROOT_DIR}
export PATH=${ROOT_DIR}/clang+llvm/bin:${ROOT_DIR}/tool/SVF/Release-build/bin:$PATH
export LD_LIBRARY_PATH=${ROOT_DIR}/clang+llvm/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}
export CLANG_PATH=${ROOT_DIR}/clang+llvm/bin/clang
export CLANGPLUS_PATH=${ROOT_DIR}/clang+llvm/bin/clang++

echo "Installation completed. Everything's fine!"

set -eux

# compile the program and get bit code
cd $ROOT_DIR/test/work
./cleanDIR.sh
clang -g -O0 -emit-llvm -c ./work.c -o work.bc

# perform static analysis
$ROOT_DIR/tool/staticAnalysis/staticAnalysis.sh work

# complie the instrumented program with ASAN
export Con_PATH=$ROOT_DIR/test/work/ConConfig.work
export ConFile_PATH=$ROOT_DIR/test/work/config.txt
$ROOT_DIR/tool/TSAFL/CUR-clang-fast -g -O0 -fsanitize=address -c ./work.c -o work.o
$CLANGPLUS_PATH ./work.o $ROOT_DIR/tool/TSAFL/Currency-instr.o $ROOT_DIR/tool/TSAFL/afl-llvm-rt.o -g -o work -lpthread -fsanitize=address -ldl
