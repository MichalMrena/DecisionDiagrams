#!/bin/bash

for SRC_FILE in $(find libteddy libtsl tests -type f -name "*.cpp"); do
    echo -n "... Checking $SRC_FILE "
    OUTPUT_FILE=".cppcheck-report/${SRC_FILE}.txt"
    OUTPUT=$(cppcheck $SRC_FILE --enable=all)
    mkdir -p $(dirname ${OUTPUT_FILE})
    echo -e "$OUTPUT"
    echo -e "$OUTPUT" | sed 's/\x1B\[[0-9;]\{1,\}[A-Za-z]//g' > $OUTPUT_FILE
    echo "... done ... output was written to ${OUTPUT_FILE}"
done