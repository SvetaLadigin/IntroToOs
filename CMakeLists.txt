#cmake_minimum_required(VERSION 3.20)
#project(hw1os)
#
#set(CMAKE_CXX_STANDARD 14)

#add_executable(hw1os main.cpp)


cmake_minimum_required(VERSION 3.13)

set(PROJECT_FILES Commands.cpp Commands.h smash.cpp signals.h signals.cpp)
add_executable(hw1os ${PROJECT_FILES})

