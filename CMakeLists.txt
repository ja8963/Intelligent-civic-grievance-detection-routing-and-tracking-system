# Minimum CMake version
cmake_minimum_required(VERSION 3.21)

# Project definition
project(CivicGrievanceApp LANGUAGES CXX)

# Require C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find Qt6 packages
find_package(Qt6 COMPONENTS Widgets Sql REQUIRED)

# Include the frontend source directory
set(SRC_DIR "frontend")

# Gather source files
file(GLOB_RECURSE SRC_FILES
    "${SRC_DIR}/*.cpp"
    "${SRC_DIR}/*.h"
)

add_executable(${PROJECT_NAME} ${SRC_FILES})

# Include directories (Qt and local)
target_include_directories(${PROJECT_NAME} PRIVATE "${SRC_DIR}")

# Link Qt libraries
target_link_libraries(${PROJECT_NAME} PRIVATE Qt6::Widgets Qt6::Sql)

# Enable MOC (Qt's meta‑object compiler) for classes with Q_OBJECT
set_target_properties(${PROJECT_NAME} PROPERTIES
    AUTOMOC ON
    AUTOUIC ON
    AUTORCC ON
)

# On Windows, ensure the binary is placed next to DLLs (optional)
if (WIN32)
    set_target_properties(${PROJECT_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
endif()
