.PHONY: all clean
CC = gcc
CFLAGS = -Wall -pthread

SRC_DIR := src
OBJ_DIR := obj
BIN_DIR := bin

SHARING := $(BIN_DIR)/sharing
STEALING := $(BIN_DIR)/stealing
SRC_QUICKSORT := $(SRC_DIR)/quicksort.c
SRC_SHARING := $(SRC_DIR)/work_sharing.c
SRC_STEALING := $(SRC_DIR)/work_stealing.c
OBJ_SHARING := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRC_SHARING))
OBJ_STEALING := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRC_STEALING))

all: sharing stealing
sharing: $(SHARING)
stealing: $(STEALING)

$(SHARING): $(SRC_QUICKSORT) $(OBJ_SHARING)
	@mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $^


$(STEALING): $(SRC_QUICKSORT) $(OBJ_STEALING)
	@mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $^


$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

clean: clean_obj clean_bin

clean_obj:
	rm -rf $(OBJ_DIR)

clean_bin:
	rm -rf $(BIN_DIR)