from idaapi import *

# Dictionary containing the top parents found and their
# hit count, i.e. how many times they have been reached
# from different functions
#
top_parents = dict()

# Climb all the references to the given function
# and when a function is found with no incoming
# references, add it to the 'parents' list
#
def get_parent(func_addr, parents):

    func_obj = get_func(func_addr)
    if not func_obj:
        return
    start_ea = func_obj.startEA

    if start_ea in visited:
        return
        
    visited.add(start_ea)

    refs = CodeRefsTo(start_ea, 0) + DataRefsTo(start_ea)
    #print 'refs', map(hex, refs)
    if len(refs) <= 1:
        #print 'no refs %s' % start_ea
        parents.append(start_ea)
    
    for ref in refs:
        get_parent(ref, parents)


print 'Start_'

# Walk all the functions and for each try to find the
# topmost function(s) in the calltree, saving them in
# the dictionary top_parents
#
for seg in Segments():
  for func_ea in Functions(seg, SegEnd(seg)):
    visited = set()
    print '%08x: cheking function\'s parents [visited:%d]' % (func_ea, len(visited))
    top_parent_addresses = list()
    get_parent(func_ea, top_parent_addresses)

    for addr in top_parent_addresses:
        top_parents[addr] = top_parents.get(addr, 0)+1
        print '%08x: function has top parent at %08x' % (
            func_ea, addr)

top_parents = [(p[1], p[0]) for p in top_parents.items()]
top_parents.sort()
for count, top_parent in top_parents:
    print '%08x: Parent found %d times' % (top_parent, count)

print 'Probable original entry point at %08x' % top_parents[-1][1]

print 'End_'
