#Symbol Guide:
# $ indicates this will be a Makefile-specific keyword
# $@: this is replaced with the name of the target it is in
# $^: this is replaced with the list of ALL dependencies for the target it is in
# $<: this is replaced with the FIRST dependency for the target it is in

# $(wildcard <some use of the * operator>): this expands to all the files that match the regex created with *

# %<suffix>: this searches the specified directory (relative to this Makefile) and all its subdirectories 
#and finds all files with a matching suffix and creates a target for each of them

# $(patsubst pattern,replacement,names): takes inputs in names, searches for pattern in them, and if found replaces the pattern with replacement 


CC = i686-elf-gcc 
AS = i686-elf-as

SRCDIR := src
BUILDDIR := build

INCLUDE_PATHS := $(shell find $(SRCDIR)/include -type d | sed 's/^/-I/')

#$(info $(INCLUDE_PATHS))

# the ?= operator checks if an env var exists of the same name as this Makefile var, and if so sets this variable to the  the env var value; otherwise it uses the value specified here in the Makefile. If I want to compile w/ debugging symbols, I will make this an env var =-g, otherwise this var won't do anything
DEBUG_ENV_VAR ?= 
DEFINED_MACROS := -DPAGE_SIZE_4K -DARCH_x86
CFLAGS := -ffreestanding -Wall -Wextra $(INCLUDE_PATHS) $(DEFINED_MACROS) -O0 $(DEBUG_ENV_VAR) #-Werror
AFLAGS := -E -P $(INCLUDE_PATHS)
LDFLAGS := -T linker.ld -ffreestanding -nostdlib -lgcc -Wall -Wextra $(DEBUG_ENV_VAR) 

#these find commands recursively search the srcdir and grabs the relative path of all files matching the regex and returns them
SOURCES := $(shell find $(SRCDIR) -type f -name '*.c')
ASSEMBLY := $(shell find $(SRCDIR) -type f -name '*.S')
#notdir takes away the path of the file, just leaving the sources, like example.c instead of src/kernel/example.c
OBJECTS := $(patsubst %.c,$(BUILDDIR)/%.o,$(notdir $(SOURCES))) $(patsubst %.S,$(BUILDDIR)/%.o,$(notdir $(ASSEMBLY))) $(shell find $(SRCDIR) -type f -name '*.o')

#vpath is a special variable in Make that specifies a list of dirs that will be searched for targets and prereqs; its use here is to find all source code subdirs that may contain code so we can use a wildcard to compile them all
VPATH := $(shell find $(SRCDIR) -type d)

#$(info $(VPATH))

TARGET := bin/maybeOS.elf

#all object files in OBJECTS are also created as dependency files for each object file; dependency files allow
DEPS := $(OBJECTS:.o=.d)

#this just says all and clean aren't actual files
.PHONY: all clean

#$(info $(SOURCES))
#$(info $(OBJECTS))

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(LDFLAGS) -o $@ $^

-include $(DEPS)

build/%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

build/%.o: %.S
	$(CC) $(AFLAGS) $< -o build/$*.i
	$(AS) build/$*.i -o $@
	
clean:
	rm -rf $(BUILDDIR)/*.o $(BUILDDIR)/*.i $(TARGET)
