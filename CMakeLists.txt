# CMakeList.txt : CMake project for mapdecompiler, include source and define
# project specific logic here.
#
cmake_minimum_required(VERSION 3.8)

set(projectName "mapdecompiler")

project(${projectName})

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Add source to this project's executable.
add_executable (${projectName} "mapdecomp.cpp")
