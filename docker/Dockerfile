# Dockerfile

FROM alpine:3.23

ARG TARGETARCH

# Install the header files.
COPY cxxopts.hpp /usr/local/include/cxxopts.hpp
COPY json.hpp    /usr/local/include/json.hpp
COPY toml.hpp    /usr/local/include/toml.hpp

# Install the carapace binary.
COPY carapace-bin_1.7.3_linux_${TARGETARCH}.apk /root/carapace-bin_1.7.3_linux_${TARGETARCH}.apk
RUN apk update \
 && apk add --no-cache build-base gzip lcov ncurses-dev ncurses-static git bash bash-completion shared-mime-info \
 && apk add --no-cache --allow-untrusted /root/carapace-bin_1.7.3_linux_${TARGETARCH}.apk \
 && rm -rf /var/cache/apk/*

