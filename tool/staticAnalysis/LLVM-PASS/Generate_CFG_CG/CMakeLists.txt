project(Generate_CFG_CGPass)
cmake_minimum_required(VERSION 3.5)

add_library(Generate_CFG_CGPass MODULE Generate_CFG_CG.cpp)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fno-rtti -fpic -O3 -funroll-loops -Wall -D_FORTIFY_SOURCE=2 -g -Wno-vla-extension -Wno-unknown-warning-option -Wno-pointer-sign -Wno-variadic-macros -fPIC")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-rtti -fpic -O3 -funroll-loops -Wall -D_FORTIFY_SOURCE=2 -g -Wno-vla-extension -Wno-unknown-warning-option -Wno-pointer-sign -Wno-variadic-macros -fPIC")

set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -Wl,-flat_namespace -Wl,-undefined,suppress")