#Symbol Guide:
# $ indicates this will be a Makefile-specific keyword
# $@: this is replaced with the name of the target it is in
# $^: this is replaced with the list of ALL dependencies for the target it is in
# $<: this is replaced with the FIRST dependency for the target it is in
# $(pat)
# $(wildcard <some use of the * operator>): this expands to all the files that match the regex created with *
# %<suffix>: this searches the specified directory (relative to this Makefile) and all its subdirectories 
#and finds all files with a matching suffix and creates a target for each of them
# $(patsubst pattern,replacement,names): searches pattern for names and creates 
CC = i686-elf-gcc 
AS = i686-elf-as

CFLAGS := -ffreestanding -Wall -Wextra -Werror -O2 -Isrc
LDFLAGS := -T linker.ld -ffreestanding -O2 -nostdlib -lgcc

VPATH := src/boot src/include
SRCDIR := src
BUILDDIR := build

SOURCES := $(shell find $(SRCDIR) -type f -name '*.c')
ASSEMBLY := $(shell find $(SRCDIR) -type f -name '*.S')
OBJECTS := $(patsubst %.c,$(BUILDDIR)/%.o,$(notdir $(SOURCES))) $(patsubst %.S,$(BUILDDIR)/%.o,$(notdir $(ASSEMBLY)))  

TARGET := bin/maybeOS.elf

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
	$(CC) -E -P $< -o build/$*.i
	$(AS) build/$*.i -o $@

# $(BUILDDIR)/%.o: $(SRCDIR)/%.c
# 	$(CC) $(CFLAGS) -c $< -o $@

# $(BUILDDIR)/%.o: $(SRCDIR)/%.S
# 	$(CC) -E -P $< -o $*.i
# 	$(AS) $*.i -o $@ 

clean:
	rm -rf $(BUILDDIR)/*.o $(TARGET)
