CC = gcc
CXX = g++
CXX_PLUGIN_DIR = $(shell $(CXX) -print-file-name=plugin)

ifeq ($(MODE),)
	PLUGIN_FLAGS = -I./src -I$(CXX_PLUGIN_DIR)/include -O2 -fno-rtti -fPIC -Wno-write-strings -DNDEBUG
endif
ifeq ($(MODE),dbg)
	PLUGIN_FLAGS = -I./src -I$(CXX_PLUGIN_DIR)/include -O2 -fno-rtti -fPIC -Wno-write-strings
endif

PLUGIN_SOURCES = $(wildcard src/*.cc) \
				 $(wildcard src/ifswitch/*.cc) \
				 $(wildcard src/initstruct/*.cc)
PLUGIN_HEADERS = $(wildcard src/*.h) \
				 $(wildcard src/ifswitch/*.h) \
				 $(wildcard src/initstruct/*.h)
PLUGIN_OBJS = $(PLUGIN_SOURCES:%.cc=%.o)
PLUGIN_SONAME = ./portcosmo.so

EXAMPLE_FLAGS = -I./examples -O2
EXAMPLE_MODFLAGS = $(EXAMPLE_FLAGS) -fplugin=$(PLUGIN_SONAME) -include examples/tmpconst.h -include ./tmpconst.h
EXAMPLE_BINS = ./examples/ex1_result ./examples/ex1_modded

all: $(EXAMPLE_BINS)

test: $(EXAMPLE_BINS)
	./examples/ex1_result
	./examples/ex1_modded

./examples/%.o: ./examples/%.c ./examples/tmpconst.h
	$(CC) $(EXAMPLE_FLAGS) $< -c -o $@

./examples/ex1_modded.o: ./examples/ex1_modded.c $(PLUGIN_SONAME)
	$(CC) $(EXAMPLE_MODFLAGS) $< -c -o $@

./examples/ex1_modded: ./examples/ex1_modded.o ./examples/functions.o ./examples/supp.o
	$(CC) $^ -o $@

./examples/ex1_result: ./examples/ex1_result.o ./examples/supp.o ./examples/functions.o
	$(CC) $^ -o $@

$(PLUGIN_SONAME): $(PLUGIN_OBJS)
	$(CXX) $^ -shared -o $@

./src/%.o:	./src/%.cc $(PLUGIN_HEADERS)
	$(CXX) $(PLUGIN_FLAGS) $< -c -o $@

clean:
	rm -f ./examples/*.o
	rm -f $(EXAMPLE_BINS)
	rm -f ./src/*.o ./src/ifswitch/*.o ./src/initstruct/*.o
	rm -f $(PLUGIN_SONAME)
