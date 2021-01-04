BIN := main
COMPILE_FLAGS = -MMD -MP -std=c++20 -Wall -Wextra -Wpedantic -Wconversion
SRC_DIRS := ./src
CXX = clang++-10
# CXX = g++

ifdef DEBUG
	COMPILE_FLAGS += -g
	BUILD_DIR ?= ./build/debug
else
	COMPILE_FLAGS += -O3
	BUILD_DIR ?= ./build/release
endif

SRCS := $(shell find $(SRC_DIRS) -name *.cpp)
OBJS := $(SRCS:%=$(BUILD_DIR)/%.o)
DEPS := $(OBJS:.o=.d)

$(BUILD_DIR)/$(BIN): $(OBJS)
	$(CXX) $(OBJS) -o $@

$(BUILD_DIR)/%.cpp.o: %.cpp
	mkdir -p $(dir $@)
	$(CXX) $(COMPILE_FLAGS) -c $< -o $@

.PHONY: clean

clean:
	rm -r ./build

-include $(DEPS)