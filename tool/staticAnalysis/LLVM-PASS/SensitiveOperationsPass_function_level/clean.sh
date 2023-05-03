#!/bin/bash -e

mv CMakeLists.txt copy_CMakeLists.txt

rm -rf CMake*
rm -rf cmake*
rm -rf Makefile
rm -rf build.ninja
rm -rf *.so

mv copy_CMakeLists.txt CMakeLists.txt