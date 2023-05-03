#!/bin/bash

if [ $# -lt 2 ]; then
    echo "Error: not feed with the number of paramater for this script"
fi

BINARIES=$(readlink -e $1)
OUTDIR=$(readlink -e $2)

if [ -z "$BINARIES" ]; then
    echo "Couldn't find binaries folder ($1)."
    exit 1
fi

if ! [ -d "$BINARIES" ]; then
    echo "No directory: $BINARIES."
    exit 1
fi

if [ -z "$OUTDIR"]; then
    echo "Couldn't find out directory ($OUTDIR), use ./ to replace the formor"
    OUTDIR="./"
fi
