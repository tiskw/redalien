# Dockerfile

FROM alpine:3.23

ARG TARGETARCH

# Install the header files.
COPY cxxopts.hpp /usr/local/include/cxxopts.hpp
COPY json.hpp    /usr/local/include/json.hpp
COPY toml.hpp    /usr/local/include/toml.hpp

# Install the carapace binary.
RUN apk update \
 && apk add --no-cache build-base gzip lcov ncurses-dev ncurses-static git bash bash-completion shared-mime-info \
 && rm -rf /var/cache/apk/*

