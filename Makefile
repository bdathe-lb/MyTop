
# The name of the final executable
TARGET_EXEC := mytop

# Directory definition
SRC_DIR := src
INC_DIR := include
BUILD_DIR := build

# Comiler and related options
CC := gcc
CFLAGS := -I$(INC_DIR) -Wall -Wextra -O0 -g -MMD -MP -fno-omit-frame-pointer -Wformat=2
LDFLAGS := 

# Automated inference
SRCS := $(wildcard $(SRC_DIR)/*.c)
OBJS := $(SRCS:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)
DEPS := $(OBJS:.o=.d)

# Compilation rules
all: $(BUILD_DIR)/$(TARGET_EXEC)

# Linking
$(BUILD_DIR)/$(TARGET_EXEC): $(OBJS)
	@echo "Linking target: $@"
	@$(CC) $(OBJS) -o $@ $(LDFLAGS)
	@echo "Build completed!"

# Compilation
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	@echo "Compiling: $<"
	@$(CC) $(CFLAGS) -c $< -o $@

# Create build/
$(BUILD_DIR):
	@mkdir -p $@

# Import dependence file
-include $(DEPS)

# Run
run: all
	@./$(BUILD_DIR)/$(TARGET_EXEC)

# Debug
debug: all
	@gdb -tui ./$(BUILD_DIR)/$(TARGET_EXEC)

# Check
mem-check:
	@valgrind \
		--tool=memcheck \
		--leak-check=full \
		--show-leak-kinds=all \
		--track-origins=yes \
		--num-callers=20 \
		--error-limit=no \
		--log-file=vg.log \
		./$(BUILD_DIR)/$(TARGET_EXEC)

# Clean
clean:
	@echo "Cleaning build directory..."
	@rm -rf $(BUILD_DIR)
	@echo "Clean completed!"

.PHONY: all run debug clean
