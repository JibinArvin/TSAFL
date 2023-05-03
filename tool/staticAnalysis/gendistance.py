#!/usr/bin/env python3
# -*- coding: UTF-8 -*-
from logging import error
from typing import Dict, List, Set
from xml.etree import ElementTree
import networkx as nx
import os
import argparse
from pathlib import Path
import config


def get_all_files(path) -> List[str]:
    all_file = []
    _path = Path(path)
    if _path.is_dir() is False:
        error("No such dir in the path: %s", path)
        exit(0)
    for f in os.listdir(path):  # listdir返回文件中所有目录
        f_name = os.path.join(path, f)
        all_file.append(f_name)
    return all_file


def parse_graph(
    graph: nx.digraph,
    funcName: str,
    operation_dict: Dict[str, config.Operation],
    function_set: Dict,
    function_useless: Set[str],
    kk_mem_value: Dict[str, List],
    kk_mem: Dict[str, List],
    node_dict: Dict[str, config.Node],
    loop_nodes: Set[str],
):
    """
    Args:
        graph: graph object from nx
    """
    # first make sure if have sensitive action
    mygraph = config.Graph(funcName)

    for node_id in list(graph.nodes):
        lables = graph._node[node_id]["label"]
        lables = lables.strip('"').strip("{}").split(";")
        lable_temp = config.build_lable(lables)
        lable_temp.remove_nosense(operation_dict, function_set, function_useless)
        node_temp = config.Node(
            node_id, lable_temp.loc, lable_temp.function, lable_temp
        )
        node_temp.change_important()
        node_dict[node_id] = node_temp
        mygraph.add_node(node_temp)

    for x in list(graph.edges(data=True)):
        mygraph.add_edge(x[:2])

    mygraph.generate_EO_from_nx_digraph(graph)
    mygraph.generate_edge_map()
    # mygraph.dump_entry_out_nodes()
    mygraph.generate_loop_node(graph, loop_nodes)
    mygraph.generate_path(graph, function_set, operation_dict, kk_mem_value, kk_mem)
    # mygraph.add_node()


def gen_distance():
    pass


def check_dot_file(file_name: str) -> bool:
    """
    Usage: make sure the name of the file is cfg.*.dot
    """
    file_name_ps = file_name.split(".")
    if file_name_ps[0] == "cfg" and file_name_ps[2] == "dot":
        return True
    else:
        return False


def get_nodes_in_function(
    functions_set: Dict[str, config.Function], funcName: str
) -> Dict[str, config.Operation]:
    # return value [Node.Source_loc, Node]
    func = functions_set[funcName]
    re_dict: Dict[str, config.Operation] = {}
    for x in func.getContain():
        re_dict[x.getLoc()] = x
    return re_dict


def check_if_have_sensitive(functions_set: set, funcName: str) -> bool:
    if funcName not in functions_set.keys():
        return False
    return True


def add_loop_to_xml(
    node_list: Set[str], path, node_dict: Dict[str, config.Node], funtion_dict
):
    op_list: Set[str] = []
    for node in node_list:
        locs_temp = node_dict[node].get_all_loc(funtion_dict)
        op_list += locs_temp
    config.add_loop_to_xml(op_list, path)


def main():
    parser = argparse.ArgumentParser(description="gendistance -d -xml")
    parser.add_argument(
        "-d", "--dot_files", type=str, required=True, help="Path to dot-files."
    )

    parser.add_argument(
        "-xml", "--xmlfile_path", type=str, required=True, help="Path to xml."
    )

    args = parser.parse_args()
    xml_path = args.xmlfile_path
    functions_dict = config.parse_funtion_xml(xml_path)
    dot_path = args.dot_files
    dot_files = get_all_files(dot_path)
    print("\nParsing %s.." % args.dot_files)
    function_useless: Set[str] = set()  # represent by the name
    kk_mem: Dict[str, List] = dict()
    kk_mem_value: Dict[str, List] = dict()
    node_dict: Dict[str, config.Node] = dict()
    operation_dict: Dict[str, config.Operation] = dict()
    loop_nodes: Set[str] = set()
    for dot_file_path in dot_files:

        path, dot_name = os.path.split(dot_file_path)
        if not (check_dot_file(dot_name)):
            error("Not dot file")
            continue
        funcName = dot_name.split(".")[1]
        operation_dict_temp = get_nodes_in_function(functions_dict, funcName)
        operation_dict.update(operation_dict_temp)
        print(funcName)
        if not check_if_have_sensitive(functions_dict, funcName):
            continue
        G = nx.DiGraph(nx.drawing.nx_pydot.read_dot(dot_file_path))
        parse_graph(
            G,
            funcName,
            operation_dict_temp,
            functions_dict,
            function_useless,
            kk_mem_value,
            kk_mem,
            node_dict,
            loop_nodes,
        )

    # print("kk_mm",kk_mem)
    config.kk_mem_to_xml(
        kk_mem_value, "distance.xml", operation_dict, node_dict, functions_dict, kk_mem
    )
    tree = ElementTree.parse("distance.xml")
    root = tree.getroot()  # 得到根元素，Element类
    config.pretty_xml(root, "\t", "\n")  # 执行美化方法
    tree.write("distance.xml")

    add_loop_to_xml(loop_nodes, "sensitive.xml", node_dict, functions_dict)


if __name__ == "__main__":
    main()
