################################################################################
# Makefile
################################################################################

.PHONY: build-amd64 build-arm64 redalien plugins release test check count clean help

#-------------------------------------------------------------------------------
# Compile settings
#-------------------------------------------------------------------------------

# Software name.
REDALIEN_PATH := build/redalien

# Version number (extracted from source/bash/redalien_body).
VERSION := $(shell grep 'VERSION = "' source/cxx/main_redalien.hxx | grep -o '[0-9]\+\.[0-9]\+\.[0-9]\+')

# Object directory.
SRC_DIR := source/cxx
OBJ_DIR := build/objects

# Source files.
CXX_FILES := $(wildcard $(SRC_DIR)/*.cxx)
HXX_FILES := $(wildcard $(SRC_DIR)/*.hxx)
OBJ_FILES := $(patsubst $(SRC_DIR)/%,$(OBJ_DIR)/%,$(CXX_FILES:.cxx=.o))

# Compile command.
CC     := g++ -std=c++23
CFLAGS := -O3 -march=native -flto=auto -Wall -Wextra -I/usr/local/include
LIBS   := -L/usr/local/lib

# Commands for Docker-based static build.
DOCKER_IMAGE := tiskw/redalien:alpine3.23
DOCKER_BASE  := run --rm -it -u `id -u`:`id -g` -v `pwd`:/work -w /work
DOCKER_AMD64 := $(DOCKER_BASE) --platform linux/amd64 $(DOCKER_IMAGE)_amd64
DOCKER_ARM64 := $(DOCKER_BASE) --platform linux/arm64 $(DOCKER_IMAGE)_arm64

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
	@echo "Usage:"
	@echo "    make <command>"
	@echo ""
	@echo "Build commands:"
	@echo "    build-amd64     Build RedAlien (x86_64)"
	@echo "    build-arm64     Build RedAlien (aarch64)"
	@echo "    release-amd64   Create a release package (x86_64)"
	@echo "    release-arm64   Create a release package (aarch64)"
	@echo ""
	@echo "Test commands:"
	@echo "    test            Run tests and measure code coverage"
	@echo ""
	@echo "Code check commands:"
	@echo "    check           Check the code quality"
	@echo "    count           Count the lines of code"
	@echo ""
	@echo "Other commands:"
	@echo "    concat          Concatenate source file into one file"
	@echo "    clean           Cleanup cache files"
	@echo "    help            Show this message"

#-------------------------------------------------------------------------------
# Build commands (outside Docker container)
#-------------------------------------------------------------------------------

build-amd64:
	@printf "$(GRAY)------------------------------------------------------------$(NC)\n"
	@printf "$(GRAY)Activate Docker Container$(NC)\n"
	@printf "$(GRAY)------------------------------------------------------------$(NC)\n"
	@printf "$(YELLOW)[CMD] $(GREEN)docker $(MAGENTA)$(DOCKER_AMD64) sh -c 'make redalien plugins'$(NC)\n"
	@docker $(DOCKER_AMD64) sh -c 'make redalien plugins'

build-arm64:
	@printf "$(GRAY)------------------------------------------------------------$(NC)\n"
	@printf "$(GRAY)Activate Docker Container$(NC)\n"
	@printf "$(GRAY)------------------------------------------------------------$(NC)\n"
	@printf "$(YELLOW)[CMD] $(GREEN)docker $(MAGENTA)$(DOCKER_ARM64) sh -c 'make redalien plugins'$(NC)\n"
	@docker $(DOCKER_ARM64) sh -c 'make redalien plugins'

release-amd64:
	@printf "$(GRAY)------------------------------------------------------------$(NC)\n"
	@printf "$(GRAY)Activate Docker Container$(NC)\n"
	@printf "$(GRAY)------------------------------------------------------------$(NC)\n"
	@printf "$(YELLOW)[CMD] $(GREEN)docker $(MAGENTA)$(DOCKER_AMD64) sh -c 'make release'$(NC)\n"
	@docker $(DOCKER_AMD64) sh -c 'make release'

release-arm64:
	@printf "$(GRAY)------------------------------------------------------------$(NC)\n"
	@printf "$(GRAY)Activate Docker Container$(NC)\n"
	@printf "$(GRAY)------------------------------------------------------------$(NC)\n"
	@printf "$(YELLOW)[CMD] $(GREEN)docker $(MAGENTA)$(DOCKER_ARM64) sh -c 'make release'$(NC)\n"
	@docker $(DOCKER_ARM64) sh -c 'make release'

#-------------------------------------------------------------------------------
# Build commands (inside Docker container)
#-------------------------------------------------------------------------------

redalien: $(OBJ_DIR)
	@printf "$(GRAY)------------------------------------------------------------$(NC)\n"
	@printf "$(GRAY)Building RedAlien$(NC)\n"
	@printf "$(GRAY)------------------------------------------------------------$(NC)\n"
	@make -j4 $(REDALIEN_PATH)
	@printf "$(GRAY)------------------------------------------------------------$(NC)\n\n"

$(OBJ_DIR):
	@printf "$(YELLOW)[CMD] $(GREEN)mkdir $(NC)-p $(BLUE)$(OBJ_DIR)$(NC)\n"
	@mkdir -p $(OBJ_DIR)

$(REDALIEN_PATH): $(OBJ_FILES)
	@printf "$(YELLOW)[LNK] $(GREEN)$(OBJ_DIR)/*.o $(NC)-> $(BLUE)$(REDALIEN_PATH)$(NC)\n"
	@$(CC) $(CFLAGS) -o $(REDALIEN_PATH) $(OBJ_FILES) $(LIBS) -static
	@printf "$(YELLOW)[CMD] $(GREEN)strip $(BLUE)$(REDALIEN_PATH)$(NC)\n"
	@strip $(REDALIEN_PATH)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cxx $(SRC_DIR)/%.hxx
	@printf "$(YELLOW)[C++] $(GREEN)$(<) $(NC)-> $(BLUE)$(@)$(NC)\n"
	@$(CC) $(CFLAGS) -c $(<) -o $(@)

plugins:
	cd plugins && make build

release:
	make redalien plugins
	mkdir -p release/redalien/bin
	mkdir -p release/redalien/default
	mkdir -p release/redalien/plugins
	cp build/redalien release/redalien/bin
	cp source/bash/redalien-integration.bash release/redalien/bin
	cp default/* release/redalien/default
	cp plugins/build/* release/redalien/plugins
	cp README.md THIRD_PARTY_LICENSES release/redalien
	cd release && tar cfz redalien_$(shell uname -m).tar.gz redalien
	rm -rf release/redalien

#-------------------------------------------------------------------------------
# Test commands
#-------------------------------------------------------------------------------

test:
	cd tests; make test

#-------------------------------------------------------------------------------
# Code check commands
#-------------------------------------------------------------------------------

check:
	cppcheck --std=c++23 --enable=all -I$(SRC_DIR) --library=posix --suppress=missingIncludeSystem --suppress=useStlAlgorithm --check-level=exhaustive $(CXX_FILES)

count:
	cloc --by-file source/cxx
	cloc --by-file source/bash

#-------------------------------------------------------------------------------
# Other commands
#-------------------------------------------------------------------------------

clean:
	cd plugins && make clean
	cd tests && make clean
	rm -f redalien_source_concatenated.txt
	rm -rf build release

concat:
	python3 utils/concatenate_source_files.py > redalien_source_concatenated.txt

# vim: noexpandtab tabstop=4 shiftwidth=4
