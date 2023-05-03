#!/usr/bin/python

from copy import deepcopy
from typing import Dict, List, Tuple, Set
from tokenize import String
import logging
from xml.dom import minidom
import argparse
import networkx as nx


def dump_print(str):
    print(str)


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
        self.contain: List[Operation] = list()
        self.val = list()

    def getContain(self):
        return self.contain

    def addOp(self, op):
        self.contain.append(op)

    def dump(self):
        for each in self.contain:
            print(each.loc)

    def get_contain_count(self):
        return len(self.contain)

    def generate_value(self):
        return len(self.contain)

    def get_first_loc(self):
        if len(self.contain) != 0:
            return self.contain[0].loc
        return -1


class Operation(Type):
    """operation"""

    def __init__(self, type_, name_, loc_, count) -> None:
        super().__init__(type_, name_, loc_)
        self.count = count

    def get_touchMem(self):
        return self.name


def parse_funtion_xml(path) -> dict:
    funtion_set = {}
    with open(path, "r+") as f_xml:
        dom = minidom.parse(f_xml)
        root = dom.documentElement
        if root.nodeName != "ROOTNODE":
            logging.error("Not file requied by the parse_funtion_xml function")
        functionsXml = root.getElementsByTagName("FUNCTION")
        if len(functionsXml) == 0:
            logging.error("There is no function node in %s" % f_xml)
            exit(1)
        for eachFunction in functionsXml:
            functionName = eachFunction.getAttribute("functionName")
            Funtion_single = Function("funtion", functionName, 0)
            for each_op in eachFunction.getElementsByTagName("OPERATION"):
                op_touchMem = each_op.getAttribute("tochMemLoc")
                op_type = each_op.getAttribute("actionType")
                op_loc = each_op.getAttribute("sourceLoc")
                op_count = each_op.getAttribute("count")
                op_input = Operation(op_type, op_touchMem, op_loc, op_count)
                Funtion_single.addOp(op_input)
            funtion_set[functionName] = Funtion_single
    return funtion_set


class Lable:
    def __init__(self, fun: List[str], loc: List[str]) -> None:
        self.function: List[str] = fun
        self.loc: List[str] = loc

    def dump(self):
        print(self.function)
        print(self.loc)

    def if_important(self) -> bool:
        return len(self.function) > 0 or len(self.loc) > 0

    def remove_nosense(
        self,
        op_set: Dict[str, Operation],
        function_set: Dict[str, Function],
        function_useless: Set[str],
    ):
        new_func: List[str] = []
        new_loc: List[str] = []
        for func in self.function:
            if func in function_useless:
                continue
            if func in function_set.keys():
                function_entry = function_set[func]
                if function_entry.get_contain_count() == 0:
                    function_useless.add(func)
                    continue
                new_func.append(func)

        for loc in self.loc:
            if loc in op_set.keys():
                new_loc.append(loc)
        self.loc = new_loc
        self.function = new_func

    def generate_value(
        self, node_dict: Dict, function_dict: Dict[str, Function]
    ) -> int:
        value: int = 0
        for func in self.function:
            value += function_dict[func].get_value()
        value += len(self.loc)
        return value

    def get_first_loc(self, function_Dict: Dict[str, Function]):
        if len(self.loc) == 0:
            assert function_Dict[self.function[0]].get_first_loc() != -1
            return function_Dict[self.function[0]].get_first_loc()
        return self.loc[0]


def build_lable(s_list: List[str]) -> Lable:
    line_list: List = []
    call_list: List = []
    for lable in s_list:
        if lable.startswith("line:"):
            line_list.append(lable[5:])
        elif lable.startswith("Call:"):
            call_list.append(lable[5:])
    ans_lable = Lable(call_list, line_list)
    # print("Inside lable--------------")
    # print("function")
    # print(ans_lable.function)
    # print("loc")
    # print(ans_lable.loc)
    return ans_lable


