#!/bin/bash

VERSION=$(<version)

if [[ -z $VERSION ]]; then
   echo "Error: missing version file!"
   exit 1
fi

DIRNAME="teddy-$VERSION"

mkdir -p $DIRNAME
cp -r libteddy/ $DIRNAME
zip -r "$DIRNAME.zip" $DIRNAME
tar -czvf "$DIRNAME.tar.gz" $DIRNAME
rm -r $DIRNAME