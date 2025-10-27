cmake_minimum_required(VERSION 3.20)
project(blackjack_ai)

set(CMAKE_CXX_STANDARD 17)

# Find required packages
find_package(OpenCV REQUIRED)
find_package(SQLite3 REQUIRED)

# Include directories
include_directories(include ${OpenCV_INCLUDE_DIRS})

# Source files
file(GLOB_RECURSE SOURCES "src/*.cpp")

# Executable
add_executable(blackjack_ai ${SOURCES})

# Link libraries
target_link_libraries(blackjack_ai ${OpenCV_LIBS} SQLite::SQLite3)