class Node:
    def __init__(
        self, name: str, actions: List, callFuntion: List, lable: Lable
    ) -> None:
        self.name = name  # example : Node123124
        self.action = actions
        self.in_edge: List = []
        self.out_edge: List = []
        self.is_loop: bool = False
        self._lable: Lable = lable
        self.is_usefull: bool = False

    @property
    def lable(self) -> Lable:
        return self._lable

    @lable.setter
    def set_lable(self, p_lable):
        self.lable = p_lable

    def get_type() -> str:
        return "Node"

    def get_in_number(self):
        return self.in_edge

    def get_out_number(self):
        return self.out_edge

    def get_name(self):
        return self.name

    def add_in_edge(self, edge):
        self.in_edge.append(edge)

    def add_out_edge(self, edge):
        self.out_edge.append(edge)

    def change_important(self):
        if self.lable.if_important() is True:
            self.is_usefull = True

    def generate_value(
        self, node_dict: Dict, function_dict: Dict[str, Function]
    ) -> int:
        return self.lable.generate_value(node_dict, function_dict)

    # include the loc in the call function
    def get_all_loc(self, function_dict: Dict[str, Function]):
        ans = []
        for node in self.lable.loc:
            ans.append(node)
        ans += self.get_func_op(function_dict)
        return ans

    def get_func_op(self, function_dict: Dict[str, Function]) -> List:
        ans = []
        for func in self.lable.function:
            if func in function_dict.keys():
                for op in function_dict[func].contain:
                    ans.append(op)
        return ans


class MulNode(Node):
    def __init__(self, name: str) -> None:
        super().__init__(name, [], [])
        self.action = []
        self.callFuntion = []
        self.node_map = dict()
        self.edges = []
        self._in_node = ""  # store by the name
        self.out_node = []
        self._all_in_edge = []
        self._in_edge = []
        self._out_edge = []
        self.result = []

    @property
    def in_edge(self):
        return self._in_edge

    @in_edge.setter
    def add_entry_edge(self, edge):
        if isinstance(edge, list) and len(edge) == 2:
            self._in_edge.append(edge)
        else:
            logging.error("wrong type for add")
            logging.error(edge)
            exit(2)

    @property
    def all_in_edge(self):
        return self._all_in_edge

    @all_in_edge.setter
    def add_all_in_edge(self, edge):
        if isinstance(edge, list) and len(edge) == 2:
            self._all_in_edge.append(edge)

    def get_type() -> str:
        return "Node->MulNode"

    def replace_node_map(self, node_map: Dict):
        self.node_map = node_map

    def replace_edge(self, edges: List):
        self.edges = edges

    def add_node(self, node_name: str, node: Node):
        self.node_map[node_name] = node

    def if_node_in(self, node_name: str) -> bool:
        if node_name in self.node_map.keys():
            return True
        else:
            return False

    def get_node(self, node_name) -> Node:
        if self.if_node_in(node_name):
            return self.node_map[node_name]
        else:
            raise Exception("none such node in MulNode")

    def to_no_loop(self) -> int:
        # return value:
        #    1 -> not loop
        #    0 -> fine
        entry_node = set()
        for x in self.in_edge:
            assert len(x) == 2
            entry_node.add(x[1])
        if len(entry_node) != 1:
            logging.error("more than one entry_node")
            return 1
        self.entry_node = entry_node.pop()
        new_add_all_in_edge = []
        # remove all edge back to entry_node
        for x in self.add_all_in_edge:
            if x[1] != self.entry_node:
                new_add_all_in_edge.append(x)
        self._all_in_edge = new_add_all_in_edge


