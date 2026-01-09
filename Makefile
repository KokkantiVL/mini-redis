# Lite KV Store - Makefile
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -pthread -O2
DEBUG_FLAGS = -g -DDEBUG

# Directories
SRC_DIR = src
INC_DIR = include
BUILD_DIR = build

# Source files
SRCS = $(wildcard $(SRC_DIR)/*.cpp)
OBJS = $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(SRCS))

# Target executable
TARGET = lite-kvstore

# Default target
all: $(BUILD_DIR) $(TARGET)

# Create build directory
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Link object files
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

# Compile source files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -I$(INC_DIR) -c $< -o $@

# Debug build
debug: CXXFLAGS += $(DEBUG_FLAGS)
debug: clean all

# Clean build artifacts
clean:
	rm -rf $(BUILD_DIR) $(TARGET)

# Run the server
run: $(TARGET)
	./$(TARGET)

# Run with custom port
run-port: $(TARGET)
	./$(TARGET) $(PORT)

.PHONY: all clean debug run run-port
