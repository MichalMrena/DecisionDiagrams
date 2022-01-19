ifdef USE_CLANG
	CXX := clang++
	STDLIB := -stdlib=libc++
endif

CXXFLAGS := -MMD -MP -std=c++20 $(STDLIB) -Iinclude \
	-Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Wshadow
SRC_DIR := ./src
LINK_NOTICE := "\e[1;33mLinking:\e[0m"
COMPILE_NOTICE := "\e[1;33mCompiling:\e[0m"

ifdef DEBUG
	CXXFLAGS += -g
	BUILD_DIR := ./build/debug
else
	CXXFLAGS += -O3
	BUILD_DIR := ./build/release
endif

ifdef USE_OMP
	CXXFLAGS += -fopenmp
	LDFLAGS += -fopenmp
endif

SRCS := $(shell find ./src -name *.cpp -exec basename {} \;)
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

.PHONY: clean main test experiment install

clean:
	rm -r ./build

PREFIX ?= /usr/local

install:
	@rm -f install_manifest.txt
	@echo "Installing into $(DIRNAME)$(PREFIX)."
	@for f in $(shell find include/teddy -type f) ; do \
		mkdir -p $$(dirname $(DIRNAME)$(PREFIX)/$$f) ; \
		cp $$f $(DIRNAME)$(PREFIX)/$$f ; \
		echo $(DIRNAME)$(PREFIX)/$$f >> install_manifest.txt ; \
	done
	@echo "Done installing. List of installed files was \
	written into install_manifest.txt."