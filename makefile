SOURCE_DIR  := ./src
INCLUDE_DIR := ./include
BUILD_DIR   := ./build

CC        := gcc
CFLAGS    := -Wall
LDFLAGS   := -lm -lpthread -lrt
INC_FLAGS := -I$(INCLUDE_DIR)

.PHONY: all
all: $(BUILD_DIR) $(BUILD_DIR)/rastreador

$(BUILD_DIR)/%.o: $(SOURCE_DIR)/%.c
	$(CC) -c -o $@ $< $(CFLAGS) $(INC_FLAGS)

$(BUILD_DIR)/rastreador: $(BUILD_DIR)/rastreador.o $(BUILD_DIR)/rastreador_main.o $(BUILD_DIR)/utils.o
	$(CC) -o $@ $^ $(LDFLAGS)

$(BUILD_DIR):
	mkdir -p $@


.PHONY: clean
clean:
	rm -rf $(BUILD_DIR)
