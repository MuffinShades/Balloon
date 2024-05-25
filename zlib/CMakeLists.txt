cmake_minimum_required(VERSION 3.0.0)
project(balloon VERSION 0.1.0 LANGUAGES C CXX)
set (CMAKE_CXX_STANDARD 20)
include(CTest)
enable_testing()

file(GLOB_RECURSE SOURCES
    src/*.h
    src/*.hpp
    src/*.c
    src/*.cpp
)

add_executable(balloon ${SOURCES})

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
