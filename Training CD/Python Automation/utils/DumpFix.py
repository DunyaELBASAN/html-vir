import re
import os
import idc

parse = re.compile('^.*DS:\[([0-9A-F]+)\]\s+(.*).*$')
parse2 = re.compile('^.*CALL\s+([0-9A-F]+)\s+(.*).*$')

def process_line(line):

    res = parse.match(line)
    if not res:
        res = parse2.match(line)
        
    if res:
        address = int(res.group(1), 16)
        name = res.group(2)
        
        MakeUnkn(address, 1)
        MakeName(address, name)
        print 'Address 0x%x renamed to [%s]' % (address, name)


def main():


    filename = idc.AskFile( 0,
        '*.txt',
        'Select filename with intermodular calls dump')

    f = file(filename, 'rt')
    for line in f.readlines():
        process_line(line)
    f.close()

main()
