#!/bin/bash

THISDIR=$(dirname $(readlink -e ${BASH_SOURCE[0]}))

source $THISDIR/env.sh

clearLogs=false
[[ "-c" = $1 ]] && clearLogs=true

if $clearLogs; then
    $THISDIR/clearlogs.sh
fi

set -ex
tail -f $LOGF
