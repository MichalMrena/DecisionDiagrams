#!/bin/bash

for file in $(find libteddy tests -type f -name "*.cpp" -or -name "*.hpp"); do
    clang-format -style=file --dry-run $file > /dev/null 2>&1
    if [[ $? -eq 1 ]]; then
        echo "Error for: $file"
    else
        clang-format -style=file --dry-run -Werror $file > /dev/null 2>&1
        if [[ $? -eq 1 ]]; then
            echo "Formatting $file"
            clang-format -i -style=file $file
        fi
    fi
done