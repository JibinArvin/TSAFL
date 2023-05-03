#!/usr/bin/env python3
# -*- coding: UTF-8 -*-
import argparse

from xml.dom import minidom

parser = argparse.ArgumentParser(description="Get function information from files")
parser.add_argument("passfile")
parser.add_argument("sofile")
args = parser.parse_args()

passInf = args.passfile
soFile = args.sofile

final_xml = "finalXml.xml"


class Type:
    def __init__(self, type_, name_, loc_) -> None:
        self.type = type_
        self.name = name_
        self.loc = loc_

    def getType(self):
        return self.type

    def getName(self):
        return self.name

    def getLoc(self):
        return self.loc


class Function(Type):
    """Fuction"""

    def __init__(self, type_, name_, loc_=0) -> None:
        super().__init__(type_, name_, loc_)
        self.contain = list()
        self.val = list()

    def getContain(self):
        return self.contain

    def addOp(self, op):
        self.contain.append(op)

    def dump(self):
        for each in self.contain:
            print(each.loc)

    def get_contain_number(self):
        return len(self.getContain())


class Operation(Type):
    """operation"""

    """ example lock
        type-> lock..
        name-> N0N
        loc -> 'lock.c:9'
        mystring -> hash_number(1, 2...)
    """

    def __init__(self, type_, name_, loc_, mystring_) -> None:
        super().__init__(type_, name_, loc_)
        self.mstring = mystring_

    def getHashString(self):
        return self.mstring


def clean_location_str(str_loc) -> str:
    list_str_temp = str_loc.split(":")
    if len(list_str_temp) == 2:
        return str_loc
    elif len(list_str_temp) == 3:
        re_str = list_str_temp[0] + ":" + list_str_temp[1]
        return re_str


class OperationContainer:
    # just for filter of the same operation, no other use
    """str as the key only a filter for read/write action"""

    def __init__(self) -> None:
        self.Opset = set()

    def addOrNUll(self, str: str, str2=""):
        """which means pro only care the coluom of the source-location"""
        strLTemp = str.split(":")
        if not (len(strLTemp) == 3 or len(strLTemp) == 2):
            print(
                "Erro: Code44 -- wrong format for OperationContainer in get_passInfo_function.py"
            )
            exit(44)
        addStr = ""
        for x in range(0, 2):
            addStr += strLTemp[x]
        if addStr in self.Opset:
            return False
        else:
            self.Opset.add(addStr)
            return True


def isSameNode(node1, node2) -> bool:
    if (
        node1.getAttribute("tochMemLoc") == node2.getAttribute("tochMemLoc")
        and node1.getAttribute("actionType") == node2.getAttribute("actionType")
        and node1.getAttribute("sourceLoc") == node2.getAttribute("sourceLoc")
    ):
        return True
    return False


def parse_final_xml():
    """ only parse the finalXml.xml """
    functuion_dict = {}
    with open(final_xml, "r+") as fx:
        # parse()获取DOM对象
        dom = minidom.parse(fx)
        # 获取根节点
        root = dom.documentElement
        functionsXml = root.getElementsByTagName("FUNCTION")
        for eachFunction in functionsXml:
            functionName = eachFunction.getAttribute("functionName")
            real_function = Function("function", functionName)
            for each_op in eachFunction.getElementsByTagName("OPERATION"):
                tem_loc = each_op.getAttribute("sourceLoc")
                temp_name = each_op.getAttribute("tochMemLoc")
                temp_type = each_op.getAttribute("actionType")
                temp_string = each_op.getAttribute("count")
                real_op = Operation(temp_type, temp_name, tem_loc, temp_string)
                real_function.addOp(real_op)
            if real_function.get_contain_number != 0:
                functuion_dict[functionName] = real_function
    return functuion_dict


def main_function(passInf, soFile):
    Opcon = OperationContainer()
    # NOTE: we only care about the lock and unlock action for now
    sens_loc = {"lock", "unlock", "std_atomic"}
    function_set = {}
    funcName = ""
    op = -1
    count = 0
    with open(passInf, "r") as pass_inf:
        for line in pass_inf:
            if len(line.strip()) == 0:
                continue
            if "Func" in line:
                # print("pass func working")
                funcName = line.split(":")[1].split(" ")[1]
                function = Function("function", funcName)
                function_set[funcName] = function
                continue
            action = line.split()
            if len(action) == 4 and action[2] == funcName:
                # print("pass op working")
                op = Operation("operation", action[1], action[0], action[3])
                function_set[funcName].addOp(op)

    # for name in function_set.keys():
    #     function_set[name].dump()

    xmlfile = open("finalXml.xml", "w")

    domStart = minidom.Document()
    rootNode = domStart.createElement("ROOTNODE")
    domStart.appendChild(rootNode)
    with open(soFile, "r+", encoding="utf8") as fh:
        # parse()获取DOM对象
        dom = minidom.parse(fh)
        # 获取根节点
        root = dom.documentElement
        functionsXml = root.getElementsByTagName("FUNCTION")
        for eachFunction in functionsXml:
            last_op = 0
            functionNode_temp = domStart.createElement("FUNCTION")
            rootNode.appendChild(functionNode_temp)
            functionName = eachFunction.getAttribute("functionName")
            functionNode_temp.setAttribute("functionName", functionName)
            if functionName not in function_set.keys():
                continue
            for child in eachFunction.getElementsByTagName("OPERATION"):
                if last_op != 0 and isSameNode(last_op, child):
                    print("yes")
                    continue
                # filter
                last_op = child
                OpNode = domStart.createElement("OPERATION")
                functionNode_temp.appendChild(OpNode)
                OpNode.setAttribute("tochMemLoc", child.getAttribute("tochMemLoc"))
                OpNode.setAttribute("actionType", child.getAttribute("actionType"))
                source_loc = clean_location_str(child.getAttribute("sourceLoc"))
                OpNode.setAttribute("sourceLoc", source_loc)
            for op in function_set[functionName].getContain():
                if not op.getName() in sens_loc:
                    continue
                    # if Opcon.addOrNUll(op.getLoc()) == False:
                    #     continue
                OpNode = domStart.createElement("OPERATION")
                functionNode_temp.appendChild(OpNode)
                OpNode.setAttribute("tochMemLoc", "N0N")
                OpNode.setAttribute("actionType", str(op.getName()))
                OpNode.setAttribute("sourceLoc", str(op.getLoc()))
            # print(functionName)
    # add count flag
    final_functions = rootNode.getElementsByTagName("FUNCTION")
    for function in final_functions:
        ops = function.getElementsByTagName("OPERATION")
        for op in ops:
            op.setAttribute("count", str(count))
            count += 1

    domStart.writexml(xmlfile, indent=" ", addindent="\t", newl="\n", encoding="UTF-8")
    xmlfile.close()


if __name__ == "__main__":
    main_function(passInf, soFile)
