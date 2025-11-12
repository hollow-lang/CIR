CFLAGS = -Wall -Wextra -std=c++23
BUILD_DIR = build

.PHONY: all
all: $(BUILD_DIR)/cas

$(BUILD_DIR)/cas: $(BUILD_DIR) core/cir.h core/asm.h main.cpp
	$(CXX) $(CFLAGS) -o $@ main.cpp


$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

examples/dl-imports/libexample.so:
	$(MAKE) -C examples/dl-imports libexample.so

run: all examples/dl-imports/libexample.so
	./$(BUILD_DIR)/cas ./example.cas -d examples/dl-imports/libexample.so

clean:
	rm -rf $(BUILD_DIR)

