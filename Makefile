BUILD_DIR := build

CFLAGS := -g -O3 -pedantic -Wall -Wextra
SRC_C := $(wildcard c/src/*.c)
OBJ_C := $(foreach file,$(SRC_C),$(BUILD_DIR)/$(file:.c=.o))
DEP := $(OBJ_C:.o=.d)
OUTPUT_C   := triplet_challenge_c

CXXFLAGS ?= -g -O3
CXXFLAGS_EXTRA := -std=c++17 -Icpp/src -pedantic -Wall -Wextra -Werror -MMD -MP
SRC_CPP := $(wildcard cpp/src/*.cpp)
OBJ_CPP := $(foreach file,$(SRC_CPP),$(BUILD_DIR)/$(file:.cpp=.o))
DEP := $(OBJ_CPP:.o=.d)
OUTPUT_CPP := triplet_challenge_cpp

UT_SRC := $(wildcard cpp/test/*.cpp) $(filter-out cpp/src/main.cpp,$(SRC_CPP))
UT_OBJ := $(foreach file,$(UT_SRC),$(BUILD_DIR)/$(file:.cpp=.o))
UT_DEP := $(UT_OBJ:.o=.d)
UT_OUTPUT_CPP := unit_test

all: $(OUTPUT_CPP) $(OUTPUT_C) check

$(BUILD_DIR):
	mkdir -p $@/c/src $@/cpp/src $@/cpp/test

$(BUILD_DIR)/%.o: %.c %.h
	$(CC) $(CFLAGS) -c -o $@ $<

$(OUTPUT_C): $(BUILD_DIR) $(OBJ_C)
	$(CC) $(CFLAGS) -o $@ $(OBJ_C)

$(BUILD_DIR)/%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(CXXFLAGS_EXTRA) -c -o $@ $<

$(OUTPUT_CPP): $(BUILD_DIR) $(OBJ_CPP)
	$(CXX) $(CXXFLAGS) $(CXXFLAGS_EXTRA) -o $@ $(OBJ_CPP)

check: $(OUTPUT_CPP) $(UT_OUTPUT_CPP)
	./$(UT_OUTPUT_CPP)

$(UT_OUTPUT_CPP): $(BUILD_DIR) $(UT_OBJ)
	$(CXX) $(CXXFLAGS) $(CXXFLAGS_EXTRA) -o $@ $(UT_OBJ)

clean:
	rm -rf $(BUILD_DIR) $(OUTPUT_CPP) $(UT_OUTPUT)

.PHONY: clean check

dbg-%:
	@echo "$* = $($*)"

-include $(DEP) $(UT_DEP)
