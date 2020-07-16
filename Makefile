# https://spin.atomicobject.com/2016/08/26/makefile-c-projects/
TARGET_EXEC ?= main.exe

INC_DIRS := $(shell find ./src -type d)
INC_FLAGS := $(addprefix -I,$(INC_DIRS))

CPPFLAGS ?= $(INC_FLAGS) -MMD -MP

DEBUG_BUILD_DIR ?= ./buildDebug
RELEASE_BUILD_DIR ?= ./buildRelease

SRC_DIRS ?= ./src

ifdef DEBUG
	CPPFLAGS += -g -Wall -Wextra -Wpedantic -std=c++17 -fopenmp
	BUILD_DIR ?= $(DEBUG_BUILD_DIR)
else
	CPPFLAGS += -O3 -Wall -Wextra -Wpedantic -std=c++17 -fopenmp
	BUILD_DIR ?= $(RELEASE_BUILD_DIR)
endif

SRCS := $(shell find $(SRC_DIRS) -name *.cpp)
OBJS := $(SRCS:%=$(BUILD_DIR)/%.o)
DEPS := $(OBJS:.o=.d)

$(BUILD_DIR)/$(TARGET_EXEC): $(OBJS)
	$(CXX) $(OBJS) -o $@ $(LDFLAGS) -fopenmp -lstdc++fs

$(BUILD_DIR)/%.cpp.o: %.cpp
	$(MKDIR_P) $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

.PHONY: clean

clean:
	$(RM) -r $(DEBUG_BUILD_DIR)
	$(RM) -r $(RELEASE_BUILD_DIR)

-include $(DEPS)

MKDIR_P ?= mkdir -p