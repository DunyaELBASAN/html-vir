#include <idc.idc>

static main()
{
    AddHotkey("Ctrl-Shift-J", "jump_to_func_top");
}

static jump_to_func_top ()
{
    auto func_start;

    // determine function start and jump to it.
    func_start = GetFunctionAttr(ScreenEA(), FUNCATTR_START);

    Jump(func_start);
}