class Loop_node:
    def __init__(self) -> None:
        self.entry_node = -1
        self.out_node = []
        self.all_in_edges = []  # useless
        self.nodes: List[str] = []
        self.if_loop: bool = False

    def generate_loop(
        self,
        node_list: List[str],
        entry_in_map: Dict[str, List[str]],
        entry_out_map: Dict[str, List[str]],
        func_in: str,
        func_out: Set[str],
    ):
        # before check if real loop, generate basic structure
        outside_in_node = []
        out_node = []
        node_set: Set[str] = set()
        for node in node_list:
            node_set.add(node)
        for node in node_set:
            if node in func_out:
                out_node.append(node)
                continue
            if node == func_in:
                outside_in_node.append(node)
                continue
            for node_out in entry_in_map[node]:
                if node_out not in node_set:
                    out_node.append(node)
            for node_in in entry_out_map[node]:
                if node_in not in node_set:
                    outside_in_node.append(node)
        if len(outside_in_node) != 1:
            logging.error("not good for the loop: outside_in_node number")
            logging.error(outside_in_node)
            self.if_loop = False
            return
        self.entry_node = outside_in_node[0]
        if len(entry_out_map[self.entry_node]) < 1:
            logging.error("not good for the loop: no return for the entrynode")
            self.if_loop = False
            return
        if len(out_node) < 1:
            logging.error("there is not out_node!!")
            self.if_loop = False
            return
        # check if qualityified for loop
        # check entrynode
        if self.entry_node not in node_set:
            logging.error(
                "wrong key for entry node (not in candidate).\n entry:"
                + str(self.entry_node)
                + "\n"
                + str(node_set)
            )
            raise KeyError
        # check out_node
        for node in self.out_node:
            if node not in node_set:
                logging.error(
                    "wrong key for out node (not in candidate).\n out:"
                    + str(node)
                    + "\n node set"
                    + +str(node_set)
                )
                raise KeyError
        self.out_node = out_node
        self.if_loop = True
        self.nodes = node_list


class GraphContext:
    """include loopnode record and possible path"""

    def __init__(self) -> None:
        self._loop_node: List[Loop_node] = []
        self._path: List[List[str]] = []

    @property
    def loop_node(self):
        return self._loop_node

    @loop_node.setter
    def add_loop_node(self, node: Node):
        self.loop_node.append(node)

    @property
    def path(self):
        return self._path

    def add_path(self, path: List[str]):
        self.path.append(path)

    def clean_path(self, node_dict: Dict[str, Node]):
        # get clean path, which don't include useless node and same path
        node_useful = set()
        for node_name in node_dict.keys():
            if node_dict[node_name].is_usefull is True:
                node_useful.add(deepcopy(node_name))
        # print("node_set------------------------------")
        # print(node_useful)
        new_paths = []
        for path in self.path:
            new_path = []
            for node in path:
                if node in node_useful:
                    new_path.append(deepcopy(node))
            # ignore only include single node
            if len(new_path) > 1:
                new_paths.append(deepcopy(new_path))
        self._path = deepcopy(new_paths)
        # print(new_paths)

    def generate_path_out(
        self,
        node_dict: Dict[str, Node],
        function_dict: Dict[str, Function],
        operation_dict: Dict[str, Operation],
        kk_mem,
    ):
        # get key points of the paths
        # print('context paths', self.path)
        for path in self.path:
            end = len(path) - 1
            if path[0] not in kk_mem.keys():
                kk_mem[path[0]] = dict()
            if path[end] not in kk_mem[path[0]].keys():
                kk_mem[path[0]][path[end]] = set()
            list_p = path[1:end]
            # print("len(list_p)")
            # print(len(list_p))
            for node_tmp in list_p:
                kk_mem[path[0]][path[end]].add(node_tmp)
        # value kk_mem
        kk_mem_value: Dict[str, List] = dict()
        # for key_p in kk_mem.keys():
        #     kk_mem_value[key_p] = []
        #     for item in kk_mem[key_p]:
        #         end_node_name = item[0]
        #         value = 0
        #         for node in item[1]:
        #             value += node_dict[node].generate_value(
        #                 node_dict, function_dict)
        #         kk_mem_value[key_p].append((end_node_name, value, item[1]))

        # kk_mem_old.update(kk_mem)
        # kk_mem_value_old.update(kk_mem_value)
        # print("kk_mm", kk_mem)


""" 
    Only be called once time in kk_mem_to_xml.
    Used to generate xmlElements which is list containing key action in path.
"""


