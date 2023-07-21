#!/bin/bash

VERSION=$(<teddy-version)

if [[ -z $VERSION ]]; then
   echo "Error: missing version file!"
   exit 1
fi

DIRNAME="teddy-v$VERSION"

mkdir -p $DIRNAME

cp -r cmake/ $DIRNAME
cp -r doxygen/ $DIRNAME
cp -r examples/ $DIRNAME
cp -r libteddy/ $DIRNAME
cp -r libtsl/ $DIRNAME
cp -r tests/ $DIRNAME
cp -r CMakeLists.txt $DIRNAME
cp    README.md $DIRNAME
cp    LICENSE $DIRNAME
cp    teddy-version $DIRNAME

zip -r "$DIRNAME.zip" $DIRNAME
tar -czvf "$DIRNAME.tar.gz" $DIRNAME
rm -r $DIRNAME