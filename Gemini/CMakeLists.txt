cmake_minimum_required(VERSION 3.16)
project(inxi VERSION 3.3.31 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Build type
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_FLAGS_DEBUG   "-g -O0 -DDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG")

# Optional: enable warnings
add_compile_options(-Wall -Wextra -Wpedantic)

# -----------------------------------------------------------------------
# Find optional dependencies
# -----------------------------------------------------------------------
find_package(PkgConfig QUIET)

# JSON output support (optional, header-only nlohmann/json)
find_path(NLOHMANN_JSON_INCLUDE nlohmann/json.hpp
  PATHS /usr/include /usr/local/include)
if(NLOHMANN_JSON_INCLUDE)
  add_compile_definitions(HAVE_NLOHMANN_JSON)
  include_directories(${NLOHMANN_JSON_INCLUDE})
  message(STATUS "nlohmann/json found: ${NLOHMANN_JSON_INCLUDE}")
else()
  message(STATUS "nlohmann/json not found; JSON output disabled")
endif()

# -----------------------------------------------------------------------
# Include dirs
# -----------------------------------------------------------------------
include_directories(${CMAKE_SOURCE_DIR}/include)

# -----------------------------------------------------------------------
# Sources
# -----------------------------------------------------------------------
set(INXI_SOURCES
  # Core / startup
  src/main.cpp
  src/initialize.cpp

  # Global state
  src/global_state.cpp

  # Utilities (no hardware probing)
  src/utils/string_utils.cpp
  src/utils/file_utils.cpp
  src/utils/process_utils.cpp
  src/utils/math_utils.cpp

  # Infrastructure
  src/configs.cpp
  src/options_handler.cpp
  src/start_client.cpp
  src/check_tools.cpp
  src/colors.cpp
  src/output.cpp
  src/downloader.cpp
  src/updater.cpp
  src/debugger.cpp
  src/system_debugger.cpp
  src/check_recommends.cpp
  src/messages.cpp

  # Hardware info collectors (InfoItem subclasses)
  src/items/audio_item.cpp
  src/items/battery_item.cpp
  src/items/bluetooth_item.cpp
  src/items/cpu_item.cpp
  src/items/disk_item.cpp
  src/items/display_item.cpp
  src/items/graphics_item.cpp
  src/items/info_item.cpp
  src/items/machine_item.cpp
  src/items/memory_item.cpp
  src/items/network_item.cpp
  src/items/partition_item.cpp
  src/items/process_item.cpp
  src/items/raid_item.cpp
  src/items/repo_item.cpp
  src/items/sensor_item.cpp
  src/items/swap_item.cpp
  src/items/system_item.cpp
  src/items/unmounted_item.cpp
  src/items/usb_item.cpp
  src/items/weather_item.cpp

  # Output generator
  src/output_generator.cpp
)

add_executable(inxi ${INXI_SOURCES})

# -----------------------------------------------------------------------
# Install
# -----------------------------------------------------------------------
include(GNUInstallDirs)
install(TARGETS inxi RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

# -----------------------------------------------------------------------
# Tests (optional)
# -----------------------------------------------------------------------
option(BUILD_TESTS "Build unit tests" OFF)
if(BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()
