# OPC UA C++ Project

A C++ project for OPC UA (Open Platform Communications Unified Architecture) development.

## Project Structure

```
.
├── CMakeLists.txt      # CMake build configuration
├── src/                # Source files (.cpp)
├── include/            # Header files (.h, .hpp)
├── lib/                # External libraries
├── bin/                # Compiled binaries
├── build/              # Build directory (generated)
├── tests/              # Unit tests
└── README.md           # This file
```

## Prerequisites

- CMake 3.16 or higher
- C++17 compatible compiler (GCC 7+, Clang 5+, MSVC 2017+)
- open62541 library (optional, for OPC UA implementation)

## Building

```bash
mkdir -p build && cd build
cmake ..
make
```

## Running

```bash
./bin/opcua_app
```

## License

MIT License


