import string
import idaapi

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

  
def find_func_arg(call_addr, stack_delta):

    # +1 so it also contains the call instruction
    #
    addr = call_addr+1

    # Retrieve the addresses, stack deltas and mnemonics
    #
    stack_deltas = [
        [ea, GetSpDiff(ea), GetMnem(ea)]
            for ea in Heads(addr-20, addr)]

    # Walk the stack deltas backwards and return
    # when we find a instruction that placed something
    # at the offset of interest
    #
    sum = 0
    for i in range(len(stack_deltas)-1, 0, -1):
      if sum == stack_delta:
        return stack_deltas[i][0]
      sum += stack_deltas[i][1]

    return None
    
print 'Start____'
for ref in CodeRefsTo(ScreenEA(), 0):

    # Get the second argument to the function,
    # if none found, continue with the next call
    #
    arg_addr = find_func_arg(ref, -8)
    if arg_addr is None:
        continue

    # Set IDA's to point to the instruction of interest
    #
    idaapi.ua_code(arg_addr)

    # Check if the instruction uses the SIB byte, meaning
    # that this will be an indexed access (for instance, into
    # a table of strings), we don't want to process those
    # in this way
    #
    op = idaapi.get_instruction_operand(idaapi.cvar.cmd, 0)
    if ord(op.specflag1) == 1:
        continue


    r = DataRefsFrom(arg_addr)
    if r:
        r=r[0]
        print '%x: %s' % (r, unrot(r).replace('\n', r'\n'))

print 'End____'
