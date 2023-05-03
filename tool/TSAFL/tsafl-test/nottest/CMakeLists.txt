set(CMAKE_BUILD_TYPE Debug) 
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-pointer-arith  -DBIN_PATH=\\\"/usr/local/bin\\\"  -DAFL_PATH=\\\"/usr/local/lib/afl\\\" -DDOC_PATH=\\\"/usr/local/share/doc/afl\\\" ")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -flto ") # -static 
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DLIBXML_STATIC ") # -static

add_executable(readxmlTest read.c)
add_subdirectory(testhash)
add_subdirectory(flexarray)
add_subdirectory(helper)
target_link_libraries(readxmlTest  PRIVATE ${LIBXML2_LIBRARIES} readxml_library)
