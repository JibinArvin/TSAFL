#!/bin/bash
# For mac usr.

if [ $(command uname) == "Darwin" ]; then
    echo "Sry not ready for mac usr!"
    exit 1;
# For linux.
else
    BIN_PATH=$(readlink -f "$0")
	ROOT_DIR=$(dirname $(dirname $(dirname $(dirname $BIN_PATH))))
fi

export ROOT_DIR=${ROOT_DIR}
export PATH=${ROOT_DIR}/clang+llvm/bin:${ROOT_DIR}/tool/SVF/Release-build/bin:$PATH
export LD_LIBRARY_PATH=${ROOT_DIR}/clang+llvm/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}

Analysis_file=$1
echo $Analysis_file

File_local_name=${Analysis_file##*/}
echo $File_local_name
cp -u "${Analysis_file:?}" ./$File_local_name.bc

$ROOT_DIR/tool/staticAnalysis/staticAnalysis.sh $File_local_name

# rm -rf *.file
# rm -rf *.xml
# rm -rf *.npy
# rm -rf mssa*
