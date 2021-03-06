
prologs = dict()

print 'Start_'
functions = list()
for seg in Segments():
  for func_ea in Functions(seg, SegEnd(seg)):
    prolog = tuple([Byte(ea) for ea in range(func_ea, func_ea+5)])
    prologs[prolog] = prologs.get(prolog, 0) + 1
    

prolog_list = [ (p[1], p[0]) for p in prologs.items() ]
prolog_list.sort()
for count, prolog in prolog_list:
    print 'Prolog[%s] found %d times' % (
        ','.join(map(hex, prolog)), count)

print 'End_'
