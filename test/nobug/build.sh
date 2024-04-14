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
cd $ROOT_DIR/test/nobug/
./cleanDIR.sh
$CLANG_PATH -g -emit-llvm -c ./nobug.c -o nobug.bc

# perform static analysis
$ROOT_DIR/tool/staticAnalysis/staticAnalysis.sh nobug

# complie the instrumented program with ASAN
export Con_PATH=$ROOT_DIR/test/nobug/ConConfig.nobug
export ConFile_PATH=$ROOT_DIR/test/nobug/config.txt
$ROOT_DIR/tool/TSAFL/CUR-clang-fast -g -O0 -c ./nobug.c -o nobug.o -fsanitize=address
$CLANGPLUS_PATH ./nobug.o $ROOT_DIR/tool/TSAFL/Currency-instr.o $ROOT_DIR/tool/TSAFL/afl-llvm-rt.o -g -o nobug -lpthread -ldl -fsanitize=address
$CLANGPLUS_PATH --version

