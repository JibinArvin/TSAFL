#!/bin/bash -e

mv CMakeLists.txt copy_CMakeLists.txt

rm -rf CMake*
rm -rf cmake*
rm -rf Makefile
rm -rf build*
rm -rf .ninja*
rm -rf build.ninja
mv copy_CMakeLists.txt CMakeLists.txt
