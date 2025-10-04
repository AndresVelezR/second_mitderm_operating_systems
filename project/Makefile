# Makefile for modular image processing project
# WHAT: Automates incremental compilation of modular C project
# HOW: Uses pattern rules, variables and automatic dependencies
# WHY: Improves productivity and avoids unnecessary recompilation

# ============================================================================
# CONFIGURATION VARIABLES
# ============================================================================

CC = gcc
CFLAGS = -O2 -Wall -Wextra -pthread -Iinclude -Istb
LDFLAGS = -lm -pthread

# Directories
SRC_DIR = src
OBJ_DIR = obj
INC_DIR = include
STB_DIR = stb
RESULTS_DIR = results

# Executable
TARGET = img_processor

# Source files (all .c files in src/)
SRCS = $(wildcard $(SRC_DIR)/*.c)

# Object files (one for each .c)
OBJS = $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRCS))

# Headers (for dependencies)
HEADERS = $(wildcard $(INC_DIR)/*.h)

# ============================================================================
# MAIN RULES
# ============================================================================

# Default target: compile everything
all: $(TARGET)

# Create executable from objects
$(TARGET): $(OBJS)
	@echo "Linking $@..."
	$(CC) $(OBJS) -o $@ $(LDFLAGS)
	@echo "Compilation successful: ./$(TARGET)"

# Pattern rule: compile .c to .o
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c $(HEADERS) | $(OBJ_DIR)
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) -c $< -o $@

# Create directories if they don't exist
$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

$(RESULTS_DIR):
	mkdir -p $(RESULTS_DIR)

# ============================================================================
# UTILITIES
#============================================================================

# Clean generated files
clean:
	@echo "Cleaning build files..."
	rm -rf $(OBJ_DIR) $(TARGET)
	@echo "Clean complete"

# Clean everything including results
clean-all: clean
	@echo "Cleaning results..."
	rm -rf $(RESULTS_DIR)
	@echo "Full clean complete"

# Complete rebuild
rebuild: clean all

# Run with test image
run: all | $(RESULTS_DIR)
	@echo "Running image processor..."
	./$(TARGET)

# Show help
help:
	@echo "Makefile for modular image processing"
	@echo ""
	@echo "Available targets:"
	@echo "  make           - Compile project (default target)"
	@echo "  make all       - Compile project"
	@echo "  make clean     - Remove compilation files"
	@echo "  make clean-all - Remove compilation files and results"
	@echo "  make rebuild   - Clean and recompile everything"
	@echo "  make run       - Compile and execute"
	@echo "  make help      - Show this help"
	@echo ""
	@echo "Program usage:"
	@echo "  ./$(TARGET) [image_path.png]"

# Avoid conflicts with files named 'clean', 'all', etc.
.PHONY: all clean clean-all rebuild run help
