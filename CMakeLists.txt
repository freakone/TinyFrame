cmake_minimum_required(VERSION 3.7)
project(tf)
SET(CMAKE_C_COMPILER cc)
SET(CMAKE_CXX_COMPILER gcc)

# This is a dummy CMakeLists file for CLion, which still doesn't understand Makefiles.
# Do not try to build the library using this file, it's here only for static analysis.

set(CMAKE_CXX_STANDARD GNU89)

set(SOURCE_FILES
        demo/demo_simple.c
        demo/demo_simple.h
        TinyModbus.c
        TinyModbus.h
        TinyModbus_Config.h
        demo/utils.c
        demo/utils.h
        )

add_executable(tf ${SOURCE_FILES})
