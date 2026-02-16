CC = gcc
CFLAGS = -Wall -Wextra -g -I./include
LDFLAGS = 

SRC_DIR = src
INC_DIR = include
BUILD_DIR = build

SOURCES = $(wildcard $(SRC_DIR)/*.c)
OBJECTS = $(SOURCES:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)
TARGET = simpleredis

. PHONY: all clean run debug

all: $(BUILD_DIR) $(TARGET)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) $(LDFLAGS) -o $(TARGET)
	@echo "✓ Build complete:  ./$(TARGET)"

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR) $(TARGET)
	@echo "✓ Clean complete"

run: $(TARGET)
	./$(TARGET)

debug: CFLAGS += -DDEBUG -O0
debug: clean $(TARGET)
	@echo "✓ Debug build complete"