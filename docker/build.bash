#!/bin/bash

####################################################################################################
# Download dependencies before building Docker images
####################################################################################################

if [ ! -e "cxxopts.hpp" ]; then
    wget -O cxxopts-3.3.1.zip https://github.com/jarro2783/cxxopts/archive/refs/tags/v3.3.1.zip
    unzip -j cxxopts-3.3.1.zip cxxopts-3.3.1/include/cxxopts.hpp
    rm -f cxxopts-3.3.1.zip
fi

if [ ! -e "json.hpp" ]; then
    wget https://github.com/nlohmann/json/releases/download/v3.12.0/json.hpp
fi

if [ ! -e "toml.hpp" ]; then
    wget -O tomlplusplus-3.4.0.zip https://github.com/marzer/tomlplusplus/archive/refs/tags/v3.4.0.zip
    unzip -j tomlplusplus-3.4.0.zip tomlplusplus-3.4.0/toml.hpp
    rm -f tomlplusplus-3.4.0.zip
fi

if [ ! -e "carapace-bin_1.7.3_linux_amd64.apk" ]; then
    wget https://github.com/carapace-sh/carapace-bin/releases/download/v1.7.3/carapace-bin_1.7.3_linux_amd64.apk
fi
if [ ! -e "carapace-bin_1.7.3_linux_arm64.apk" ]; then
    wget https://github.com/carapace-sh/carapace-bin/releases/download/v1.7.3/carapace-bin_1.7.3_linux_arm64.apk
fi


####################################################################################################
# Verify checksum of a file
####################################################################################################

verify_checksum()
{
    local expected_chksum="${1}"
    local target_file="${2}"

    # Check if both arguments are provided.
    if [[ -z "${target_file}" || -z "${expected_chksum}" ]]; then
        echo "Error: input arguments are missing." >&2
        exit 1
    fi

    # Check if the target file exists.
    if [[ ! -f "${target_file}" ]]; then
        echo "Error: File '${target_file}' does not exist." >&2
        exit 1
    fi

    # Verify the checksum of the target file.
    if ! echo "${expected_chksum}  ${target_file}" | sha256sum --check > /dev/null 2>&1; then
        echo "Error: Checksum verification failed for '${target_file}'." >&2
        exit 1
    fi

    echo "Checksum verification passed for '${target_file}'."
}

verify_checksum "80c35bb692f44a4cae5fc47f3a2dbe7138fa268f26340f5ab556d8fd07edbb6d" "cxxopts.hpp"
verify_checksum "aaf127c04cb31c406e5b04a63f1ae89369fccde6d8fa7cdda1ed4f32dfc5de63" "json.hpp"
verify_checksum "6b5172ad4dd6519aec67b919181fa7a38a2234131e5b2afa232dfe444819783e" "toml.hpp"
verify_checksum "5775397b0632ab6b746f226f6a2d35ccccc1896da6291fa2472dd367918fdc43" "carapace-bin_1.7.3_linux_amd64.apk"
verify_checksum "e14536b1283737d266a69937b67d3ff3440e699b5135b65a070a3026a31167d4" "carapace-bin_1.7.3_linux_arm64.apk"


####################################################################################################
# Build Docker image
####################################################################################################

docker build --platform linux/amd64 -t "tiskw/redalien:alpine3.23_amd64" .
docker build --platform linux/arm64 -t "tiskw/redalien:alpine3.23_arm64" .


# vim: expandtab tabstop=4 shiftwidth=4 fdm=marker
