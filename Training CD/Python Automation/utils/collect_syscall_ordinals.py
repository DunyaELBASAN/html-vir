syscall_ordinal_code = 'b8 ? 00 00 00 ba 00 03 fe 7f'

for seg in Segments():
	for func in Functions(seg, SegEnd(seg)):
		address = FindBinary(func, SEARCH_DOWN, syscall_ordinal_code)
		if address == func:
			print "%08x: (%d, '%s')," % ( address, Dword(func+1), Name(func) )
			
      