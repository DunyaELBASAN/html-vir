import string

ignore_unnamed = False
visited = set()

def do_function(level, ea):
    global visited, global_counter
    
    f_name = GetFunctionName(ea)
    f_start = LocByName(f_name)

    flags = GetFunctionFlags(ea)
    if flags & FUNC_LIB:
        return

    if f_start in visited:
        return

    caller = '0x%x %s' % (f_start, f_name)

    for h in Heads(f_start, FindFuncEnd(f_start)):
    
        
        if GetMnem(h) == 'call':
            follow = True
                
            dest_ea = Rfirst0(h)
            if dest_ea == BADADDR:
                follow = False
                dest_ea = Dfirst(h)
                continue
            
            #if ignore_unnamed and Name(dest_ea).startswith('sub_'):
            #    continue
            
            callee = '0x%x %s' % (dest_ea, Name(dest_ea))
            print '  '*level, '%s to %s' % (caller, callee)
            global_counter = global_counter+1
            
            if follow:
                visited.add(f_start)
                do_function(level+1, dest_ea)


print '---- DEAD TRACE ----'

global_counter = 1
start_ea =  ScreenEA()
do_function(0, start_ea)

print '---- DONE ----'

