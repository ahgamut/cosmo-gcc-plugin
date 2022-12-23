CC = gcc
CXX = g++

CXX_PLUGIN_DIR = $(shell $(CXX) -print-file-name=plugin)
PLUGIN_FLAGS = -I./src -I$(CXX_PLUGIN_DIR) -O2 -fno-rtti -fPIC

PLUGIN_SOURCES = $(wildcard src/*.cpp)
PLUGIN_HEADERS = $(wildcard src/*.h)

EXAMPLE_FLAGS = -I./examples -O2
EXAMPLE_BINS = ./examples/ex1_default ./examples/ex1_result

all: $(EXAMPLE_BINS)

test: $(EXAMPLE_BINS)
	./examples/ex1_default
	./examples/ex1_result

./src/%.o:	./src/%.cpp
	$(CXX) $(PLUGIN_FLAGS) $< -c -o $@

./examples/%.o: ./examples/%.c
	$(CC) $(EXAMPLE_FLAGS) $< -c -o $@

./examples/ex1_default: ./examples/ex1_default.o ./examples/functions.o
	$(CC) $^ -o $@

./examples/ex1_result: ./examples/ex1_result.o ./examples/supp.o ./examples/functions.o
	$(CC) $^ -o $@

clean:
	rm -f ./examples/*.o
	rm -f $(EXAMPLE_BINS)
	rm -f ./src/*.o
