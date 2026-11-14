#!/bin/sh

case "$(uname -m)" in
    x86_64)    echo amd64 ;;
    aarch64)   echo arm64 ;;
    armv7l)    echo arm   ;;
    i386|i686) echo 386   ;;
    *)         uname -m   ;;
esac

# vim: expandtab tabstop=4 shiftwidth=4 fdm=marker
