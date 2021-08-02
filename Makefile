BIN := main
COMPILE_FLAGS = -MMD -MP -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Wshadow -Iinclude
LINK_FLAGS = 
SRC_DIRS := ./src
CXX = clang++

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
	$(CXX) $(OBJS) $(LINK_FLAGS) -o $@

$(BUILD_DIR)/%.cpp.o: %.cpp
	mkdir -p $(dir $@)
	$(CXX) $(COMPILE_FLAGS) -c $< -o $@

.PHONY: clean

clean:
	rm -r ./build

# install:
# copy ./include/teddy to /usr/local/include/ or selected path

-include $(DEPS)