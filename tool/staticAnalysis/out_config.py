#!/usr/bin/env python
import argparse
from typing import Set
from xml.dom import minidom
import sys

parser = argparse.ArgumentParser(description="Get function information from files")

parser.add_argument("finalXmlParse")
args = parser.parse_args()

finalXml = args.finalXmlParse

if __name__ == "__main__":
    with open(finalXml, "r+", encoding="utf8") as finalXmlFile:
        fileContent = ""
        dom = minidom.parse(finalXmlFile)
        root = dom.documentElement
        functionsXml = root.getElementsByTagName("FUNCTION")
        loc_set: Set = set()
        for eachFunction in functionsXml:
            for child in eachFunction.getElementsByTagName("OPERATION"):
                source_loc = child.getAttribute("sourceLoc")
                if source_loc in loc_set:
                    print(
                        "There are same loction, have ignore this",
                        source_loc,
                        file=sys.stderr,
                    )
                    continue
                loc_set.add(source_loc)

                fileContent += (
                    str(source_loc) + "\n" + str(child.getAttribute("count")) + "\n"
                )

    print(fileContent[:-1])
