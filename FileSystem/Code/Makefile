# run `make init` to initialize project
# run `make` to build the project, using default mode specified in the makefile. can either be release or debug. by default it's debug. Make sure you have run `make init` before running this command.
# run `make debug` to build the project in debug mode.
# run `make release` to build the project in release mode.
# run `make run` to run the executable

# Set the Default build. Set to `release` to get the release build (optimized binary without debug statements)
BUILD_DEFAULT = debug

# Directories
BUILD_DIR=build
SRC_DIR=src
INCLUDE_DIR=include
TEST_DIR=test
TEST_PROGRAMS_DIR=$(TEST_DIR)/t

APP_DIR=app

APP_PROGRAMS := $(wildcard $(APP_DIR)/*.c)
APP_PROGRAMS_BIN := $(patsubst $(APP_PROGRAMS)/%.c, $(BUILD_DIR)/%.out, $(APP_PROGRAMS))

# Target executable
TARGET_NAME=libfs.a
TARGET=$(BUILD_DIR)/$(TARGET_NAME)

# Shell Commands
CC=gcc
AR=ar rcs
MKDIR=mkdir -p
RM=rm -rf
CP=cp

# useful utility to convert lowercase to uppercase. 
UPPERCASE_CMD = tr '[:lower:][\-/]' '[:upper:][__]'

# Flags for compiler and other programs
# TODO : Remember to remove the -Wno flags before the release
CFLAGS=-Wall -Wextra
VALG_FLAGS = --leak-check=full --track-origins=yes
DEBUG_FLAGS = -g -DDEBUG
RELEASE_FLAGS = -O3 -march=native 
LINKER_FLAGS = -lreadline -lncurses

# Color codes for print statements
GREEN = \033[1;32m
CYAN = \033[1;36m
RED = \033[1;31m
RESET = \033[0m

# Verbosity control. Inspired from the Contiki-NG build system. A few hacks here and there, will probably improve later.
ifeq ($(V),1)
  TRACE_CC =
  TRACE_LD =
  TRACE_MKDIR =
  TRACE_CP =
	TRACE_AR =
	TRACE_RUN =
  Q ?=
  
  BUILD_SUCCESS=
  BUILD_FAILURE=:
  LINK_FAILURE=:
  INIT_SUCCESS=
  INIT_MAIN=
  RUN=
  VALGRIND_RUN=
  
  CLEAN=
  MK_INIT_ERROR=
else

  TRACE_CC       = @echo "$(CYAN)   CC     $(RESET)" $<
  TRACE_LD       = @echo "$(CYAN)   LD     $(RESET)" $@
	TRACE_AR 		   = @echo "$(CYAN)   AR     $(RESET)" $@
  TRACE_MKDIR    = @echo "$(CYAN)   MKDIR  $(RESET)" $@
  TRACE_CP       = @echo "$(CYAN)   CP     $(RESET)" $< "-->" $@
	TRACE_RUN      = @echo "$(GREEN)   RUN    $(RESET)" $<
  Q ?= @
  
  BUILD_SUCCESS  =@echo "   --      $(GREEN)Build successful in $(BUILD_DEFAULT) mode.$(RESET)"
  BUILD_FAILURE  =echo  "   --      $(RED)Build failed.$(RESET)"; exit 1
  LINK_FAILURE   =echo  "   --      $(RED)Linking failed.$(RESET)"; exit 1
  INIT_MAIN      =@echo "   --      $(CYAN)Creating main.c$(RESET)"
  INIT_SUCCESS   =@echo "   --      $(GREEN)Initialized the project structure$(RESET)"
  RUN            =@echo "   --      $(CYAN)Executing$(RESET): $(TARGET_NAME)"
  VALGRIND_RUN   =@echo "   --      $(CYAN)Running Valgrind on$(RESET): $(TARGET_NAME)"
  CLEAN          =@echo "   --      $(GREEN)Cleaned$(RESET): $(BUILD_DIR)/*"
  
  MK_INIT_ERROR  =@echo "$(RED)Error: $(SRC_DIR) directory doesn't exist. Please run make init to initialize the project.$(RESET)"
endif

# Sets flags based on the build mode.
ifeq ($(BUILD_DEFAULT), release)
  CFLAGS += $(RELEASE_FLAGS)
else
  CFLAGS += $(DEBUG_FLAGS)
endif

# Find all the source files and corresponding objects
SRCS := $(wildcard $(SRC_DIR)/*.c)
OBJS := $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(SRCS))

SRC_DIR_EXISTS := $(shell if [ -d "$(SRC_DIR)" ]; then echo 1; else echo 0; fi)

# Checks if src directory exists. If it doesn't, probably they haven't run `make init` yet.
ifeq ($(SRC_DIR_EXISTS), 0)
all: 
	$(MK_INIT_ERROR)
else
all: $(TARGET)
endif

# The TARGET target depends on the generated object files. 
$(TARGET): $(OBJS)
	$(TRACE_AR)
	$(Q) $(AR) $@ $^ || ($(LINK_FAILURE))
	$(BUILD_SUCCESS)

# The object files' targets, depend on their corresponding source files.
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	$(TRACE_CC)
	$(Q) $(CC) $(CFLAGS) -I$(INCLUDE_DIR) -c $< -o $@ || ($(BUILD_FAILURE))

# Create the build, src and include directories if they don't exist.
$(BUILD_DIR) $(SRC_DIR) $(INCLUDE_DIR):
	$(TRACE_MKDIR)
	$(Q) $(MKDIR) $@

# Initializes the project directories, and creates a main.c file in the src directory.
init: $(BUILD_DIR) $(SRC_DIR) $(INCLUDE_DIR)
	$(INIT_SUCCESS)

# Cleans the build directory.
clean:
	$(Q) $(RM) $(BUILD_DIR)/*
	$(CLEAN)

# RUNS THE SHELL
ARGS=
shell: $(BUILD_DIR)/shell.out
	$(Q) $(TRACE_RUN)
	$(Q) $(BUILD_DIR)/shell.out $(ARGS)

$(BUILD_DIR)/shell.out: $(APP_DIR)/shell.c $(TARGET)
	$(TRACE_CC)
	$(Q) $(CC) $(CFLAGS) -I$(INCLUDE_DIR) $< -o $@ -L$(BUILD_DIR) -lfs -lm

ARGS=

driver: $(BUILD_DIR)/driver.out
	$(Q) $(TRACE_RUN)
	$(Q) $(BUILD_DIR)/driver.out $(ARGS)

$(BUILD_DIR)/driver.out: $(TEST_DIR)/driver.c $(TARGET)
	$(TRACE_CC)
	$(Q) $(CC) -I$(INCLUDE_DIR) $< -o $@ -L$(BUILD_DIR) -lfs -lm

# phony targets
.PHONY: all init run debug release valgrind clean