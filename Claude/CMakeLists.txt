cmake_minimum_required(VERSION 3.13)  # Automatically generated syntax

# project(inxi-cpp) --> When CLion project is created, it is automatically created if it is created as c project,
# Because of this, the C++ gtest is not built.
# Therefore, remove c as below.
project(inxi-cpp)

set(CMAKE_C_STANDARD 17) # Automatic generation syntax when creating a C project
set(CMAKE_CXX_STANDARD 17)

include_directories(${PROJECT_SOURCE_DIR}/inc)
file(GLOB SRCS "${PROJECT_SOURCE_DIR}/src/*.cpp" "main.cpp")
add_executable(inxi-cpp ${SRCS})