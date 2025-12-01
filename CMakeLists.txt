cmake_minimum_required(VERSION 3.14)
project(OpcUaExpertPractice LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)

# --- 1. Fetch open62541 (The Engine) ---
include(FetchContent)
FetchContent_Declare(
    open62541
    GIT_REPOSITORY https://github.com/open62541/open62541.git
    GIT_TAG        v1.4.3  # Stable release
)

# Enable full Namespace 0 (Standard nodes) so we can use standard types
set(UA_NAMESPACE_ZERO "FULL" CACHE STRING "" FORCE)

# Enable NodeSet XML loader for loading information models from XML files
set(UA_ENABLE_NODESETLOADER ON CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(open62541)

# --- 2. Build Your Executable ---
add_executable(opcua_server src/main.cpp)

# Link the library
target_link_libraries(opcua_server PRIVATE open62541::open62541)
target_include_directories(opcua_server PRIVATE ${CMAKE_SOURCE_DIR}/include)

# Pass the nodesetloader flag to our code
if(UA_ENABLE_NODESETLOADER)
    target_compile_definitions(opcua_server PRIVATE UA_ENABLE_NODESETLOADER)
endif()