################################################################################
# Makefile
################################################################################

.PHONY: clean

#-------------------------------------------------------------------------------
# Constants
#-------------------------------------------------------------------------------

CARAPACE_URL := https://github.com/carapace-sh/carapace-bin/releases/download
CARAPACE_VER := 1.8.0

#-------------------------------------------------------------------------------
# Commands
#-------------------------------------------------------------------------------

carapace-bin-amd64.tar.gz:
	wget $(CARAPACE_URL)/v$(CARAPACE_VER)/carapace-bin_$(CARAPACE_VER)_linux_amd64.tar.gz -O /tmp/carapace-bin-amd64.tar.gz
	echo "6883bb61fe88a8f56c7b74bc9ff68445791ee22697f705b59a063443d8ffc8ad  /tmp/carapace-bin-amd64.tar.gz" | sha256sum -c
	mv /tmp/carapace-bin-amd64.tar.gz carapace-bin-amd64.tar.gz

carapace-bin-arm64.tar.gz:
	wget $(CARAPACE_URL)/v$(CARAPACE_VER)/carapace-bin_$(CARAPACE_VER)_linux_arm64.tar.gz -O /tmp/carapace-bin-arm64.tar.gz
	echo "d501dfb17f3d1fe32d71e2a3704ef92572fc407f90fa9684d07c64f50d9d2fe4  /tmp/carapace-bin-arm64.tar.gz" | sha256sum -c
	mv /tmp/carapace-bin-arm64.tar.gz carapace-bin-arm64.tar.gz

clean:
	rm -f carapace-bin-amd64.tar.gz carapace-bin-arm64.tar.gz

# vim: noexpandtab tabstop=4 shiftwidth=4
