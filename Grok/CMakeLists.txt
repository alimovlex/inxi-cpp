cmake_minimum_required(VERSION 3.16)
project(inxi-cpp VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(INXI_STATIC "Prefer static linking where possible" OFF)

add_executable(inxi
  src/main.cpp
  src/util.cpp
  src/options.cpp
  src/output.cpp
  src/pci.cpp
  src/cpu.cpp
  src/system.cpp
  src/hardware.cpp
  src/storage.cpp
  src/report.cpp
)

target_include_directories(inxi PRIVATE include)
target_compile_definitions(inxi PRIVATE
  INXI_CPP_VERSION="${PROJECT_VERSION}"
  INXI_BASED_ON="3.3.40"
)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  target_compile_options(inxi PRIVATE -Wall -Wextra -Wpedantic -O2)
endif()

install(TARGETS inxi RUNTIME DESTINATION bin)
