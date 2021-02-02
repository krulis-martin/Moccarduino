CPP=g++
CFLAGS=-Wall -O3 -std=c++17
INCLUDE=../shared
HEADERS=$(shell find . -name '*.hpp') $(shell find ../shared -name '*.hpp')
SOURCES=$(shell find . -name '*.cpp')
SHARED_SOURCES=$(shell find ../shared -name '*.cpp')
OBJS=$(patsubst ./%,./.objs/%,$(SOURCES:%.cpp=%.o))
SHARED_OBJS=$(patsubst ../shared/%,./.shobjs/%,$(SHARED_SOURCES:%.cpp=%.o))
TARGET=test_funshield_seg_display


.PHONY: all clear clean purge

all: $(TARGET)

# Calculate dependencies...

Makefile.dep: $(SOURCES) $(HEADERS)
	@echo Calculating dependencies...
	@$(CPP) $(CFLAGS) -MM $(addprefix -I,$(INCLUDE)) $(SOURCES) > $@

-include Makefile.dep


# Building Targets

$(TARGET): .objs .shobjs $(OBJS) $(SHARED_OBJS) $(HEADERS)
	@echo Compiling and linking executable "$@" ...
	@$(CPP) $(CFLAGS) $(addprefix -I,$(INCLUDE)) $(LDFLAGS) $(addprefix -L,$(LIBDIRS)) $(addprefix -l,$(LIBS)) $(OBJS) $(SHARED_OBJS) -o $@

.objs:
	@mkdir -p "$@"

.shobjs:
	@mkdir -p "$@"

.objs/%.o: %.cpp
	@echo Compiling \'"$@"\' ...
	@$(CPP) -c $(CFLAGS) $(addprefix -I,$(INCLUDE)) "$<" -o "$@"

.shobjs/%.o: ../shared/%.cpp
	@echo Compiling \'"$@"\' ...
	@$(CPP) -c $(CFLAGS) $(addprefix -I,$(INCLUDE)) "$<" -o "$@"


# Cleaning Stuff

clear:
	@echo Removing object files ...
	-@rm -rf ./.objs
	-@rm -rf ./.shobjs

clean: clear

purge: clear
	@echo Removing executable ...
	-@rm -f ./$(TARGET) ./Makefile.dep
