#!/bin/bash

PROJECT_NUMBER=$(<teddy-version)

if [[ -z $PROJECT_NUMBER ]]; then
   echo "Error: missing version file!"
   exit 1
fi

export PROJECT_NUMBER

doxygen doxygen/Doxyfile