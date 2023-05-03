export CC=/home/dongjibin/llvm12/build/bin/clang
export CXX=/home/dongjibin/llvm12/build/bin/clang++
export RANLIB=llvm-ranlib
export CFLAGS=" -flto -std=gnu99 "
export LDFLAGS=" -flto -fuse-ld=gold -Wl,-plugin-opt=emit-llvm "

./configure