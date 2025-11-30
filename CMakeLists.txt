cmake_minimum_required(VERSION 3.16)
project(opcua_project VERSION 1.0.0 LANGUAGES CXX)

# C++ Standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib)

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/include)

# Source files
file(GLOB_RECURSE SOURCES "src/*.cpp")

# Main executable
add_executable(opcua_app ${SOURCES})

# Optional: Find and link open62541 library for OPC UA
# find_package(open62541 REQUIRED)
# target_link_libraries(opcua_app open62541::open62541)

# Compiler warnings
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(opcua_app PRIVATE -Wall -Wextra -Wpedantic)
elseif(MSVC)
    target_compile_options(opcua_app PRIVATE /W4)
endif()

# Enable testing
enable_testing()
add_subdirectory(tests EXCLUDE_FROM_ALL)
