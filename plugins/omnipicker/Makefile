################################################################################
# Makefile for plugins
################################################################################

.PHONY: help build build-in-docker check count clean

#-------------------------------------------------------------------------------
# Constants
#-------------------------------------------------------------------------------

# Name of the software.
SOFTWARE_PATH := omnipicker

# Source code directory.
SRC_DIR := source
OBJ_DIR := objects

# Object directory for tests.
SRC_DIR_TEST := tests/source
OBJ_DIR_TEST := tests/objects

# Target C++ source files.
CXX_FILES := $(wildcard $(SRC_DIR)/*.cxx)
OBJ_FILES := $(patsubst $(SRC_DIR)/%,$(OBJ_DIR)/%,$(CXX_FILES:.cxx=.o))

# Target C++ source files for tests.
OBJ_TEST_MAIN     := $(patsubst $(SRC_DIR)/%,$(OBJ_DIR_TEST)/%,$(CXX_FILES:.cxx=.o))
OBJ_TEST_STREXTRA := $(filter-out $(OBJ_DIR_TEST)/main.o,$(OBJ_TEST_MAIN)) $(OBJ_DIR_TEST)/test_strextra.o
OBJ_TEST_UTILS    := $(filter-out $(OBJ_DIR_TEST)/main.o,$(OBJ_TEST_MAIN)) $(OBJ_DIR_TEST)/test_utils.o

# Target files for tests.
TARGETS_TEST := tests/test_main tests/test_strextra tests/test_utils

#-------------------------------------------------------------------------------
# C++ build settings
#-------------------------------------------------------------------------------

# Compile command.
CC      := g++ -O3 -flto=auto
CC_TEST := g++ -O0 -g -fPIC -coverage -fprofile-update=atomic -DGCOV_BUILD
CFLAGS  := -std=c++23 -Wall -Wextra -I/usr/local/include -I$(SRC_DIR)
LIBS    := -L/usr/local/lib -lncurses

# LCOV command.
LCOV    := lcov --quiet --gcov-tool gcov --ignore-errors gcov,gcov,inconsistent,range,corrupt -c
GENHTML := genhtml --quiet --ignore-errors gcov,gcov,inconsistent,range,corrupt

# Colors.
RED     := \033[38;2;204;102;102m
GREEN   := \033[38;2;181;189;104m
YELLOW  := \033[38;2;240;198;116m
BLUE    := \033[38;2;129;162;190m
MAGENTA := \033[38;2;178;148;187m
CYAN    := \033[38;2;138;190;183m
GRAY    := \033[38;2;197;200;198m
NC      := \033[0m

#-------------------------------------------------------------------------------
# Help message
#-------------------------------------------------------------------------------

help:
	@echo $(OBJ_TEST_MAIN)
	@echo $(OBJ_TEST_STREXTRA)
	@echo $(OBJ_TEST_UTILS)
	@echo "Usage:"
	@echo "    make <command>"
	@echo ""
	@echo "Build commands:"
	@echo "    build             Build C++ plugin via Docker"
	@echo "    build-in-docker   Build command called inside Docker container"
	@echo ""
	@echo "Code check commands:"
	@echo "    check             Check the code quality"
	@echo "    count             Count the lines of code"
	@echo ""
	@echo "Other commands:"
	@echo "    clean             Cleanup cache files"
	@echo "    help              Show this message"

#-------------------------------------------------------------------------------
# Build commands
#-------------------------------------------------------------------------------

build: $(OBJ_DIR)
	@printf "$(GRAY)------------------------------------------------------------$(NC)\n"
	@printf "$(GRAY)Building omnipicker$(NC)\n"
	@printf "$(GRAY)------------------------------------------------------------$(NC)\n"
	make $(SOFTWARE_PATH)

$(OBJ_DIR):
	@printf "$(YELLOW)[CMD] $(GREEN)mkdir $(NC)-p $(BLUE)$(OBJ_DIR)$(NC)\n"
	@mkdir -p $(OBJ_DIR)

$(SOFTWARE_PATH): $(OBJ_FILES)
	@printf "$(YELLOW)[LNK] $(GREEN)$(OBJ_DIR)/*.o $(NC)-> $(BLUE)$(SOFTWARE_PATH)$(NC)\n"
	@$(CC) $(CFLAGS) -o $(SOFTWARE_PATH) $(OBJ_FILES) $(LIBS) -static

$(OBJ_DIR)/%.o: source/%.cxx
	@printf "$(YELLOW)[C++] $(GREEN)$(<) $(NC)-> $(BLUE)$(@)$(NC)\n"
	@$(CC) $(CFLAGS) -c $(<) -o $(@)

#-------------------------------------------------------------------------------
# Test commands
#-------------------------------------------------------------------------------

test: $(OBJ_DIR_TEST) $(TARGETS_TEST)
	cd tests && sh test_main.sh && ./test_strextra && ./test_utils && $(LCOV) -d . -o lcov.info && $(GENHTML) lcov.info -o ./lcov

$(OBJ_DIR_TEST):
	@printf "$(YELLOW)[CMD] $(GREEN)mkdir $(NC)-p $(BLUE)$(@)$(NC)\n"
	@mkdir -p $(@)

tests/test_main: $(OBJ_TEST_MAIN)
	@printf "$(YELLOW)[LNK] $(GREEN)$(OBJ_DIR_TEST)/*.o $(NC)-> $(BLUE)$(@)$(NC)\n"
	@$(CC_TEST) $(CFLAGS) -o $(@) $(OBJ_TEST_MAIN) $(LIBS)

tests/test_strextra: $(OBJ_TEST_STREXTRA)
	@printf "$(YELLOW)[LNK] $(GREEN)$(OBJ_DIR_TEST)/*.o $(NC)-> $(BLUE)$(@)$(NC)\n"
	@$(CC_TEST) $(CFLAGS) -o $(@) $(OBJ_TEST_STREXTRA) $(LIBS)

tests/test_utils: $(OBJ_TEST_UTILS)
	@printf "$(YELLOW)[LNK] $(GREEN)$(OBJ_DIR_TEST)/*.o $(NC)-> $(BLUE)$(@)$(NC)\n"
	@$(CC_TEST) $(CFLAGS) -o $(@) $(OBJ_TEST_UTILS) $(LIBS)

$(OBJ_DIR_TEST)/test_strextra.o: tests/source/test_strextra.cxx
	@printf "$(YELLOW)[C++] $(GREEN)$(<) $(NC)-> $(BLUE)$(@)$(NC)\n"
	@$(CC_TEST) $(CFLAGS) -c -o $(@) $(<)

$(OBJ_DIR_TEST)/test_utils.o: tests/source/test_utils.cxx
	@printf "$(YELLOW)[C++] $(GREEN)$(<) $(NC)-> $(BLUE)$(@)$(NC)\n"
	@$(CC_TEST) $(CFLAGS) -c -o $(@) $(<)

$(OBJ_DIR_TEST)/%.o: $(SRC_DIR)/%.cxx
	@printf "$(YELLOW)[C++] $(GREEN)$(<) $(NC)-> $(BLUE)$(@)$(NC)\n"
	@$(CC_TEST) $(CFLAGS) -c -o $(@) $(<)

#-------------------------------------------------------------------------------
# Code check commands
#-------------------------------------------------------------------------------

check:
	cppcheck --enable=all --std=c++23 $(CXX_FILES) --suppress=missingIncludeSystem
	@echo "Check completed!"

count:
	cloc --by-file $(SRC_DIR)

#-------------------------------------------------------------------------------
# Other commands
#-------------------------------------------------------------------------------

clean:
	rm -fr $(SOFTWARE_PATH) $(OBJ_DIR) $(OBJ_DIR_TEST) $(TARGETS_TEST)
	rm -rf tests/lcov tests/lcov.info tests/*.gcov tests/*.gcda tests/*.gcno

# vim: noexpandtab tabstop=4 shiftwidth=4
