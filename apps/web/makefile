# WILL change when someone other than me makes this
RAEC := ../../bin/raec
DIST_LOCATION := /srv/http/rwa

# list of rae sources
SRCS := main.rae

# list of web artifacts
WEB_ARTIFACTS := env.js
WEB_ARTIFACTS += index.js
WEB_ARTIFACTS += index.html 
WEB_ARTIFACTS += favicon.ico

# project structure
OBJ_DIR := ./obj
BIN_DIR := ./dist
SRC_DIR := ./src
INC_DIR := ./include

OBJS := $(SRCS:%.rae=$(OBJ_DIR)/%.o)
WEB_ARTIFACTS := $(patsubst %, $(BIN_DIR)/%, $(WEB_ARTIFACTS))

BIN_TARGET := $(BIN_DIR)/index.wasm

all: $(BIN_TARGET) $(WEB_ARTIFACTS)

$(BIN_TARGET): $(OBJS)
	@mkdir -p $(dir $@)
	wasm-ld --no-entry \
		--import-undefined \
		--initial-memory=6553600 \
		--export=main_entry_point \
		-o $@ $(OBJS)

# rae code
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.rae
	@mkdir -p $(dir $@)
	cat $< | gpp -C -c '//' -I$(INC_DIR) | $(RAEC) -target wasm $@

# web artifacts
$(BIN_DIR)/%: $(SRC_DIR)/%
	@mkdir -p $(dir $@)
	cp $< $@

.PHONY: clean install
install: all
	sudo cp dist/* $(DIST_LOCATION)
clean:
	rm -f $(BIN_TARGET) $(WEB_ARTIFACTS) $(OBJS)
	rmdir $(BIN_DIR) $(OBJ_DIR)
