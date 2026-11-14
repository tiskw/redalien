################################################################################
# Makefile for plugins
################################################################################

.PHONY: help build clean

#-------------------------------------------------------------------------------
# Constants
#-------------------------------------------------------------------------------

sources := fulleditor omnipicker
targets := $(patsubst %,build/%,$(sources))

#-------------------------------------------------------------------------------
# Help message
#-------------------------------------------------------------------------------

help:
	@echo "Usage:"
	@echo "    make <command>"
	@echo ""
	@echo "Build commands:"
	@echo "    build    Build all plugins"
	@echo ""
	@echo "Other commands:"
	@echo "    clean    Cleanup build files"
	@echo "    help     Show this message"

#-------------------------------------------------------------------------------
# Build commands
#-------------------------------------------------------------------------------

build: $(targets)
	@echo "Build completed!"

build/%:
	mkdir -p build
	cd $(notdir $@) && make build && cp $(notdir $@) ../build/

#-------------------------------------------------------------------------------
# Test commands
#-------------------------------------------------------------------------------

test:
	$(foreach srcdir,$(sources),cd $(srcdir) && make test && cd ..;)

#-------------------------------------------------------------------------------
# Clean command
#-------------------------------------------------------------------------------

clean:
	rm -rf build
	$(foreach srcdir,$(sources),cd $(srcdir) && make clean && cd ..;)

# vim: noexpandtab tabstop=4 shiftwidth=4
