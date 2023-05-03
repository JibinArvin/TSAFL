#!/usr/bin/env python3

import argparse
from typing import Dict, List
from xml.dom import minidom as dom


def read_config_with_number(info: Dict[str, int], info_filename):
    with open(info_filename, "r+", encoding="UTF-8") as infoFile:
        loc: str = ""
        for num, line in enumerate(infoFile):
            if num % 2 == 0:
                loc = line.strip(" ").strip("\n").strip("'")
            elif num % 2 == 1:
                number = int(line)
                if loc in info.keys():
                    print("There are same line in ", info_filename)
                    exit(1)
                info[loc] = number


def read_configtxt(res_map: Dict[str, List[str]], filename: str):
    with open(filename, "r+") as configFile:
        key = ""
        for num, line in enumerate(configFile):
            if line.startswith("[KEY]"):
                line_str = line[5:].strip(" ").strip("\n").strip("'")
                key = line_str
                if key not in res_map.keys():
                    res_map[key] = []
            elif line.startswith("[VALUE]"):
                line_str = line[7:].strip(" ").strip("\n").strip("'")
                res_map[key].append(line_str)


def create_xml(res_map: Dict[str, List[str]], filename, info: Dict[str, int]):
    with open(filename, "w+", encoding="UTF-8") as gruop_key_xml:
        domStart = dom.Document()
        rootNode: dom.Element = domStart.createElement("ROOTNODE")
        domStart.appendChild(rootNode)
        for key in res_map.keys():
            keyNode: dom.Element = domStart.createElement("KEY")
            keyNode.setAttribute("keyName", key)
            if key not in info.keys():
                print("find no key", key)
                print(info.keys())
                exit(1)
            keyNode.setAttribute("Number", str(info[key]))
            rootNode.appendChild(keyNode)
            for ele in res_map[key]:
                valueNode = domStart.createElement("VALUE")
                valueNode.setAttribute("valueName", ele)
                valueNode.setAttribute("Number", str(info[ele]))
                keyNode.appendChild(valueNode)

        domStart.writexml(
            gruop_key_xml, indent="", addindent="\t", newl="\n", encoding="UTF-8"
        )


def python_main():
    parser = argparse.ArgumentParser(description="Generate groupKeys.xml")
    parser.add_argument("configTxt")

    args = parser.parse_args()
    groupFile = args.configTxt

    res_map = dict()
    info: Dict[str, int] = dict()
    read_config_with_number(info, "config.txt")
    read_configtxt(res_map, groupFile)
    create_xml(res_map, "groupKeys.xml", info)


if __name__ == "__main__":
    python_main()
