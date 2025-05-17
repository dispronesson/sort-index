CC = gcc
CFLAGS_DEBUG = -g -ggdb -pthread -std=c17 -pedantic -W -Wall -Wextra -Wno-unused-parameter -Wno-unused-variable
CFLAGS_RELEASE = -std=c17 -pthread -pedantic -W -Wall -Wextra -Werror -Wno-unused-parameter -Wno-unused-variable
LDFLAGS = -pthread

SRC_GENERATOR_DIR = src/generator
SRC_SORT_CHECK_DIR = src/sort_check
SRC_SORT_INDEX_DIR = src/sort_index

BUILD_DIR = build

DEBUG_GENERATOR_DIR = $(BUILD_DIR)/debug/generator
DEBUG_SORT_CHECK_DIR = $(BUILD_DIR)/debug/sort_check
DEBUG_SORT_INDEX_DIR = $(BUILD_DIR)/debug/sort_index

RELEASE_GENERATOR_DIR = $(BUILD_DIR)/release/generator
RELEASE_SORT_CHECK_DIR = $(BUILD_DIR)/release/sort_check
RELEASE_SORT_INDEX_DIR = $(BUILD_DIR)/release/sort_index

SRC_GENERATOR_FILES = $(SRC_GENERATOR_DIR)/generator.c
SRC_SORT_CHECK_FILES = $(SRC_SORT_CHECK_DIR)/sort_check.c
SRC_SORT_INDEX_FILES = $(SRC_SORT_INDEX_DIR)/main.c $(SRC_SORT_INDEX_DIR)/sort_index.c $(SRC_SORT_INDEX_DIR)/valid_args.c

OBJ_FILES_GENERATOR_DEBUG = $(patsubst $(SRC_GENERATOR_DIR)/%.c, $(DEBUG_GENERATOR_DIR)/%.o, $(SRC_GENERATOR_FILES))
OBJ_FILES_GENERATOR_RELEASE = $(patsubst $(SRC_GENERATOR_DIR)/%.c, $(RELEASE_GENERATOR_DIR)/%.o, $(SRC_GENERATOR_FILES))

OBJ_FILES_SORT_CHECK_DEBUG = $(patsubst $(SRC_SORT_CHECK_DIR)/%.c, $(DEBUG_SORT_CHECK_DIR)/%.o, $(SRC_SORT_CHECK_FILES))
OBJ_FILES_SORT_CHECK_RELEASE = $(patsubst $(SRC_SORT_CHECK_DIR)/%.c, $(RELEASE_SORT_CHECK_DIR)/%.o, $(SRC_SORT_CHECK_FILES))

OBJ_FILES_SORT_INDEX_DEBUG = $(patsubst $(SRC_SORT_INDEX_DIR)/%.c, $(DEBUG_SORT_INDEX_DIR)/%.o, $(SRC_SORT_INDEX_FILES))
OBJ_FILES_SORT_INDEX_RELEASE = $(patsubst $(SRC_SORT_INDEX_DIR)/%.c, $(RELEASE_SORT_INDEX_DIR)/%.o, $(SRC_SORT_INDEX_FILES))

EXEC_GENERATOR_NAME = generator
EXEC_SORT_CHECK_NAME = sortcheck
EXEC_SORT_INDEX_NAME = sortindex

DEBUG_TARGETS = $(DEBUG_GENERATOR_DIR)/$(EXEC_GENERATOR_NAME) \
				$(DEBUG_SORT_CHECK_DIR)/$(EXEC_SORT_CHECK_NAME) \
				$(DEBUG_SORT_INDEX_DIR)/$(EXEC_SORT_INDEX_NAME)

RELEASE_TARGETS = $(RELEASE_GENERATOR_DIR)/$(EXEC_GENERATOR_NAME) \
				  $(RELEASE_SORT_CHECK_DIR)/$(EXEC_SORT_CHECK_NAME) \
				  $(RELEASE_SORT_INDEX_DIR)/$(EXEC_SORT_INDEX_NAME)

all: debug

debug: $(DEBUG_TARGETS)

release: $(RELEASE_TARGETS)

define compile_rule
$2/%.o: $1/%.c | $2/
	$(CC) $3 -c $$< -o $$@
endef

$(eval $(call compile_rule,$(SRC_GENERATOR_DIR),$(DEBUG_GENERATOR_DIR),$(CFLAGS_DEBUG)))
$(eval $(call compile_rule,$(SRC_GENERATOR_DIR),$(RELEASE_GENERATOR_DIR),$(CFLAGS_RELEASE)))

$(eval $(call compile_rule,$(SRC_SORT_CHECK_DIR),$(DEBUG_SORT_CHECK_DIR),$(CFLAGS_DEBUG)))
$(eval $(call compile_rule,$(SRC_SORT_CHECK_DIR),$(RELEASE_SORT_CHECK_DIR),$(CFLAGS_RELEASE)))

$(eval $(call compile_rule,$(SRC_SORT_INDEX_DIR),$(DEBUG_SORT_INDEX_DIR),$(CFLAGS_DEBUG)))
$(eval $(call compile_rule,$(SRC_SORT_INDEX_DIR),$(RELEASE_SORT_INDEX_DIR),$(CFLAGS_RELEASE)))

define link_rule
$2/$3: $1
	$(CC) $(LDFLAGS) $$^ -o $$@
endef

$(eval $(call link_rule,$(OBJ_FILES_GENERATOR_DEBUG),$(DEBUG_GENERATOR_DIR),$(EXEC_GENERATOR_NAME)))
$(eval $(call link_rule,$(OBJ_FILES_GENERATOR_RELEASE),$(RELEASE_GENERATOR_DIR),$(EXEC_GENERATOR_NAME)))

$(eval $(call link_rule,$(OBJ_FILES_SORT_CHECK_DEBUG),$(DEBUG_SORT_CHECK_DIR),$(EXEC_SORT_CHECK_NAME)))
$(eval $(call link_rule,$(OBJ_FILES_SORT_CHECK_RELEASE),$(RELEASE_SORT_CHECK_DIR),$(EXEC_SORT_CHECK_NAME)))

$(eval $(call link_rule,$(OBJ_FILES_SORT_INDEX_DEBUG),$(DEBUG_SORT_INDEX_DIR),$(EXEC_SORT_INDEX_NAME)))
$(eval $(call link_rule,$(OBJ_FILES_SORT_INDEX_RELEASE),$(RELEASE_SORT_INDEX_DIR),$(EXEC_SORT_INDEX_NAME)))

%/:
	mkdir -p $@

clean:
	rm -fr $(BUILD_DIR)

.PHONY: all debug release clean