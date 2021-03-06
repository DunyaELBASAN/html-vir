import os
import pefile


def log(msg):
    os.sys.stderr.write(msg+'\n')

def fix_headers(pe):

    # Set the VirtualSize to the RawSize so we can abuse the slack space
    #
    pe.sections[0].Misc_VirtualSize = pe.sections[0].SizeOfRawData
    
    # Mark the section executable
    #
    pe.sections[0].Characteristics = (
        pe.sections[0].Characteristics | 
        pefile.SECTION_CHARACTERISTICS['IMAGE_SCN_MEM_EXECUTE'] |
        pefile.SECTION_CHARACTERISTICS['IMAGE_SCN_MEM_WRITE'])
        
    # Change the target address of the entry point
    #
    pe.OPTIONAL_HEADER.AddressOfEntryPoint = 0x4e00
    

def add_new_section(pe):

    section = pefile.SectionStructure(pe.__IMAGE_SECTION_HEADER_format__)
    section_offset = pe.sections[-1].get_file_offset() + section.sizeof()
    section.set_file_offset(section_offset)
    section.__unpack__('\0'*section.sizeof())
    pe.__structures__.append(section)

    # Change the section's properties
    #    
    section.Misc_VirtualSize = 0x1000
    section.VirtualAddress = 0x9000
    section.SizeOfRawData = 0
    section.PointerToRawData = 0
    section.Name = '.nzight'
    
    pe.FILE_HEADER.NumberOfSections += 1


def pack_file(file_to_pack):

    pe = pefile.PE(file_to_pack)
    
    log('> PE File loaded.')
    
    sect_raw_offset = pe.sections[0].PointerToRawData
    sect_raw_size = pe.sections[0].SizeOfRawData
    sect_virtual_size = pe.sections[0].Misc_VirtualSize
    
    log('> Section Raw Offset: %d' % sect_raw_offset)
    log('> Section Raw Size: %d' % sect_raw_size)
    log('> Section Virtual Size: %d' % sect_virtual_size)
    
    f = file(file_to_pack, 'rb')
    raw_data = list(f.read())
    f.close()
    
    size = min(sect_raw_size, sect_virtual_size)
    
    #for idx in range(sect_raw_offset, sect_raw_offset+size):
    for idx in range(0x1048, 0x1048+0x2d86):
        raw_data[idx] = chr(ord(raw_data[idx])^0xf)
        
    log('> File encoded')
    
    output_file = file_to_pack+'.pack'
    f = file(output_file, 'w+b')
    f.write(''.join(raw_data))
    f.close()

    pe = pefile.PE(output_file)
    fix_headers(pe)
    #add_new_section(pe)
    pe.write(output_file)
    

    log('> Encoded file written to: [%s]' % output_file)
        

if __name__ == '__main__':
    if len(os.sys.argv) > 1 and os.path.exists(os.sys.argv[1]):
        pack_file(os.sys.argv[1])
    else:
        log('> Can\'t process file.')
    