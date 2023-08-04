#!/bin/sh

for TARGET in "libteddy-example-1" "libteddy-example-2"; do
    echo -e "\e[32m${TARGET}\e[0m:"
    hyperfine --prepare "cd build/release && make clean" \
        "cd build/release && make ${TARGET}"
    EXEC_SIZE=$(du -k "build/release/examples/${TARGET}" | cut -f1)
    echo "Executable size: ${EXEC_SIZE} kiB"
    echo ""
done
