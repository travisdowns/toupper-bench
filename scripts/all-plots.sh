#!/bin/bash

set -e

# https://stackoverflow.com/a/12694189
SCRIPTDIR="${BASH_SOURCE%/*}"
if [[ ! -d "$DIR" ]]; then DIR="$PWD"; fi

PLOTPY="$SCRIPTDIR/plot-csv.py"
RESULTDIR="$SCRIPTDIR/../results"

function plot {
    if [ -z "$OUTDIR" ]; then
        local OUT=()
    else
        local OUTNAME=${1%.*}.svg
        echo "INPUT: $1 OUTPUT: $OUTNAME"
        local OUT=("--out" "$OUTDIR/$OUTNAME")
    fi
    "$PLOTPY" "$RESULTDIR/$1" "${OUT[@]}" --tight --ylabel "$2" --xlabel "$3" --title "$4" --scatter --jitter 1 --markersize 3 "${@:5}"
}

plot toupper-skl.csv "Cycles per char" "Input size in chars" "Skylake-S: toupper() performance" --cols 1 3 4
plot toupper-skx.csv "Cycles per char" "Input size in chars" "Skylake-X: toupper() performance" --cols 1 3 4
plot toupper-big-skl.csv "Cycles per char" "Input size in chars" "Skylake-S: toupper() performance (inputs up to 500k elements)"
plot toupper-big-skx.csv "Cycles per char" "Input size in chars" "Skylake-X: toupper() performance (inputs up to 500k elements)"
