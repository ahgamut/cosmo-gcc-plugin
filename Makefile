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
EXAMPLE_MODFLAGS = $(EXAMPLE_FLAGS) -fplugin=$(PLUGIN_SONAME) -DUSING_PLUGIN=1\
				   -include examples/tmpconst.h -include ./tmpconst.h
EXAMPLE_RESFLAGS = $(EXAMPLE_FLAGS) -include examples/tmpconst.h -include ./tmpconst.h

EXAMPLE_SOURCES = $(wildcard examples/ex*.c)
EXAMPLE_RUNS = $(EXAMPLE_SOURCES:%.c=%.runs)
EXAMPLE_MODBINS = $(EXAMPLE_SOURCES:examples/%.c=examples/modded_%)
EXAMPLE_RESBINS = $(EXAMPLE_SOURCES:examples/%.c=examples/result_%)

EXAMPLE_BINS = $(EXAMPLE_MODBINS) $(EXAMPLE_RESBINS)

all: $(EXAMPLE_RUNS) $(EXAMPLE_BINS)

$(EXAMPLE_RUNS): $(EXAMPLE_BINS)

./examples/%.runs: ./examples/modded_% ./examples/result_%
	diff -s <($(word 2,$^)) <($(word 1,$^))

./examples/%.o: ./examples/%.c ./examples/tmpconst.h
	$(CC) $(EXAMPLE_FLAGS) $< -c -o $@

./examples/modded_%.o: ./examples/%.c $(PLUGIN_SONAME)
	$(CC) $(EXAMPLE_MODFLAGS) $< -c -o $@

./examples/modded_%: ./examples/modded_%.o ./examples/functions.o ./examples/supp.o
	$(CC) $^ -o $@

./examples/result_%.o: ./examples/%.c $(PLUGIN_SONAME)
	$(CC) $(EXAMPLE_RESFLAGS) $< -c -o $@

./examples/result_%: ./examples/result_%.o ./examples/functions.o
	$(CC) $^ -o $@

$(PLUGIN_SONAME): $(PLUGIN_OBJS)
	$(CXX) $^ -shared -o $@

./src/%.o:	./src/%.cc $(PLUGIN_HEADERS)
	$(CXX) $(PLUGIN_FLAGS) $< -c -o $@

clean:
	rm -f ./examples/*.o
	rm -f $(EXAMPLE_BINS)
	rm -f $(EXAMPLE_RUNS)
	rm -f ./src/*.o ./src/ifswitch/*.o ./src/initstruct/*.o
	rm -f $(PLUGIN_SONAME)
