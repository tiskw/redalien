################################################################################
# Makefile
################################################################################

.PHONY: build-amd64 build-arm64 release-amd64 release-arm64 test-amd64 test-arm64 redalien plugins carapace-bin release test debug install check count clean concat help

#-------------------------------------------------------------------------------
# Compile settings
#-------------------------------------------------------------------------------

# Software name.
REDALIEN_PATH := build/redalien
REDALIEN_TEST := tests/redalien_test

# Version number (extracted from source/cxx/main_redalien.hxx).
VERSION := $(shell grep 'VERSION = "' source/cxx/main_redalien.hxx | grep -o '[0-9]\+\.[0-9]\+\.[0-9]\+')

# Object directory for building RedAlien.
SRC_DIR := source/cxx
OBJ_DIR := build/objects

# Object directory for tests.
SRC_DIR_TEST := tests/source
OBJ_DIR_TEST := tests/objects

# Source files for building RedAlien.
CXX_FILES := $(wildcard $(SRC_DIR)/*.cxx)
OBJ_FILES := $(patsubst $(SRC_DIR)/%,$(OBJ_DIR)/%,$(CXX_FILES:.cxx=.o))

# Source files for tests.
CXX_FILES_TEST := $(filter-out $(SRC_DIR_TEST)/dummy.cxx, $(wildcard $(SRC_DIR_TEST)/*.cxx))
OBJ_FILES_TEST := $(patsubst $(SRC_DIR)/%,$(OBJ_DIR_TEST)/%,$(CXX_FILES:.cxx=.o)) $(patsubst $(SRC_DIR_TEST)/%,$(OBJ_DIR_TEST)/%,$(CXX_FILES_TEST:.cxx=.o))

# Compile command.
CC      := g++ -O3 -flto=auto
CC_TEST := g++ -O0 -g -fPIC -coverage -fprofile-update=atomic -DGCOV_BUILD -I$(SRC_DIR)
CFLAGS  := -std=c++23 -Wall -Wextra -I/usr/local/include
LIBS    := -L/usr/local/lib

# LCOV command.
LCOV    := lcov --quiet --gcov-tool gcov --ignore-errors gcov,gcov,inconsistent,range,corrupt -c
GENHTML := genhtml --quiet --ignore-errors gcov,gcov,inconsistent,range,corrupt

# CPPCHECK command.
CPPCHECK := cppcheck --std=c++23 --enable=all --library=posix --suppress=missingIncludeSystem --suppress=useStlAlgorithm --check-level=exhaustive

# Commands for Docker-based static build.
DOCKER_IMAGE := tiskw/redalien:alpine3.23
DOCKER_BASE  := run --rm -it -u `id -u`:`id -g` -v `pwd`:/work -w /work
DOCKER_AMD64 := $(DOCKER_BASE) --platform linux/amd64 $(DOCKER_IMAGE)_amd64
DOCKER_ARM64 := $(DOCKER_BASE) --platform linux/arm64 $(DOCKER_IMAGE)_arm64

# Detect the architecture of the host machine.
ARCH := $(shell sh utils/get_arch.sh)

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
	@echo "    build-amd64      Build RedAlien for AMD64 architecture"
	@echo "    build-arm64      Build RedAlien for ARM64 architecture"
	@echo "    release-amd64    Create a release package for AMD64 architecture"
	@echo "    release-arm64    Create a release package for ARM64 architecture"
	@echo "    test-amd64       Test RedAlien on AMD64 architecture"
	@echo "    test-arm64       Test RedAlien on ARM64 architecture"
	@echo ""
	@echo "Code check commands:"
	@echo "    check            Check the code quality"
	@echo "    count            Count the lines of code"
	@echo ""
	@echo "Other commands:"
	@echo "    clean            Cleanup cache files"
	@echo "    help             Show this message"

#-------------------------------------------------------------------------------
# Build commands (outside Docker container)
#-------------------------------------------------------------------------------

build-amd64:
	@docker $(DOCKER_AMD64) sh -c 'make redalien plugins'

build-arm64:
	@docker $(DOCKER_ARM64) sh -c 'make redalien plugins'

release-amd64:
	@docker $(DOCKER_AMD64) sh -c 'make release'

release-arm64:
	@docker $(DOCKER_ARM64) sh -c 'make release'

test-amd64:
	@docker $(DOCKER_AMD64) sh -c 'make test'

test-arm64:
	@docker $(DOCKER_ARM64) sh -c 'make test'

#-------------------------------------------------------------------------------
# Build commands (inside Docker container)
#-------------------------------------------------------------------------------

redalien: $(OBJ_DIR)
	@printf "$(GRAY)------------------------------------------------------------$(NC)\n"
	@printf "$(GRAY)Building RedAlien$(NC)\n"
	@printf "$(GRAY)------------------------------------------------------------$(NC)\n"
	make -j4 $(REDALIEN_PATH)
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

carapace-bin:
	cd downloads && make carapace-bin-$(ARCH).tar.gz
	mkdir -p release/redalien/bin
	tar -xzf downloads/carapace-bin-$(ARCH).tar.gz -C release/redalien/bin
	rm -f release/redalien/bin/LICENSE release/redalien/bin/README.md

release:
	mkdir -p release/redalien/bin
	mkdir -p release/redalien/default
	mkdir -p release/redalien/plugins
	make redalien plugins carapace-bin
	cp build/redalien release/redalien/bin
	cp source/bash/redalien-integration.bash release/redalien/bin
	cp default/* release/redalien/default
	cp plugins/build/* release/redalien/plugins
	cp README.md THIRD_PARTY_LICENSES release/redalien
	cd release && tar cfz redalien_$(shell uname -m).tar.gz redalien
	rm -rf release/redalien

#-------------------------------------------------------------------------------
# Test commands (inside Docker container)
#-------------------------------------------------------------------------------

test: $(OBJ_DIR_TEST)
	@printf "$(GRAY)------------------------------------------------------------$(NC)\n"
	@printf "$(GRAY)Testing RedAlien$(NC)\n"
	@printf "$(GRAY)------------------------------------------------------------$(NC)\n"
	make -j4 $(REDALIEN_TEST)
	cd tests && ./$(notdir $(REDALIEN_TEST)) && $(LCOV) -d . -o lcov.info && $(GENHTML) lcov.info -o ./lcov
	@printf "$(GRAY)------------------------------------------------------------$(NC)\n\n"
	cd plugins && make test

$(OBJ_DIR_TEST):
	@printf "$(YELLOW)[CMD] $(GREEN)mkdir $(NC)-p $(BLUE)$(OBJ_DIR_TEST)$(NC)\n"
	@mkdir -p $(OBJ_DIR_TEST)

$(REDALIEN_TEST): $(OBJ_FILES_TEST)
	@printf "$(YELLOW)[LNK] $(GREEN)$(OBJ_DIR_TEST)/*.o $(NC)-> $(BLUE)$(REDALIEN_TEST)$(NC)\n"
	@$(CC_TEST) $(CFLAGS) -o $(REDALIEN_TEST) $(OBJ_FILES_TEST) $(LIBS) -static
	@printf "$(YELLOW)[CMD] $(GREEN)strip $(BLUE)$(REDALIEN_TEST)$(NC)\n"

$(OBJ_DIR_TEST)/main.o: $(SRC_DIR_TEST)/dummy.cxx
	@printf "$(YELLOW)[C++] $(NC)$(<) $(NC)-> $(BLUE)$(@)$(NC)\n"
	@$(CC_TEST) $(CFLAGS) -c $(<) -o $(@)

$(OBJ_DIR_TEST)/test_main.o: $(SRC_DIR_TEST)/test_main.cxx
	@printf "$(YELLOW)[C++] $(GREEN)$(<) $(NC)-> $(BLUE)$(@)$(NC)\n"
	@$(CC_TEST) $(CFLAGS) -c $(<) -o $(@)

$(OBJ_DIR_TEST)/%.o: $(SRC_DIR)/%.cxx $(SRC_DIR)/%.hxx
	@printf "$(YELLOW)[C++] $(GREEN)$(<) $(NC)-> $(BLUE)$(@)$(NC)\n"
	@$(CC_TEST) $(CFLAGS) -c $(<) -o $(@)

#-------------------------------------------------------------------------------
# Code check commands
#-------------------------------------------------------------------------------

check:
	$(CPPCHECK) -I$(SRC_DIR) $(CXX_FILES)

count:
	cloc --by-file source/cxx
	cloc --by-file source/bash

#-------------------------------------------------------------------------------
# Other commands
#-------------------------------------------------------------------------------

clean:
	rm -rf build release
	rm -rf $(REDALIEN_TEST) $(OBJ_DIR_TEST) tests/lcov tests/lcov.info tests/*.gcov tests/*.gcda tests/*.gcno
	rm -f redalien_source_concatenated.txt
	cd plugins && make clean

concat:
	python3 utils/concatenate_source_files.py > redalien_source_concatenated.txt

find_extra_whitespaces:
	grep -rnE '[[:blank:]]+$$' source tests plugins

install:
	rm -rf ~/.local/share/redalien
	cat release/redalien_$(shell uname -m).tar.gz | tar xz -C ~/.local/share

# vim: noexpandtab tabstop=4 shiftwidth=4
