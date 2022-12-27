CC = gcc
CXX = g++

CXX_PLUGIN_DIR = $(shell $(CXX) -print-file-name=plugin)
PLUGIN_FLAGS = -I./src -I$(CXX_PLUGIN_DIR)/include -O2 -fno-rtti -fPIC -Wno-write-strings

PLUGIN_SOURCES = $(wildcard src/*.cpp)
PLUGIN_OBJS = $(PLUGIN_SOURCES:%.cpp=%.o)
PLUGIN_HEADERS = $(wildcard src/*.h)
PLUGIN_SONAME = ./src/ifswitch.so

EXAMPLE_FLAGS = -I./examples -O2
EXAMPLE_MODFLAGS = $(EXAMPLE_FLAGS) -fplugin=$(PLUGIN_SONAME)
EXAMPLE_BINS = ./examples/ex1_default ./examples/ex1_result ./examples/ex1_modded

all: $(EXAMPLE_BINS)

test: $(EXAMPLE_BINS)
	./examples/ex1_default
	./examples/ex1_result
	./examples/ex1_modded

./examples/%.o: ./examples/%.c
	$(CC) $(EXAMPLE_FLAGS) $< -c -o $@

./examples/ex1_modded.o: ./examples/ex1_modded.c ./src/ifswitch.so
	$(CC) $(EXAMPLE_MODFLAGS) $< -c -o $@

./examples/ex1_default: ./examples/ex1_default.o ./examples/functions.o
	$(CC) $^ -o $@

./examples/ex1_modded: ./examples/ex1_modded.o ./examples/functions.o
	$(CC) $^ -o $@

./examples/ex1_result: ./examples/ex1_result.o ./examples/supp.o ./examples/functions.o
	$(CC) $^ -o $@

./src/ifswitch.so: $(PLUGIN_OBJS)
	$(CXX) $^ -shared -o $@

./src/%.o:	./src/%.cpp
	$(CXX) $(PLUGIN_FLAGS) $< -c -o $@



clean:
	rm -f ./examples/*.o
	rm -f $(EXAMPLE_BINS)
	rm -f ./src/*.o
	rm -f ./src/ifswitch.so
