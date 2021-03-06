import string

def get_str(ea):
    s = list()
    while True:
        b = Byte(ea)
        if not b:
            break
        s.append(b)
        ea += 1
    return s

def unrot(ea):
    dec = ''
    for b in get_str(ea):
        if chr(b) in string.uppercase: base_char = ord('A')
        elif chr(b) in string.lowercase: base_char = ord('a')
        else:
            dec += chr(b)
            continue
        dec += chr(base_char+((b-base_char)+13)%26)
    return dec


print 'Start____'
for ref in CodeRefsTo(ScreenEA(), 0):

    # Get the mnemonics for all instructions in the
    # previous 20 bytes and save the PUSHes
    #
    pushes = []
    for ea in Heads(ref-20, ref):
        if GetMnem(ea)=='push':
            pushes.append( (GetMnem(ea), ea) )
    
    # This is the same as the previous loop
    #
    #mnemonics = [ (GetMnem(ea), ea) for ea in Heads(ref-20, ref) if GetMnem(ea)=='push']

    
    r = DataRefsFrom(pushes[-2][1])
    if r:
        r=r[0]
        print '%x: %s' % (r, unrot(r).replace('\n', r'\n'))

print 'End____'
