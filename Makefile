SRC_DIR = src
INCLUDE_DIR = include
BIN_DIR = bin
CC = clang
NAME = flapjack

CPP_SRC = $(shell find $(SRC_DIR) -name *.cpp)

CPP_FLAGS=-g -std=c++20 -xc++ -lstdc++

build: $(OBJS)
	@mkdir -p $(BIN_DIR)
	$(CC) $(CPP_FLAGS) -I $(INCLUDE_DIR) -o $(BIN_DIR)/$(NAME) $(CPP_SRC)

run:
	./$(BIN_DIR)/$(NAME) 

.PHONY: clean
clean:
	@rm -rf $(OBJ_DIR)
	@rm -rf $(BIN_DIR)
	@rm -rf $(DEP_DIR)
