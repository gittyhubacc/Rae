CC := clang
CFLAGS := -g -O0 -I./include #-DSCFE_DEBUG
LDFLAGS := -lmf -lLLVM

BIN_NAME := raec

SRC_DIR := ./src
BIN_DIR := ./bin
TST_DIR := ./tst
INC_DIR := ./tst/include
EXAMPLES_DIR := ./examples

SRCS := $(shell find $(SRC_DIR) -name '*.c')
OBJS := $(SRCS:%.c=%.o)

TSTS := $(shell find $(TST_DIR) -name '*.rae')
TST_ENTRY := $(TST_DIR)/entry_point.o
TSTS_ROOT := $(basename $(notdir $(TSTS)))
TST_OBJS := $(addsuffix .ro, $(basename $(TSTS)))
TST_RUNNER := $(TST_DIR)/suite.sh

EXAMPLES := $(shell find $(EXAMPLES_DIR) -name '*.rae')
EXAMPLES_ROOT := $(basename $(notdir $(EXAMPLES)))
EXAMPLES_OBJS := $(addsuffix .ro, $(basename $(EXAMPLES)))

BIN_TARGET = $(BIN_DIR)/$(BIN_NAME)
TST_TARGETS = $(addprefix $(BIN_DIR)/tst/, $(TSTS_ROOT))
EXAMPLE_TARGETS = $(addprefix $(BIN_DIR)/examples/, $(EXAMPLES_ROOT))

all: $(BIN_TARGET)

$(BIN_TARGET): $(OBJS)
	@mkdir -p $(dir $@)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

%.ro: %.rae $(BIN_TARGET)
	cat $< | gpp -C -c '//' -I$(INC_DIR) | $(BIN_TARGET) $@

tsts: $(TST_TARGETS)
	$(TST_RUNNER)

$(BIN_DIR)/tst/%: $(TST_DIR)/%.ro $(TST_ENTRY)
	@mkdir -p $(dir $@)
	$(CC) $< $(TST_ENTRY) -o $@

examples: $(EXAMPLE_TARGETS)

$(BIN_DIR)/examples/%: $(EXAMPLES_DIR)/%.ro $(TST_ENTRY)
	@mkdir -p $(dir $@)
	$(CC) $< $(TST_ENTRY) -o $@
	


.PRECIOUS: %.ro
.PHONY: clean install
clean:
	rm -f $(OBJS) $(TST_OBJS) $(EXAMPLES_OBJS) $(TST_ENTRY)
	rm -f $(BIN_TARGET) $(TST_TARGETS) $(EXAMPLE_TARGETS)
	if [ -d $(BIN_DIR)/examples ]; then rmdir $(BIN_DIR)/examples; fi
	if [ -d $(BIN_DIR)/tst ]; then rmdir $(BIN_DIR)/tst; fi
	if [ -d $(BIN_DIR) ]; then rmdir $(BIN_DIR); fi
