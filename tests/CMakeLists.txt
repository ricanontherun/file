cmake_minimum_required(VERSION 2.8.7)
project(tests)

set(CMAKE_CXX_COMPILER g++)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY bin)

set(SOURCE_FILES
    FileOpenTests.cpp
    ReadTests.cpp
    ../file.cpp
)

add_executable(tests ${SOURCE_FILES})
