#!/bin/bash

THISDIR=$(dirname $(readlink -e ${BASH_SOURCE[0]}))

set -ex

cd $THISDIR/..
copy-vsts-to-bin.sh

'C:\opt\presonus\StudioOne4\Studio One.exe' \
    /c/m/projects/studio-one/Songs/000new/juce-vst3-testing/juce-vst3-testing.song &>/dev/null &

$THISDIR/followlogs.sh -c

