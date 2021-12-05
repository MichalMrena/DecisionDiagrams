ifdef USE_CLANG
	CXX = clang++
	LIB = -stdlib=libc++
endif

CXXFLAGS = -MMD -MP -std=c++20 $(LIB) -Iinclude \
	-Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Wshadow
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

ifdef USE_OMP
	CXXFLAGS += -fopenmp
	LDFLAGS += -fopenmp
endif

SRCS := main.cpp test.cpp
OBJS := $(SRCS:%=$(BUILD_DIR)/%.o)
DEPS := $(OBJS:.o=.d)
-include $(DEPS)

main: $(BUILD_DIR)/main

test: $(BUILD_DIR)/test

experiment: $(BUILD_DIR)/experiment

$(BUILD_DIR)/%: $(BUILD_DIR)/%.cpp.o
	@echo $(LINK_NOTICE)
	$(CXX) $< -o $@ $(LDFLAGS)

$(BUILD_DIR)/%.cpp.o: src/%.cpp
	@echo $(COMPILE_NOTICE)
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# My precious see:
# https://stackoverflow.com/questions/42830131/an-unexpected-rm-occur-after-make
.PRECIOUS: $(OBJS)

.PHONY: clean main test

clean:
	rm -r ./build

# install:
# 	cp -r ./include/teddy /usr/local/include/
# TODO install_manifest