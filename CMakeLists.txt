# CMakeList.txt : CMake project for mapdecompiler, include source and define
# project specific logic here.
#
cmake_minimum_required(VERSION 3.8)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_BUILD_TYPE "Debug")
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(projectName "mapdecompiler")

project(${projectName})

message("generator is set to ${CMAKE_GENERATOR}")

if(MSVC)
	add_compile_options(/W4 /WX)
else()
	add_compile_options("-Wall" "-Wextra" "-Wpedantic" "-Werror")
endif()
# Add source to this project's executable.
add_executable (${projectName} "mapdecomp.cpp")

