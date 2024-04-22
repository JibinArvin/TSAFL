#!/usr/bin/env python

from ast import literal_eval
from ctypes import memset
from logging import root
import os
from pickle import TRUE
import string
import sys
import argparse
import re
import itertools
from typing import Dict, List, Set
from unicodedata import name
from xml.dom import minidom as dom
import numpy as np

# PREFIX = os.environ['STATIC_ANALYSIS_KERNEL_DIR']
PREFIX = ""

parser = argparse.ArgumentParser(description="Get aliased pair from the SVF result.")
parser.add_argument("mssa")
parser.add_argument("--aset", dest="aset", action="store_true")

args = parser.parse_args()

mssa = args.mssa
aset = args.aset


def remove_column(text):
    if aset:
        return text
    toks = text.split(":")
    if len(toks) < 3:
        return text + ":0"
    toks[2] = "0"
    return ":".join(toks)


def strip_start(text, prefix):
    if not text.startswith(prefix):
        return text
    ret = text[-(len(text) - len(prefix)) :]
    if ret.startswith("./"):
        return ret[2:]
    return ret


class Instruction:
    def __init__(self):
        self.load_from = set()
        self.store_to = set()
        self.source_loc = None
        self.pointer_type = None

    def is_integer(self):
        return self.pointer_type in ["i8", "i16", "i32", "i64"]

    def is_general_pointer(self):
        return self.pointer_type in ["i8*", "i16*", "i32*", "i64*"]

    def extract_type(self, line):
        typ = line[line.find("({") + 2 : line.find("})")]
        for regex in re.findall("struct\.[^\ ]*\.[0-9]+[^\ \*]*", typ):
            newregex = re.sub(r"\.[0-9]+$", "", regex)
            typ = typ.replace(regex, newregex)
        for regex in re.findall("\.[0-9]+:", typ):
            newregex = re.sub(r"\.[0-9]+:", ":", regex)
            typ = typ.replace(regex, newregex)
        self.pointer_type = typ

    def extract_source_location(self, line):
        loc = line.strip().split("[[")[1]
        if loc.find("@[") != -1:
            # It is inlined at somewhere, but I don't care where it is inlined at
            delim = "@["
        else:
            # No inlined
            delim = "]]"
        self.source_loc = loc.split(delim)[0].strip()

    def __parse_pts(line):
        line = line.strip()
        line = line[line.index("{") + 1 : len(line) - 1]
        return set(map(int, line.split()))

    def feed_line(self, line, is_write):
        pts = Instruction.__parse_pts(line)
        if is_write:
            self.store_to |= pts
        else:
            self.load_from |= pts

    def get_accessed_memory_location(self):
        return list(zip(self.store_to, [True] * len(self.store_to))) + list(
            zip(self.load_from, [False] * len(self.load_from))
        )

    def get_source_location(self):
        return remove_column(strip_start(self.source_loc, PREFIX))

    def get_pointer_type(self):
        return self.pointer_type


class MemoryLocation:
    def __init__(self, id):
        self.id = id
        self.load_insn = set()
        self.store_insn = set()

    def add_instruction(self, insn, is_write):
        source_loc = insn.get_source_location()[:-2]
        if is_write:
            self.store_insn.add(source_loc)
        else:
            self.load_insn.add(source_loc)

    def generate_result(self):
        if aset:
            return self.__generate_aliased_set()
        else:
            return self.__generate_mempair()

    def __generate_aliased_set(self):
        return (
            self.id,
            list(self.load_insn) + list(self.store_insn),
            ["R"] * len(self.load_insn) + ["W"] * len(self.store_insn),
        )

    def __generate_mempair(self):
        st_st = list(itertools.product(self.store_insn, self.store_insn))
        st_ld = list(itertools.product(self.store_insn, self.load_insn))

        return (
            self.id,
            (st_st + st_ld),
            [("W", "W")] * len(st_st) + [("W", "R")] * len(st_ld),
        )


class Function:
    def __init__(self, line: string) -> None:
        self.functionName = re.findall("FUNCTION:.*?[=]{1}", line)[0][10:-1]
        self.opContainer: List[Instruction] = list()

    def addOp(self, ins: Instruction):
        self.opContainer.append(ins)

    def getName(self):
        return self.functionName

    def getOp(self):
        return self.opContainer

    def remove_op(self, op):
        self.opContainer.remove(op)

    def dump(self):
        if len(self.opContainer) == 0:
            return
        print("FUNCTION:" + self.functionName + "\n")
        for each in self.opContainer:
            str_temp = ""
            for memloc, is_write in each.get_accessed_memory_location():
                # if memloc == 1:
                #     continue
                str_temp += "tochMemLoc : " + str(memloc) + "\n"
                if is_write:
                    str_temp += "actionType : write\n"
                else:
                    str_temp += "actionType : read\n"
            str_temp += "sourceLoc : " + each.get_source_location() + "\n"
            print(str_temp)


