cmake_minimum_required(VERSION 3.14)
project(ModbusTcpPractice LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Generate compile_commands.json for IDE IntelliSense
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# =============================================================================
# Find libmodbus using pkg-config
# =============================================================================
find_package(PkgConfig REQUIRED)
pkg_check_modules(MODBUS REQUIRED libmodbus)

message(STATUS "Found libmodbus version: ${MODBUS_VERSION}")
message(STATUS "  Include dirs: ${MODBUS_INCLUDE_DIRS}")
message(STATUS "  Libraries: ${MODBUS_LIBRARIES}")

# =============================================================================
# Modbus TCP Server
# =============================================================================
add_executable(modbus_server src/server.cpp)
target_include_directories(modbus_server PRIVATE ${MODBUS_INCLUDE_DIRS})
target_link_libraries(modbus_server PRIVATE ${MODBUS_LIBRARIES})

# =============================================================================
# Modbus TCP Client
# =============================================================================
add_executable(modbus_client src/client.cpp)
target_include_directories(modbus_client PRIVATE ${MODBUS_INCLUDE_DIRS})
target_link_libraries(modbus_client PRIVATE ${MODBUS_LIBRARIES})
