cmake_minimum_required(VERSION 3.20)
project(blackjack_ai)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Compiler warnings
if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Find required packages
find_package(Threads REQUIRED)

# Include directories
include_directories(
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/include/core
        ${CMAKE_SOURCE_DIR}/include/ai
        ${CMAKE_SOURCE_DIR}/third_party
)

# Core source files, shared by the CLI and the web server
set(CORE_SOURCES
        src/core/Card.cpp
        src/core/Deck.cpp
        src/core/Player.cpp
        src/core/Dealer.cpp
        src/core/Rules.cpp
        src/core/HandSession.cpp
        src/core/Game.cpp
        src/ai/QLearningAI.cpp
        src/ai/MonteCarloAI.cpp
        src/api/Api.cpp
)

# Interactive CLI (menu-driven)
add_executable(blackjack_ai
        src/main.cpp
        ${CORE_SOURCES}
)

# Web demo server (serves web/ and the JSON API over the same core)
add_executable(blackjack_server
        src/server/main.cpp
        ${CORE_SOURCES}
)
target_link_libraries(blackjack_server PRIVATE Threads::Threads)

# Create data directory for Q-table storage
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/data)

# Copy data directory to build folder (if it exists in source)
if(EXISTS ${CMAKE_SOURCE_DIR}/data)
    file(COPY ${CMAKE_SOURCE_DIR}/data DESTINATION ${CMAKE_BINARY_DIR})
endif()

# Copy the web demo's static assets next to the server binary
if(EXISTS ${CMAKE_SOURCE_DIR}/web)
    file(COPY ${CMAKE_SOURCE_DIR}/web DESTINATION ${CMAKE_BINARY_DIR})
endif()

# Testing (optional - uncomment when you add tests)
# enable_testing()
# add_subdirectory(tests)

# Installation rules (optional)
install(TARGETS blackjack_ai blackjack_server DESTINATION bin)
install(DIRECTORY data/ DESTINATION share/blackjack_ai/data)
install(DIRECTORY web/ DESTINATION share/blackjack_ai/web OPTIONAL)

# Print configuration summary
message(STATUS "=================================")
message(STATUS "Blackjack AI Configuration")
message(STATUS "=================================")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Targets: blackjack_ai (CLI), blackjack_server (web demo)")
message(STATUS "Source Directory: ${CMAKE_SOURCE_DIR}")
message(STATUS "Binary Directory: ${CMAKE_BINARY_DIR}")
message(STATUS "=================================")