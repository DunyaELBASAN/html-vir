static doit (void)
{
    auto address, byte;
    
    for (address = 0x652; address < 0x87C; address++)
    {
        byte = Byte(address);
        byte = byte ^ 0x17;
        
        PatchByte(address, byte);
    }
}