set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -flto ") # -static 
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DLIBXML_STATIC ") # -static
add_library(helper_library STATIC helper.cc)
add_executable(main main.c)
target_link_libraries(main PRIVATE helper_library)
