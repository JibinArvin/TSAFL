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

echo "Installation completed. Everything's fine!"

set -eux

# compile the program and get bit code
cd $ROOT_DIR/test/UAF/
./cleanDIR.sh
clang -g -emit-llvm -c ./uaf.c -o uaf.bc

# perform static analysis
$ROOT_DIR/tool/staticAnalysis/staticAnalysis.sh uaf

# complie the instrumented program with ASAN
export Con_PATH=$ROOT_DIR/test/UAF/ConConfig.uaf
export ConFile_PATH=$ROOT_DIR/test/UAF/config.txt
# $ROOT_DIR/tool/TSAFL/CUR-clang-fast -g -c ./uaf.c -o uaf.o
# clang++ ./uaf.o $ROOT_DIR/tool/TSAFL/Currency-instr.o -g -o uaf -lpthread -ldl
