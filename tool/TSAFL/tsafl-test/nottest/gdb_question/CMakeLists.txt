set(CMAKE_BUILD_TYPE RelWithDebInfo) 
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-pointer-arith  -DBIN_PATH=\\\"/usr/local/bin\\\"  -DAFL_PATH=\\\"/usr/local/lib/afl\\\" -DDOC_PATH=\\\"/usr/local/share/doc/afl\\\" ")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -flto ") # -static 
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DLIBXML_STATIC -O0") # -static

add_executable(tsafl TSAFL-fuzz.c hashmap.c)
target_link_libraries(tsafl  PRIVATE ${LIBXML2_LIBRARIES} readxml_library)

add_subdirectory(nottest)