def generate_key_number_onPath(
    ele: minidom.Element,
    node_list_path: set[str],
    node_dict: Dict[str, Node],
    function_Dict: Dict[str, Function],
    operation_dict: Dict[str, Operation],
    doc: minidom.Document(),
):
    for node in node_list_path:
        count = operation_dict[node_dict[node].lable.get_first_loc(function_Dict)].count
        node_count: minidom.Element = doc.createElement("Node_count")
        node_count.setAttribute("count", count)
        ele.appendChild(node_count)


def kk_mem_to_xml(
    path_name: str,
    operation_dict: Dict[str, Operation],
    node_dict: Dict[str, Node],
    function_Dict: Dict[str, Function],
    kk_mem: Dict[str, Dict],
):
    doc = minidom.Document()
    root: minidom.Element = doc.createElement("ROOTNODE")
    doc.appendChild(root)
    for key_p in kk_mem.keys():
        start: minidom.Element = doc.createElement("Start")
        root.appendChild(start)
        # count here means name or number of the operation!
        if (
            node_dict[key_p].lable.get_first_loc(function_Dict)
            not in operation_dict.keys()
        ):
            continue
        node_count = operation_dict[
            node_dict[key_p].lable.get_first_loc(function_Dict)
        ].count
        start.setAttribute("Node_count", node_count)
        for end_node in kk_mem[key_p].keys():

            if (
                node_dict[end_node].lable.get_first_loc(function_Dict)
                not in operation_dict.keys()
            ):
                logging.error("not as excption")
                continue

            new_count = operation_dict[
                node_dict[end_node].lable.get_first_loc(function_Dict)
            ].count
            new_e: minidom.Element = doc.createElement("End")
            new_e.setAttribute("Node_count", str(new_count))
            new_e.setAttribute("Value", 0)  # waiting for change.
            start.appendChild(new_e)
            generate_key_number_onPath(
                new_e,
                kk_mem[key_p][end_node],
                node_dict,
                function_Dict,
                operation_dict,
                doc,
            )

    with open(path_name, "wb") as fp:
        fp.write(doc.toxml(encoding="utf-8"))


def add_loop_to_xml(op_location_list: Set[str], path):
    dom: minidom.Document = minidom.parse(path)
    root: minidom.Node = dom.documentElement
    for var in root.childNodes:
        if isinstance(var, minidom.Text):
            continue
        var: minidom.Element = var
        for op in var.childNodes:
            if isinstance(op, minidom.Text):
                continue
            if op.getAttribute("sourceLoc") in op_location_list:
                op.setAttribute("loop", "1")
            else:
                op.setAttribute("loop", "0")
    with open(path, "r+") as fp:
        dom.writexml(fp, encoding="UTF-8")


