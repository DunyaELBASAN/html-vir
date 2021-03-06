#include <idc.idc>

static main()
{
    AddHotkey("Ctrl-Shift-A", "hotkey_assign_color");
    AddHotkey("Ctrl-Alt-A",   "hotkey_deassign_color");
    AddHotkey("Ctrl-Shift-B", "hotkey_assign_block_color");
    AddHotkey("Ctrl-Shift-D", "hotkey_deassign_block_color");
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// core functionality
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


// convenience wrapper around assign_color_to() that will automatically resolve the 'start' and 'end' arguments with
// the start and end address of the block containing ea.
static assign_block_color_to (ea, color)
{
    auto block_start, block_end;

    block_start = find_block_start(ea);
    block_end   = find_block_end(ea);

    if (block_start == BADADDR || block_end == BADADDR)
    {
        Message("Unable to determine block start or block end.\n");
        return BADADDR;
    }

    assign_color_to(block_start, block_end, color);
}


// the core color assignment routine.
static assign_color_to (start, end, color)
{
    auto color_code, ea;

    color_code = translate_color(color);

    if (color_code == BADADDR)
        return BADADDR;

    if (start != end)
    {
        Message("Applying color code %s through range 0x%08x - 0x%08x\n", color_code, start, end);

        for (ea = start; ea < end; ea = NextNotTail(ea))
            SetColor(ea, CIC_ITEM, color_code);
    }
    else
    {
        Message("Applying color code %s at 0x%08x\n", color_code, start);
        SetColor(start, CIC_ITEM, color_code);
    }
}


// convenience wrapper around deassign_color_from() that will automatically resolve the 'start' and 'end' arguments with
// the start and end address of the block containing ea.
static deassign_block_color_from (ea)
{
    auto block_start, block_end;

    block_start = find_block_start(ea);
    block_end   = find_block_end(ea);

    if (block_start == BADADDR || block_end == BADADDR)
    {
        Message("Unable to determine block start or block end.\n");
        return BADADDR;
    }

    deassign_color_from(block_start, block_end);
}


// the core color removal routine.
static deassign_color_from (start, end)
{
    auto ea;

    if (start != end)
    {
        Message("Removing color codes through range 0x%08x - 0x%08x\n", start, end);

        for (ea = start; ea < end; ea = NextNotTail(ea))
            SetColor(ea, CIC_ITEM, DEFCOLOR);
    }
    else
    {
        Message("Removing color code at 0x%08x\n", start);
        SetColor(start, CIC_ITEM, DEFCOLOR);
    }
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// hotkey handlers
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


// hotkey handler to assigning a color to the block under ScreenEA().
static hotkey_assign_block_color ()
{
    auto color, ea;
    
    color = AskStr("blue", "color? ");
    ea    = ScreenEA();
    
    assign_block_color_to(ea, color);
    
    // refresh the screen.
    //Jump(MaxEA() - 1); Jump(ea);
}


// hotkey handler to assigning a color to current ScreenEA() or selection, if available.
static hotkey_assign_color ()
{
    auto color, ea;
    auto select_start, select_end;

    color = AskStr("blue", "color? ");
    ea    = ScreenEA();
    
    select_start = SelStart();
    select_end   = SelEnd();

    if (select_start == BADADDR)
        select_start = select_end = ea;

    assign_color_to(select_start, select_end, color);
    
    // refresh the screen.
    //Jump(MaxEA() - 1); Jump(ea);
}


// hotkey handler for removing the color code from each ea in the block under ScreenEA().
static hotkey_deassign_block_color ()
{
    auto ea;
    
    ea = ScreenEA();

    deassign_block_color_from(ea);
    
    // refresh the screen.
    //Jump(MaxEA() - 1); Jump(ea);
}


// hotkey handler for removing color code from current ScreenEA() or selection, if available.
static hotkey_deassign_color ()
{
    auto ea;
    auto select_start, select_end;

    ea = ScreenEA();
    
    select_start = SelStart();
    select_end   = SelEnd();

    if (select_start == BADADDR)
        select_start = select_end = ea;

    deassign_color_from(select_start, select_end);
    
    // refresh the screen.
    //Jump(MaxEA() - 1); Jump(ea);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// support functions
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


// returns address of start of block if found, BADADDR on error.
static find_block_start (current_ea)
{
    auto ea, prev_ea;
    auto xref_type;

    // walk up from current ea.
    for (ea = current_ea; ea != BADADDR; ea = PrevNotTail(ea))
    {
        prev_ea = PrevNotTail(ea);

        // if ea is the start of the function, we've found the start of the block.
        if (GetFunctionAttr(ea, FUNCATTR_START) == ea)
            return ea;

        // if prev_ea is the start of the function, we've found the start of the block.
        if (GetFunctionAttr(ea, FUNCATTR_START) == prev_ea)
            return prev_ea;

        // if there is a code reference *from* prev_ea or *to* ea.
        if (Rfirst0(prev_ea) != BADADDR || RfirstB0(ea) != BADADDR)
        {
            xref_type = XrefType();

            // block start found if the code reference was a JMP near or JMP far.
            if (xref_type == fl_JN || xref_type == fl_JF)
                return ea;
        }
    }

    return BADADDR;
}


// returns address of end of block if found, BADADDR on error.
static find_block_end (current_ea)
{
    auto ea, next_ea;
    auto xref_type;

    // walk down from current ea.
    for (ea = current_ea; ea != BADADDR; ea = NextNotTail(ea))
    {
        next_ea = NextNotTail(ea);

        // if next_ea is the start of the function, we've found the end of the block.
        if (GetFunctionAttr(ea, FUNCATTR_END) == next_ea)
            return next_ea;

        // if there is a code reference *from* ea or *to* next_ea.
        if (Rfirst0(ea) != BADADDR || RfirstB0(next_ea) != BADADDR)
        {
            xref_type = XrefType();

            // block end found if the code reference was a JMP near or JMP far.
            if (xref_type == fl_JN || xref_type == fl_JF)
                return next_ea;
        }
    }

    return BADADDR;
}


// returns translated color if known, BADADDR on error.
static translate_color (color)
{
    auto color_code;

    // SetColor() takes color codes in the format 0xBBGGRR.

    if (color == "red")
    {
        color_code = "0x6767BA";
    }
    else if (color == "blue")
    {
        color_code = "0xBA6767";
    }
    else if (color == "green")
    {
        color_code = "0x67BA67";
    }
    else if (color == "orange")
    {
        color_code = "0x5493E1";
    }
    else if (color == "yellow")
    {
        color_code = "0x54DFE1";
    }
    else if (strlen(color) == 7 && substr(color, 0, 1) == "#")
    {
        // reverse #RRGGBB to 0xBBGGRR.
        color_code = "0x" + substr(color, 5, 7) + substr(color, 3, 5) + substr(color, 1, 3);
    }
    else
    {
        Message("Unrecognized color. Currently supports red, green, blue, orange, yellow or custom via #RRGGBB.\n");
        return BADADDR;
    }

    return color_code;
}