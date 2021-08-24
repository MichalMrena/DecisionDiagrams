COMPILE_FLAGS = -MMD -MP -std=c++20 -Iinclude
COMPILE_FLAGS += -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Wshadow
CXX = clang++
LINK_HEADER = "\e[1;33mLinking:\e[0m"
COMPILE_HEADER = "\e[1;33mCompiling:\e[0m"

ifdef DEBUG
	COMPILE_FLAGS += -g
	BUILD_DIR ?= ./build/debug
else
	COMPILE_FLAGS += -O3
	BUILD_DIR ?= ./build/release
endif

main: $(BUILD_DIR)/main
test: $(BUILD_DIR)/test

$(BUILD_DIR)/%: $(BUILD_DIR)/%.cpp.o
	@echo $(LINK_HEADER)
	$(CXX) $< -o $@

$(BUILD_DIR)/test.cpp.o: test/test.cpp
	@echo $(COMPILE_HEADER)
	mkdir -p $(dir $@)
	$(CXX) $(COMPILE_FLAGS) -c $< -o $@

$(BUILD_DIR)/main.cpp.o: src/main.cpp
	@echo $(COMPILE_HEADER)
	mkdir -p $(dir $@)
	$(CXX) $(COMPILE_FLAGS) -c $< -o $@

.PHONY: clean

clean:
	rm -r ./build

# install:
# copy ./include/teddy to /usr/local/include/ or selected path

-include $(DEPS)