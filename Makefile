BUILD_DIR := build

CFLAGS := -g -O3 -pedantic -Wall -Wextra -pthread
SRC_C := $(wildcard c/src/*.c)
OBJ_C := $(foreach file,$(SRC_C),$(BUILD_DIR)/$(file:.c=.o))
DEP_C := $(OBJ_C:.o=.d)
OUTPUT_C   := triplet_challenge

all: $(OUTPUT_C) check

$(BUILD_DIR):
	mkdir -p $@/c/src $@/cpp/test

$(BUILD_DIR)/%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

$(OUTPUT_C): $(BUILD_DIR) $(OBJ_C)
	$(CC) $(CFLAGS) -o $@ $(OBJ_C)

clean:
	rm -rf $(BUILD_DIR) $(OUTPUT_C) $(UT_OUTPUT)

.PHONY: clean check

dbg-%:
	@echo "$* = $($*)"

-include  $(DEP_C) $(UT_DEP)


test:
	/usr/bin/time -v ./triplet_challenge pg2009.txt radix
	hyperfine -w 3 -m 300 "./triplet_challenge pg2009.txt"

test-big:
	/usr/bin/time -v ./triplet_challenge gutenberg-utf-8.txt radix
	hyperfine -w 3 -m 10 "./triplet_challenge gutenberg-utf-8.txt"