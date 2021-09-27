CXX = g++
CXXFLAGS = -MMD -MP -std=c++20 -Iinclude -Wall -Wextra \
 -Wpedantic -Wconversion -Wsign-conversion -Wshadow
SRC_DIR = ./src
LINK_NOTICE = "\e[1;33mLinking:\e[0m"
COMPILE_NOTICE = "\e[1;33mCompiling:\e[0m"

ifdef DEBUG
	CXXFLAGS += -g
	BUILD_DIR ?= ./build/debug
else
	CXXFLAGS += -O3
	BUILD_DIR ?= ./build/release
endif

SRCS := main.cpp test.cpp
OBJS := $(SRCS:%=$(BUILD_DIR)/%.o)
DEPS := $(OBJS:.o=.d)
-include $(DEPS)

main: $(BUILD_DIR)/main

test: $(BUILD_DIR)/test

$(BUILD_DIR)/%: $(BUILD_DIR)/%.cpp.o
	@echo $(LINK_NOTICE)
	$(CXX) $< -o $@ $(LDFLAGS)

$(BUILD_DIR)/%.cpp.o: src/%.cpp
	@echo $(COMPILE_NOTICE)
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

.PHONY: clean dps

dps:
	@echo $(SRCS)
	@echo $(OBJS)
	@echo $(DEPS)

clean:
	rm -r ./build

# install:
# 	cp -r ./include/teddy /usr/local/include/
# TODO install_manifest