def functionSetTOXML(function_set: Dict, fileLocation: string):
    with open(fileLocation, "w+", encoding="UTF-8") as xmlfile:
        domStart = dom.Document()
        rootNode = domStart.createElement("ROOTNODE")
        domStart.appendChild(rootNode)
        for eachName in function_set.keys():
            functionNode = domStart.createElement("FUNCTION")
            functionNode.setAttribute("functionName", eachName)
            rootNode.appendChild(functionNode)
            for op in function_set[eachName].getOp():
                for memloc, is_write in op.get_accessed_memory_location():
                    OpNode = domStart.createElement("OPERATION")
                    functionNode.appendChild(OpNode)
                    # if memloc == 1:
                    #     continue
                    OpNode.setAttribute("tochMemLoc", str(memloc))
                    if is_write:
                        OpNode.setAttribute("actionType", "write")
                    else:
                        OpNode.setAttribute("actionType", "read")
                    OpNode.setAttribute("sourceLoc", str(op.get_source_location()))

        domStart.writexml(
            xmlfile, indent="", addindent="\t", newl="\n", encoding="UTF-8"
        )


def clean_function_set(function_set: Dict[str, Function]):
    # create dict to figure out which var is real meaningfull
    var_dict: Dict[str, Set[str]] = dict()
    for func in function_set.keys():
        for op in function_set[func].getOp():
            for memloc, is_write in op.get_accessed_memory_location():
                if memloc not in var_dict.keys():
                    var_dict[memloc] = set()
                var_dict[memloc].add(func)

    print(var_dict)

    var_set: Set[str] = set()
    for var in var_dict.keys():
        if len(var_dict[var]) != 1:
            var_set.add(var)
    var_dict.clear()
    print(var_set)
    for func in function_set.keys():
        mem: List = list()
        for op in function_set[func].getOp():
            flag: bool = False
            for memloc, is_write in op.get_accessed_memory_location():
                if memloc in var_set:
                    flag = True
                    break
            if flag == False:
                mem.append(op)

        for op in mem:
            function_set[func].remove_op(op)


if __name__ == "__main__":
    with open(mssa, "r") as mssa_file:
        memory_locations = {}
        reset = True
        function_set = {}
        function_name = ""
        insn = Instruction()
        for line in mssa_file:

            if "@[" and "]]]" in line:
                continue
            if "cpp:0]]" in line:
                continue
            if "FUNCTION" in line:
                function_tmp = Function(line)
                function_name = function_tmp.getName()
                function_set[function_name] = function_tmp
                continue
            if len(line.strip()) == 0:
                continue
            if reset:
                reset = False
                insn = Instruction()
            if "LDMU" in line:
                insn.feed_line(line, is_write=False)
            elif "STCHI" in line:
                insn.feed_line(line, is_write=True)
            elif "[[" in line:
                reset = True
                # set a source location
                insn.extract_source_location(line)
                # set a type of the pointer
                insn.extract_type(line)

                # if insn.is_integer() or insn.is_general_pointer():
                #     continue
                # exclude the function or loc not in the project.
                if "/usr" in line:
                    continue
                # print(insn.get_source_location())
                function_set[function_name].addOp(insn)
                typ = insn.get_pointer_type()
                for memloc, is_write in insn.get_accessed_memory_location():
                    if memloc == 1:
                        continue
                    key = memloc
                    if not key in memory_locations:
                        memory_locations[key] = MemoryLocation(key)
                    memory_locations[key].add_instruction(insn, is_write)
        for k in function_set.keys():
            function_set[k].dump()

    SaveList = []
    for key in memory_locations:
        SaveList.append(
            [key, memory_locations[key].store_insn, memory_locations[key].load_insn]
        )

    # load list
    # loadedList=np.load('memGroup.npy', allow_pickle=True)
    # memGroupList=loadedList.tolist()
    # print(memGroupList)

    # locSet = set()
    # for each in SaveList:
    #     for eachstore in each[1]:
    #         locSet.add(eachstore)
    #     for eachload in each[2]:
    #         locSet.add(eachload)

    # for each in sorted(locSet):
    #     if "/usr/lib/gcc/x86_64-linux-gnu" not in each and not each.endswith(":0"):
    #         print(each)
    # clean_function_set(function_set)
    functionSetTOXML(function_set, "dom_write.xml")
