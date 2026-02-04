CC := clang
RAEC := ../../bin/raec
CFLAGS := -g -O0 -fPIC
LDFLAGS := -llua

OBJ_DIR := ./obj
INC_DIR := ./include

C_LIB_NAME := env.so

SRCS := env.c filters.rae
OBJS := $(OBJ_DIR)/env.o $(OBJ_DIR)/filters.o

all: $(C_LIB_NAME)

$(C_LIB_NAME): $(OBJS)
	@mkdir -p $(dir $@)
	$(CC) $(OBJS) -o $@ -shared $(LDFLAGS)

$(OBJ_DIR)/filters.o: filters.rae
	@mkdir -p $(dir $@)
	cat $< | gpp -C -c '//' -I$(INC_DIR) | $(RAEC) $@

$(OBJ_DIR)/env.o: env.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: clean
clean:
	rm -f $(OBJS) $(C_LIB_NAME)
	rmdir $(OBJ_DIR)
