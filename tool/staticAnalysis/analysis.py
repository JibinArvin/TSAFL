#!/usr/bin/python

import argparse
import os
import sys

parser = argparse.ArgumentParser(description="Analyze the given bitcode using wpa")
parser.add_argument("bitcode")

args = parser.parse_args()

cmd = (
    "wpa --ind-call-limit=100000 -dump-callgraph -ander -vgep -svfg -dump-mssa -dump-race "
    + args.bitcode
)
os.system(cmd)
