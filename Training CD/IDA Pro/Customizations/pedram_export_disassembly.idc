#include <idc.idc>

static main()
{
    AddHotkey("Ctrl-Shift-X", "export_disassembly");
}

static export_disassembly ()
{
    auto func_start, func_end;
    auto disasm;
    auto fname, fhandle;
    auto ea;
    auto export_addresses;

    // determine the file to write exported disassembly to.
    fname = AskFile(1, ".txt", "Filename to save export disassembly to?");

    if (fname == "")
    {
        Message("[!] Disassembly export cancelled by user.\n");
        return;
    }

    // open the target filename.
    fhandle = fopen(fname, "w+");

    if (fhandle == 0)
    {
        Message("[!] Unable to open '%s' for writing.\n", fname);
    }

    // determine whether or not the user wishes to include addresses.
    export_addresses = AskYN(1, "Export instruction addresses as well?");

    // determine function start/end address.
    func_start = GetFunctionAttr(ScreenEA(), FUNCATTR_START);
    func_end   = GetFunctionAttr(ScreenEA(), FUNCATTR_END);

    // export the disassembly of the current function under ScreenEA().
    for (ea = func_start; ea != func_end; ea = NextNotTail(ea))
    {
        if (export_addresses)
            fprintf(fhandle, "%08x: %s\n", ea, GetDisasm(ea));
        else
            fprintf(fhandle, "%s\n", GetDisasm(ea));
    }

    fclose(fhandle);
}