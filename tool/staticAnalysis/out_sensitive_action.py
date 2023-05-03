#!/usr/bin/env python3
# -*- coding: UTF-8 -*-
import argparse
from xml.dom import minidom

parser = argparse.ArgumentParser(description="ouput sensitive action from variale")

parser.add_argument("finalXmlParse")
args = parser.parse_args()

finalXml = args.finalXmlParse


class opContainer:
    def __init__(self) -> None:
        self.op_list = []
        self.vName = 0

    def add(self, op_temp):
        self.op_list.append(op_temp)

    def setVName(self, name):
        self.vName = name

    def getList(self):
        return self.op_list


if __name__ == "__main__":
    op_set = {}
    with open(finalXml, "r+", encoding="utf8") as finalXmlFile:
        dom = minidom.parse(finalXmlFile)
        root = dom.documentElement
        functionsXml = root.getElementsByTagName("FUNCTION")
        for eachFunction in functionsXml:
            for child in eachFunction.getElementsByTagName("OPERATION"):
                touchMem = child.getAttribute("tochMemLoc")
                if touchMem == "N0N":
                    continue
                if touchMem not in op_set.keys():
                    op_set[touchMem] = opContainer()
                    op_set[touchMem].add(child)
                else:
                    op_set[touchMem].add(child)

    with open("sensitive.xml", "w") as xmlfile:
        domStart = minidom.Document()
        rootNode = domStart.createElement("ROOTNODE")
        domStart.appendChild(rootNode)
        for key in op_set.keys():
            V_node = domStart.createElement("Value")
            rootNode.appendChild(V_node)
            V_node.setAttribute("VName", key)
            for op in op_set[key].getList():
                V_node.appendChild(op)
        domStart.writexml(
            xmlfile, indent=" ", addindent=" ", newl="\n", encoding="UTF-8"
        )
