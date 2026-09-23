################################################################################
# Makefile - Test for C/C++ code
################################################################################

.PHONY: help test test_inside_docker clean

#-------------------------------------------------------------------------------
# Constants
#-------------------------------------------------------------------------------

# C/C++ source directory.
ORG_DIR := ../source/cxx
SRC_DIR := ./source
OBJ_DIR := ./objects

# C/C++ source files (excluding header files and main.cxx).
CXX_FILES := $(shell find $(SRC_DIR) -type f | grep -v hxx | grep -v main.cxx | sort)
OBJ_FILES := $(patsubst $(SRC_DIR)/%,$(OBJ_DIR)/%,$(CXX_FILES:.cxx=.o))

#-------------------------------------------------------------------------------
# C++ build settings
#-------------------------------------------------------------------------------

# Compile command.
CC      := g++ -std=c++23
CFLAGS  := -O0 -g -Wall -Wextra -I/usr/local/include -I$(SRC_DIR)
LIBS    := -L/usr/local/lib -lncurses
COVOPTS := -fPIC -coverage -fprofile-update=atomic -DGCOV_BUILD

# Commands for Docker-based static build.
DOCKER_IMAGE := tiskw/redalien:alpine3.23
DOCKER_BASE  := run --rm -it -u `id -u`:`id -g` -v `pwd`:/work -w /work
DOCKER_CMND  := $(DOCKER_BASE) $(DOCKER_IMAGE)

#-------------------------------------------------------------------------------
# Help message
#-------------------------------------------------------------------------------

help:
	@echo "Usage:"
	@echo "    make <command>"
	@echo ""
	@echo "Test commands:"
	@echo "    test    Run tests for C/C++ files and measure code coverage"
	@echo ""
	@echo "Other commands:"
	@echo "    clean   Cleanup cache files"
	@echo "    help    Show this message"

#-------------------------------------------------------------------------------
# Test commands
#-------------------------------------------------------------------------------

test:
	make clean
	cp -r $(ORG_DIR) $(SRC_DIR)
	git init
	@docker $(DOCKER_CMND) sh -c 'make -i -j4 test_inside_docker'
	rm -rf .git

test_inside_docker:
	mkdir -p $(OBJ_DIR)
	make run_test
	mkdir -p /dev/shm/redalien
	echo "ls -l" > /tmp/bash_history
	./run_test
	lcov --quiet --gcov-tool gcov --ignore-errors gcov,gcov,inconsistent,range,corrupt -c -d . -o lcov.info
	genhtml --quiet --ignore-errors gcov,gcov,inconsistent,range,corrupt lcov.info -o ./lcov

run_test: $(OBJ_DIR)/test_main.o $(OBJ_FILES)
	$(CC) $(CFLAGS) $(COVOPTS) -o run_test $(OBJ_FILES) $(OBJ_DIR)/test_main.o

$(OBJ_DIR)/test_main.o: test_main.cxx
	$(CC) $(CFLAGS) $(COVOPTS) -c -o $(@) -I$(SRC_DIR) $(<)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cxx
	$(CC) $(CFLAGS) $(COVOPTS) -c -o $(@) -I$(SRC_DIR) $(<)

redalien: source/main.cxx $(CXX_FILES)
	$(CC) $(CFLAGS) $(COVOPTS) -o run_test -I$(SRC_DIR) source/main.cxx $(CXX_FILES)

#-------------------------------------------------------------------------------
# Other commands
#-------------------------------------------------------------------------------

clean:
	rm -f *.gcda *.gcno *.gcov lcov.info run_test
	rm -rf lcov source objects

# vim: noexpandtab shiftwidth=4 tabstop=4 fdm=marker
