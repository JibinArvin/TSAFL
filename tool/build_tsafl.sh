#!/bin/bash

# For Mac
if [ $(command uname) == "Darwin" ]; then
	if ! [ -x "$(command -v greadlink)"  ]; then
		brew install coreutils
	fi
	BIN_PATH=$(greadlink -f "$0")
	ROOT_DIR=$(dirname $(dirname $BIN_PATH))
# For Linux
else
	BIN_PATH=$(readlink -f "$0")
	ROOT_DIR=$(dirname $(dirname $BIN_PATH))
fi

set -euxo pipefail

if ! [ -d "${ROOT_DIR}/clang+llvm"]; then
	echo "There is not clang+llvm files, now install them automatically or install by hand"
fi

export ROOT_DIR=${ROOT_DIR}
export PATH=${ROOT_DIR}/clang+llvm/bin:$PATH
export LD_LIBRARY_PATH=${ROOT_DIR}/clang+llvm/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}
export CC=clang
export CXX=clang++

cd ${ROOT_DIR}/tool/TSAFL/llvm_mode
make clean
make all


# mkdir ${ROOT_DIR}/tool/TSAFL/build
if [ ! -d ${ROOT_DIR}/tool/TSAFL/build ]; then
    # 如果目录存在，则删除它
    mkdir ${ROOT_DIR}/tool/TSAFL/build
fi
cmake -B ${ROOT_DIR}/tool/TSAFL/build ${ROOT_DIR}/tool/TSAFL
make -C ${ROOT_DIR}/tool/TSAFL/build/

echo "llvm_mode make is finished! PLZ check"