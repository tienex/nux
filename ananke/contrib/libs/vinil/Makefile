# VINIL - Vincent Intermediate Language Unified Library
# Makefile

# Compiler settings
CC ?= gcc
AR = ar
CFLAGS = -Wall -Wextra -O2 -fPIC -Iinclude -Isrc
LDFLAGS = -shared
LIBS = -lm -lpthread

# Directories
SRC_DIR = src
INC_DIR = include
BUILD_DIR = build
OBJ_DIR = $(BUILD_DIR)/obj
LIB_DIR = $(BUILD_DIR)/lib

# Target library names
STATIC_LIB = $(LIB_DIR)/libvinil.a
SHARED_LIB = $(LIB_DIR)/libvinil.so

# Source files
SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(SRCS:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)

# Header dependencies
HEADERS = $(wildcard $(INC_DIR)/vinil/*.h)

# Targets
.PHONY: all clean install shared static example

all: shared static

example: shared
	$(CC) $(CFLAGS) example.c -o $(BUILD_DIR)/example -L$(LIB_DIR) -lvinil $(LIBS)

shared: $(SHARED_LIB)

static: $(STATIC_LIB)

# Create build directories
$(OBJ_DIR) $(LIB_DIR):
	mkdir -p $@

# Compile source files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c $(HEADERS) | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Build shared library
$(SHARED_LIB): $(OBJS) | $(LIB_DIR)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)

# Build static library
$(STATIC_LIB): $(OBJS) | $(LIB_DIR)
	$(AR) rcs $@ $(OBJS)

# Clean build artifacts
clean:
	rm -rf $(BUILD_DIR)

# Install (to be implemented)
install: all
	@echo "Install target not yet implemented"
	@echo "To use vinil, add -I$(PWD)/include and -L$(PWD)/$(LIB_DIR) to your build"

# Show build information
info:
	@echo "VINIL Build Information"
	@echo "======================="
	@echo "Source files: $(SRCS)"
	@echo "Object files: $(OBJS)"
	@echo "Static library: $(STATIC_LIB)"
	@echo "Shared library: $(SHARED_LIB)"
	@echo "Compiler: $(CC)"
	@echo "CFLAGS: $(CFLAGS)"
