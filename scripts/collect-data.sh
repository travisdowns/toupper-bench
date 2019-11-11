#!/bin/bash

set -e

# https://stackoverflow.com/a/12694189
SCRIPTDIR="${BASH_SOURCE%/*}"
if [[ ! -d "$DIR" ]]; then DIR="$PWD"; fi

if [[ -z "$MHZ" ]]; then
    echo "You should set MHZ to the machine MHZ to avoid constant re-calibration"
    exit 1
fi

if [[ -z "$SUFFIX" ]]; then
    echo "Set SUFFIX to the desired suffix for the data files"
    exit 1
fi

DRIVER=$SCRIPTDIR/driver.py

$DRIVER --base-env="{\"CYCLE_TIMER_FORCE_MHZ\" : \"$MHZ\"}" --xvar SIZE=100-100-10000 \
    --aggr all > results/toupper-$SUFFIX.csv

$DRIVER --base-env="{\"CYCLE_TIMER_FORCE_MHZ\" : \"$MHZ\"}" --xvar SIZE=10000-10000-500000 \
    --aggr all > results/toupper-big-$SUFFIX.csv

