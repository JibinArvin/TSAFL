#!/bin/bash -e

mv CMakeLists.txt copy_CMakeLists.txt

rm -rf CMake*
rm -rf cmake*
rm -rf Makefile
rm -rf build.ninja
rm -rf *.so
rm -rf *.a

mv copy_CMakeLists.txt CMakeLists.txt