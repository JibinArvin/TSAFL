#!/usr/bin/env python3
# -*- coding: UTF-8 -*-

from cmath import log
import sys
import getopt
import os
import time
import datetime
import numpy as np
import logging

# delete all mutex about message but only take message need to instruction
def update_mutex_message(tmp_list) -> list:
    flag = 0
    offset = -1
    result = []
    for x in range(len(tmp_list)):
        if tmp_list[x][1] == "lock":
            offset += 1
            flag = 1
            continue
        if tmp_list[x][1] == "unlock":
            flag = 0
            offset = -1
            continue
        if flag == 1 and offset < 1:
            result.append(tmp_list[x])
            offset += 1
            continue
        if flag == 1 and offset >= 1:
            continue
        result.append(tmp_list[x])
    return result


def delte_same(input_list) -> list:
    result = []
    for x in range(len(input_list) - 1):
        if input_list[x] != input_list[x + 1]:
            result.append(input_list[x])
    return result


def update_memGroupList(mem_list) -> set:
    result = set()
    for x in mem_list.keys():
        mem_list[x]
    return result


def main(argv):
    soFile = ""
    memGroupFile = ""
    try:
        opts, args = getopt.getopt(argv, "hi:m:", ["help", "soFile=", "memGroupFile"])

    except getopt.GetoptError:
        print("Error: updateMempairSo.py -i <soFile> -m <memGroupFile>")
        sys.exit(2)

    for opt, arg in opts:
        if opt in ("-h", "--help"):
            print("updateMempairSo.py -i <soFile> -m <memGroupFile>")
            sys.exit()
        elif opt in ("-i", "--soFile"):
            soFile = arg
        elif opt in ("-m", "--memgroupFile"):
            memGroupFile = arg

    if soFile == "" or memGroupFile == "":
        print("Error: Command line is empty")
        print("Tips: Using -h to view help")
        sys.exit(2)

    # start
    soPath = os.path.abspath(soFile)
    soDIR = os.path.dirname(soPath)

    # load memgroup from npy file
    loadedList = np.load(memGroupFile, allow_pickle=True)
    memGroupList = loadedList.tolist()

    # load so file
    soList = [[]]
    # use flag to control if it is a new function
    flag = 0
    with open(soFile, "r") as f:
        for line in f.readlines():
            if line[0:7] == "[DEBUG]":
                pass
            elif line[0:2] == "Fu":
                flag = 1
            else:
                if flag == 1:
                    soList.append([])
                    flag = 0
                list = line.split()
                soList[-1].append([list[0], list[1], list[2], list[3]])
    soList = sorted(soList)

    final_output = []
    for x in range(len(soList)):
        if len(soList[x]) == 0:
            continue
        final_output.append(soList[x])

    for x in range(len(final_output)):
        final_output[x] = delte_same(final_output[x])
        final_output[x] = update_mutex_message(final_output[x])
    print(final_output)


if __name__ == "__main__":
    starttime = datetime.datetime.now()
    main(sys.argv[1:])
    endtime = datetime.datetime.now()
