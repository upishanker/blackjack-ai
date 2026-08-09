# Dependency-free build for machines without CMake.
#   make -f build.mk            # build both binaries into bin/
#   make -f build.mk run-server # build and serve the web demo on :8080
#   make -f build.mk clean
#
# Named build.mk rather than Makefile because .gitignore excludes "Makefile"
# (a leftover from in-source CMake builds).

CXX      ?= g++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra -Iinclude -Ithird_party
LDFLAGS  ?= -pthread

BUILD := build/obj
BIN   := bin

CORE_SRC := $(wildcard src/core/*.cpp) $(wildcard src/ai/*.cpp) $(wildcard src/api/*.cpp)
CORE_OBJ := $(CORE_SRC:%.cpp=$(BUILD)/%.o)

CLI_OBJ    := $(BUILD)/src/main.o
SERVER_OBJ := $(BUILD)/src/server/main.o

.PHONY: all clean run-server run-cli
all: $(BIN)/blackjack_ai $(BIN)/blackjack_server

$(BIN)/blackjack_ai: $(CORE_OBJ) $(CLI_OBJ)
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

$(BIN)/blackjack_server: $(CORE_OBJ) $(SERVER_OBJ)
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

$(BUILD)/%.o: %.cpp
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -MMD -MP -c $< -o $@

-include $(shell find $(BUILD) -name '*.d' 2>/dev/null)

# Both binaries resolve data/ and web/ relative to the working directory,
# so they must be launched from the repo root.
run-server: $(BIN)/blackjack_server
	./$(BIN)/blackjack_server

run-cli: $(BIN)/blackjack_ai
	./$(BIN)/blackjack_ai

clean:
	rm -rf build $(BIN)