class Graph:
    def __init__(self, name) -> None:
        self.nodes: Dict[str, Node] = dict()
        self.edges = []
        self.entry_node: str = ""
        self.out_node: List[str] = []
        self.context: GraphContext = GraphContext()
        self.edge_in_map: Dict[str, List[str]] = None
        self.edge_out_map: Dict[str, List[str]] = None
        self.name = "Graph for"
        self.sigle = False
        self.out_node_set = set()

    def build_out_node_set(self):
        for node in self.edge_out_map.keys():
            if len(self.edge_out_map[node]) > 1:
                self.out_node_set.add(deepcopy(node))
        for node in self.out_node:
            self.out_node_set.add(deepcopy(node))

    def add_node(self, node: Node):
        self.nodes[node.get_name()] = node

    def add_edge(self, edge):
        if not isinstance(edge, tuple):
            logging.error(
                "Graph->add_edge: wrong type for add edge  "
                + str(type(edge))
                + " "
                + str(edge)
            )
            exit(1)
        if len(edge) != 2:
            logging.error("edge format is wrong")
            exit(1)
        self.edges.append(edge)
        self.nodes[edge[0]].add_out_edge(edge[0])
        self.nodes[edge[1]].add_in_edge(edge[1])

    def generate_edge_map(self):
        # build edge_in_map and edge_out_map, that edge_in_map
        # means take edge first as key
        if len((self.nodes)) <= 1:
            self.sigle = True
            return
        self.edge_in_map = dict()
        self.edge_out_map = dict()
        for edge in self.edges:
            if edge[1] not in self.edge_in_map:
                self.edge_in_map[edge[1]] = []
            self.edge_in_map[edge[1]].append(edge[0])
            if edge[0] not in self.edge_out_map:
                self.edge_out_map[edge[0]] = []
            self.edge_out_map[edge[0]].append(edge[1])
        # check if ok?
        # print("graph out_edge!")
        # print(self.edge_out_map)
        for node in self.nodes.keys():
            if (node not in self.edge_in_map.keys()) and (
                node not in self.edge_out_map.keys()
            ):
                logging.error("can't find the key in map")
                raise KeyError
        self.build_out_node_set()

    def generate_entry_and_out(self, entry_node: str, out_node: List[str]):
        self.entry_node = deepcopy(entry_node)
        self.out_node = deepcopy(out_node)
        # print("out_node----------------------------------")
        # print(self.out_node)

    def generate_EO_from_nx_digraph(self, nx_graph: nx.digraph):
        entry_node_temp = []
        out_node_temp = []
        for node in nx_graph.nodes:
            if nx_graph.in_degree(node) == 0:
                entry_node_temp.append(node)
            if nx_graph.out_degree(node) == 0:
                out_node_temp.append(node)
        if len(entry_node_temp) != 1:
            logging.error("wrong entry node for the funtion")
            exit(0)
        self.generate_entry_and_out(entry_node_temp[0], out_node_temp)

    def dump_entry_out_nodes(self):
        print(
            "dumping entry and out nodes\n entry_node   "
            + str(self.entry_node)
            + "\n out_nodes   "
            + str(self.out_node)
        )

    def check_loop(self, node_list: List[str]):
        MulNode
        pass

    def generate_loop_node(self, nx_graph: nx.DiGraph, old_loop_nodes: Set[str]):
        # create context for one graph
        loop_nodes: List[Loop_node] = []
        mayloop = list(set())
        for x in nx.strongly_connected_components(nx_graph):
            if len(x) >= 2:
                mayloop.append(x)
        for loop in mayloop:
            loop_node = Loop_node()
            loop_node.generate_loop(
                loop,
                self.edge_in_map,
                self.edge_out_map,
                self.entry_node,
                self.out_node,
            )
            print(loop_node.if_loop)
            loop_nodes.append(loop_node)
        # print("len(loop_nodes)")
        # print(len(loop_nodes))
        for loop in loop_nodes:
            for x in loop.nodes:
                old_loop_nodes.add(x)

    # node is something like Node0x675adf0
    def get_next_nodes(self, node: str) -> List[str]:
        next_nodes_list = list()
        if self.edge_out_map == None:
            print("self.edge_out_map == None")
            exit(1)
        if node not in self.edge_out_map.keys():
            if node not in self.nodes:
                print("WOW key wrong in graph.node!")
                exit(1)
            return next_nodes_list
        for node_ in self.edge_out_map[node]:
            next_nodes_list.append(node_)
        return next_nodes_list

    # use set_path to remember all node, use path to remember the sequence of path
    # have_split to remember if the fathers of node have branch!
    # (这什么呀，只有当天的我看的懂吧！！！不知所云！！)
    # (这太多情况没有考虑了吧！如果当前节点的孩子节点大于两个，
    # 但只有一个孩子节点的路径有useful node？)
    def sub_breadth_first_search_path(
        self,
        node_useful: Set,
        path: List[str],
        have_split: bool,
        node_name: str,
        node_set: Set[str],
    ):
        if (len(path) != 1) and (node_name in self.out_node_set):
            self.context.add_path(deepcopy(path))
            # print("end node")
            # print(node_name)
            # print("add path OK!")
            # print(path)
            return
        next_nodes_list = self.get_next_nodes(node_name)
        have_split_old = have_split
        if (len(next_nodes_list)) > 1:
            have_split = True
        for node in next_nodes_list:
            if node in path:
                continue
            path.append(node)
            node_set.add(node)
            self.sub_breadth_first_search_path(
                node_useful, path, have_split, node, node_set
            )
            path = path[0:-1]
            node_set.remove(node)

    def breadth_first_search_path(
        self, graph: nx.DiGraph,
    ):
        node_useful = set()
        for node_name in self.nodes.keys():
            if self.nodes[node_name].is_usefull is True:
                node_useful.add(node_name)
        if len(graph.nodes) == 0:
            return
        for node in graph.nodes:
            if node not in node_useful:
                continue
            path = list()
            path.append(node)
            node_set = set()
            node_set.add(node)
            self.sub_breadth_first_search_path(node_useful, path, False, node, node_set)

    def generate_path(
        self,
        graph: nx.DiGraph,
        function_dict: Dict[str, Function],
        operation_dict: Dict[str, Operation],
        kk_mem,
    ):
        """ How to get the final kk_mm? Firstly, find the keys which exist in different paths.
            Secondly, find the the path between these keys. """
        if self.entry_node is None or self.out_node is None:
            logging.error("enty_node or out_node not init")
            logging.error(self.name)
            raise KeyError
        if not isinstance(self.entry_node, str):
            logging.error(
                "entry_node wrong type"
                + str(type(self.entry_node))
                + "indent:"
                + str(self.entry_node)
            )
            logging.error(self.name)
            raise KeyError
        if not isinstance(self.out_node, List):
            logging.error(
                "out_node wrong type"
                + type(self.out_node)
                + "indent:"
                + str(self.out_node)
            )
            logging.error(self.name)
            raise KeyError
        # print("looking for all simple paths.")
        self.breadth_first_search_path(graph)
        self.context.clean_path(self.nodes)
        # print("print cleaned path")
        # print(self.context.path)
        self.context.generate_path_out(
            self.nodes, function_dict, operation_dict, kk_mem
        )


