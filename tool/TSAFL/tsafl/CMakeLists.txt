set(CMAKE_BUILD_TYPE DEBUG)
set(CMAKE_C_COMPILER "clang-14")
set(CMAKE_CXX_COMPILER "clang++-14")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-pointer-arith -DBIN_PATH=\\\"/usr/local/bin\\\"  -DAFL_PATH=\\\"/usr/local/lib/afl\\\" -DDOC_PATH=\\\"/usr/local/share/doc/afl\\\" ")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -O0 -Wall -DLIBXML_STATIC ") # -static
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0 -Wall -std=c++14")  #设置c++的编译选项
message(AUTHOR_WARNING "Disable warning output! ")
add_definitions(-w) # the afl-fuzz contain a lot feature cause warning.

add_executable(tsafl ./TSAFL-fuzz.c ./helper.cc ./readxml.c)

target_link_libraries(tsafl  PRIVATE ${LIBXML2_LIBRARIES} readxml_library)
