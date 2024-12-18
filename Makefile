SRC_DIR = src
INCLUDE_DIR = include
BIN_DIR = bin
CC = clang
NAME = flapjack

C_SRC = $(shell find $(SRC_DIR) -name *.c)

C_FLAGS=-g 

build: $(OBJS)
	@mkdir -p $(BIN_DIR)
	$(CC) $(C_FLAGS) -I $(INCLUDE_DIR) -o $(BIN_DIR)/$(NAME) $(C_SRC)

run:
	./$(BIN_DIR)/$(NAME) 

.PHONY: clean
clean:
	@rm -rf $(OBJ_DIR)
	@rm -rf $(BIN_DIR)
	@rm -rf $(DEP_DIR)
