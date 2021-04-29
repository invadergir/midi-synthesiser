#!/bin/bash

THISDIR=$(dirname $(readlink -e ${BASH_SOURCE[0]}))

buildDir=Builds

set -ex
cd $THISDIR/..
if [[ -d $buildDir ]]; then
    rm $buildDir -rf
fi
