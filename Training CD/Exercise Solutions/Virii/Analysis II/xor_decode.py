start_ea = 0x41d01e
length = 1432
xor_key = 0x59e0f1

for i in range(length, 0, -1):
 addr = start_ea+i*4
 PatchDword(addr, Dword(addr)^xor_key)