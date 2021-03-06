#!c:\\python\\python.exe

import sys
import pida

graphs = []

try:
    mod_name    = sys.argv[1]
    entry_point = int(sys.argv[2], 16)
except:
    print "USAGE: module_graphs.py <mod_name> <any function address>"
    sys.exit(1)

print "analyzing %s from entry point 0x%08x" % (mod_name, entry_point)
mod = pida.load(mod_name, progress_bar="ascii")
print

# create the main down graph from the entry point.
main_graph = mod.graph_down(entry_point)
print "%d of %d nodes in main graph" % (len(main_graph.nodes), len(mod.nodes))

# add it to the list of graphs.
graphs.append(main_graph)

# step through every function in the module.
for func_ea in mod.functions.keys():
    # if this function address exists in any known downgraphs, then continue
    found = False
    for graph in graphs:
        if func_ea in graph.nodes.keys():
            found = True
            break
    
    # this function was found in a graph, skip it.
    if found:
        continue
    
    # if the function was not found in any existing graphs, find this sub graphs root node.
    print "%08x not in any existing graphs. looking for root node..." % func_ea,
    root_node = None

    for node in mod.graph_up(func_ea).nodes.keys():
        if not mod.edges_to(node):
            root_node = node
            break
    
    # ensure a root node was found.
    assert(root_node)
    
    print "found at %08x" % root_node
    
    # add the down graph from this discovered root node to the list of known graphs.
    graphs.append(mod.graph_down(root_node))

# now render all the graphs to disk.
i     = 0
total = 0
for graph in graphs:
    i     += 1
    total += len(graph.nodes)

    print "%d nodes in graph %d" % (len(graph.nodes), i)
    g = graph.render_graph_graphviz()
    g.write_dot("module_graphs/%d.dot" % i)
    g.write_png("module_graphs/%d.png" % i)
	
    
print "total nodes: %d" % total