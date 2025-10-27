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
find_package(OpenCV REQUIRED)
find_package(SQLite3 REQUIRED)

# Include directories
include_directories(
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/include/core
        ${CMAKE_SOURCE_DIR}/include/ai
        ${CMAKE_SOURCE_DIR}/include/db
        ${CMAKE_SOURCE_DIR}/include/gui
        ${CMAKE_SOURCE_DIR}/include/utils
        ${OpenCV_INCLUDE_DIRS}
)

# Core source files
set(CORE_SOURCES
        src/core/Card.cpp
        src/core/Deck.cpp
        src/core/Player.cpp
        src/core/Dealer.cpp
        src/core/Game.cpp
)

# AI source files
set(AI_SOURCES
        src/ai/QLearningAI.cpp
        # src/ai/MonteCarloAI.cpp  # Uncomment when implemented
)

# Database source files (when implemented)
set(DB_SOURCES
        # src/db/Database.cpp  # Uncomment when implemented
)

# GUI source files (when implemented)
set(GUI_SOURCES
        # src/gui/BlackjackGUI.cpp  # Uncomment when implemented
)

# Utility source files (when implemented)
set(UTILS_SOURCES
        # src/utils/Logger.cpp  # Uncomment when implemented
)

# Main executable
add_executable(blackjack_ai
        src/main.cpp
        ${CORE_SOURCES}
        ${AI_SOURCES}
        ${DB_SOURCES}
        ${GUI_SOURCES}
        ${UTILS_SOURCES}
)

# Link libraries
target_link_libraries(blackjack_ai
        ${OpenCV_LIBS}
        SQLite::SQLite3
)

# Create data directory for Q-table storage
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/data)

# Copy data directory to build folder (if it exists in source)
if(EXISTS ${CMAKE_SOURCE_DIR}/data)
    file(COPY ${CMAKE_SOURCE_DIR}/data DESTINATION ${CMAKE_BINARY_DIR})
endif()

# Optional: Copy assets directory (for card images when GUI is implemented)
if(EXISTS ${CMAKE_SOURCE_DIR}/assets)
    file(COPY ${CMAKE_SOURCE_DIR}/assets DESTINATION ${CMAKE_BINARY_DIR})
endif()

# Testing (optional - uncomment when you add tests)
# enable_testing()
# add_subdirectory(tests)

# Installation rules (optional)
install(TARGETS blackjack_ai DESTINATION bin)
install(DIRECTORY data/ DESTINATION share/blackjack_ai/data)
install(DIRECTORY assets/ DESTINATION share/blackjack_ai/assets OPTIONAL)

# Print configuration summary
message(STATUS "=================================")
message(STATUS "Blackjack AI Configuration")
message(STATUS "=================================")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "OpenCV Version: ${OpenCV_VERSION}")
message(STATUS "SQLite3 Found: ${SQLite3_FOUND}")
message(STATUS "Source Directory: ${CMAKE_SOURCE_DIR}")
message(STATUS "Binary Directory: ${CMAKE_BINARY_DIR}")
message(STATUS "=================================")