class factory:
    """"factory class without real things"""

    def make_mulNode(self, node_list: List[Node]) -> MulNode:
        node_map: Dict[str, Node] = dict()
        for x in node_list:
            node_map[x.get_name()] = x
        full_name = node_list[0].get_name
        result = MulNode(full_name)
        result.replace_node_map(node_map)
        return result

    def make_node(self, node_info):
        result = Node(node_info, [], [])
        return result


def pretty_xml(
    element, indent, newline, level=0
):  # elemnt为传进来的Elment类，参数indent用于缩进，newline用于换行
    if element:  # 判断element是否有子元素
        if (element.text is None) or element.text.isspace():  # 如果element的text没有内容
            element.text = newline + indent * (level + 1)
        else:
            element.text = (
                newline
                + indent * (level + 1)
                + element.text.strip()
                + newline
                + indent * (level + 1)
            )
            # else:  # 此处两行如果把注释去掉，Element的text也会另起一行
            # element.text = newline + indent * (level + 1) +
            # element.text.strip() + newline + indent * level
    temp = list(element)  # 将element转成list
    for subelement in temp:
        if temp.index(subelement) < (
            len(temp) - 1
        ):  # 如果不是list的最后一个元素，说明下一个行是同级别元素的起始，缩进应一致
            subelement.tail = newline + indent * (level + 1)
        else:  # 如果是list的最后一个元素， 说明下一行是母元素的结束，缩进应该少一个
            subelement.tail = newline + indent * level
        pretty_xml(subelement, indent, newline, level=level + 1)  # 对子元素进行递归操作


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="config.py test")
    parser.add_argument(
        "-xml", "--xmlfile_path", type=str, required=True, help="Path to xml."
    )
    args = parser.parse_args()

    xmlpath = args.xmlfile_path
    funs = parse_funtion_xml(xmlpath)

    for key in funs.keys():
        print(key)
        funs[key].dump()
