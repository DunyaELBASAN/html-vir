/*
    pedram amini
    analysis tracking
    ida script

    when run, this script will register a hotkey that can be utilized in place
    of "enter" to follow function cross references. the script will tag the
    visited function with "x__" marking the area as one that has already been
    examined.
*/

#include <idc.idc>

// #define this to see debug output.
#undef DEBUG


////////////////////////////////////////////////////////////////////////////////
// main()
//
// called when script is first loaded.
//
// arguments: none.
// returns:   none.
//

static main ()
{
    Message("[*] Analysis Tracker\n");
    Message("[*] Prefixes visited functions with the tag 'x__'\n");
    Message("[-] Registering Hotkey Ctrl-Shift-Enter\n");
    Message("[-] Registering Hotkey Ctrl-Shift-N\n");

    AddHotkey("Ctrl-Shift-Enter", "track_follow");
    AddHotkey("Ctrl-Shift-N",     "track_name");
}


////////////////////////////////////////////////////////////////////////////////
// track_follow()
//
// a registered hotkey function.
//
// arguments: none.
// returns:   none.
//

static track_follow ()
{
    auto xref, xref_type;
    auto name, new_name;
    auto prefix;

    // get the cross reference (if any) from the current address.
    xref = Rfirst0(ScreenEA());

    // if no xref exists. return.
    if (xref == BADADDR)
        return;

    // determine the xref type.
    xref_type = XrefType();

    // if the xref is not either a far or a near call, return.
    if (xref_type != fl_CF && xref_type != fl_CN)
        return;

    // jump to the xref.
    Jump(xref);

    // extract the function name.
    name = GetFunctionName(xref);

    // ensure a name was extracted
    if (!strlen(name))
        return;

    // determine if this function has already been tagged.
    prefix = substr(name, 0, 3);

    if (prefix == "x__")
        return;

    // tag this function.
    new_name = form("x__%s", name);

    MakeName(xref, new_name);

    #ifdef DEBUG
        Message("%08x renaming from %s to %s\n", xref, name, new_name);
    #endif
}


////////////////////////////////////////////////////////////////////////////////
// track_name()
//
// a registered hotkey function.
//
// arguments: none.
// returns:   none.
//

static track_name ()
{
    auto xref, xref_type;
    auto name, new_name;
    auto prefix;

    // get the cross reference (if any) from the current address.
    xref = Rfirst0(ScreenEA());

    // if no xref exists. return.
    if (xref == BADADDR)
        return;

    // determine the xref type.
    xref_type = XrefType();

    // if the xref is not either a far or a near call, return.
    if (xref_type != fl_CF && xref_type != fl_CN)
        return;

    // extract the function name.
    name = GetFunctionName(xref);

    // ensure a name was extracted
    if (!strlen(name))
        return;

    // determine if this function has already been tagged.
    prefix = substr(name, 0, 3);

    if (prefix == "x__")
        return;

    // tag this function.
    new_name = form("x__%s", name);

    MakeName(xref, new_name);

    #ifdef DEBUG
        Message("%08x renaming from %s to %s\n", xref, name, new_name);
    #endif
}