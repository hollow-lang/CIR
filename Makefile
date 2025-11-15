# CFLAGS = -Wall -Wextra -std=c++23
CFLAGS = -std=c++23 -flto -O3
BUILD_DIR = build

LIB_SHARED = $(BUILD_DIR)/libcir.so
LIB_STATIC = $(BUILD_DIR)/libcir.a

.PHONY: all
all: $(LIB_SHARED) $(LIB_STATIC) $(BUILD_DIR)/cas $(BUILD_DIR)/discas $(BUILD_DIR)/decbc

$(LIB_SHARED): core/cir.cpp core/cir.h
	$(CXX) -shared -fPIC -o $@ core/cir.cpp -DCIR_AS_LIB $(CFLAGS)

$(LIB_STATIC): core/cir.cpp core/cir.h
	$(CXX) -c -o $(BUILD_DIR)/cir.o core/cir.cpp $(CFLAGS)
	ar rcs $@ $(BUILD_DIR)/cir.o


$(BUILD_DIR)/cas: main.cpp $(LIB_SHARED)
	$(CXX) $(CFLAGS) -o $@ main.cpp $(LIB_SHARED)

$(BUILD_DIR)/discas: tools/disassembly/main.cpp $(LIB_SHARED)
	$(CXX) $(CFLAGS) -o $@ tools/disassembly/main.cpp $(LIB_SHARED) -I.

$(BUILD_DIR)/decbc: tools/debugger/main.cpp $(LIB_SHARED)
	$(CXX) $(CFLAGS) -o $@ tools/debugger/main.cpp $(LIB_SHARED) -I.


$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

examples/dl-imports/libexample.so:
	$(MAKE) -C examples/dl-imports libexample.so

run: all examples/dl-imports/libexample.so
	./$(BUILD_DIR)/cas ./example.cas -d examples/dl-imports/libexample.so

clean:
	rm -rf $(BUILD_DIR)
