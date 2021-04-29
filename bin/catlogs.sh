#!/bin/bash

THISDIR=$(dirname $(readlink -e ${BASH_SOURCE[0]}))

source $THISDIR/env.sh

cat $LOGF
