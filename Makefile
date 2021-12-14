BIN := main
COMPILE_FLAGS = -MMD -MP -std=c++17 -stdlib=libc++ -Wall -Wextra -Wpedantic -Wconversion -Wshadow
LINK_FLAGS = -stdlib=libc++ -L/usr/local/lib/ -Wl,-rpath,/usr/local/lib/
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

-include $(DEPS)