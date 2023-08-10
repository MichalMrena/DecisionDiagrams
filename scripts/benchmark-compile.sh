#!/bin/sh

EXAMPLES="libteddy-example-1 libteddy-example-2"

echo -e "\e[32mExamples:\e[0m"
echo ""

for TARGET in $EXAMPLES; do
    echo -e "\e[33m${TARGET}\e[0m:"
    hyperfine --prepare "cd build/release && make clean" \
        "cd build/release && make ${TARGET}"
    EXEC_SIZE=$(du -k "build/release/examples/${TARGET}" | cut -f1)
    echo "Executable size: ${EXEC_SIZE} kiB"
    echo ""
done

echo -e "\e[32mOverall:\e[0m"
cloc ./libteddy

SRC_DIR="./libteddy/"
INCLUDE_PATTERN="^#include\s[<].*[>]$"
TMP_FILE="__tmp.cpp"

grep -r $SRC_DIR -e $INCLUDE_PATTERN | cut -d: -f2 | sort | uniq > $TMP_FILE
echo "#include <libteddy/reliability.hpp>" >> $TMP_FILE
echo "int main(){}" >> $TMP_FILE

POST_PRE_LINE_COUNT=$(g++ -I. -E examples/example_2.cpp | wc -l)

echo ""
echo -e "\e[33mLine count post-processed:\e[0m ${POST_PRE_LINE_COUNT}"
echo ""

rm $TMP_FILE