/*
    pGRAPH IDA Plug-in
    Copyright (C) 2005 Pedram Amini <pedram.amini@gmail.com>

    This program is free software; you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by the Free
    Software Foundation; either version 2 of the License, or (at your option)
    any later version.

    This program is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
    more details.

    You should have received a copy of the GNU General Public License along with
    this program; if not, write to the Free Software Foundation, Inc., 59 Temple
    Place, Suite 330, Boston, MA 02111-1307 USA
*/

#include <ida.hpp>
#include <idp.hpp>
#include <bytes.hpp>
#include <expr.hpp>
#include <frame.hpp>
#include <kernwin.hpp>
#include <loader.hpp>
#include <name.hpp>

#pragma warning (disable:4273)

#include "function_analyzer.h"

/////////////////////////////////////////////////////////////////////////////////////////
// _ida_init()
//
// IDA will call this function only once.
// If this function returns PLUGIN_SKIP, IDA will never load it again.
// If this function returns PLUGIN_OK, IDA will unload the plugin but
// remember that the plugin agreed to work with the database.
// The plugin will be loaded again if the user invokes it by
// pressing the hot key or by selecting it from the menu.
// After the second load, the plugin will stay in memory.
// If this function returns PLUGIN_KEEP, IDA will keep the plugin
// in memory.
//
// arguments: none.
// returns:   plugin status.
//

int _ida_init (void)
{
    // this plug-in only works with metapc (x86) CPU types.
    if(strcmp(inf.procName, "metapc") != 0)
    {
        msg("[!] Detected an incompatible non-metapc CPU type: %s\n", inf.procName);
        return PLUGIN_SKIP;
    }

    return PLUGIN_OK;
}


/////////////////////////////////////////////////////////////////////////////////////////
// _ida_run()
//
// the run function is called when the user activates the plugin. this is the main
// function of the plugin.
//
// arguments: arg - the input argument. it can be specified in the
//                  plugins.cfg file. the default is zero.
// returns:   none.
//

void _ida_run (int arg)
{
    function_analyzer *fa;
    int fid;

    const unsigned short MANHATTAN    = 0x0;
    const unsigned short SPLINES      = 0x1;

    const unsigned short MINBACKWARD  = 0x0;
    const unsigned short MINDEPTHSLOW = 0x1;
    const unsigned short MAXDEPTHSLOW = 0x2;

    const unsigned short YES          = 0x0;
    const unsigned short NO           = 0x1;

    const char dialog_format [] =
        "STARTITEM 0\n"
        "pGRAPH\n"
        "pGRAPH Custom Options\n\n"
        "Edge Type:\n"
            "           <manhattan:R> <splines:R>>\n\n\n"
        "Layout Algorithm:\n"
            "           <minbackward:R> <mindepthslow:R> <maxdepthslow:R>>\n\n\n\n"
        "Enable Finetuning:\n"
            "           <yes:R> <no:R>>\n\n\n\n"
        "Strip Comments:\n"
        "           <yes:R> <no:R>>\n\n\n\n"
        "Layout Downfactor: <:D:10:5::>\n"
        "X-Line Space:      <:D:10:5::>\n"
        "X-Node Space:      <:D:10:5::>\n"
        "\n\n";

    unsigned short edge_type        = SPLINES;
    unsigned short fine_tuning      = YES;
    unsigned short layout_algorithm = MINDEPTHSLOW;
    unsigned short strip_comments   = NO;
    int layout_downfactor           = 100;
    int xlspace                     = 20;
    int xspace                      = 30;

    // ensure we are within the confines of a known function.
    if ((fid = get_func_num(get_screen_ea())) == -1)
    {
        warning("Current screen effective address:\n\n"
                "          0x%08x\n\n"
                "does not lie within a known function.",
                get_screen_ea());
        return;
    }

    // present the user with the dialog box.
    if (!AskUsingForm_c(dialog_format, &edge_type,      &layout_algorithm,  &fine_tuning,
                                       &strip_comments, &layout_downfactor, &xlspace,     &xspace))
        return;

    fa = new function_analyzer(fid);

    // set the current screen ea into the function analyzer.
    fa->set_current_ea(get_screen_ea());

    switch (edge_type)
    {
        case MANHATTAN:
            fa->set_manhattan_edges(true);
            fa->set_splines(false);
            break;
        case SPLINES:
            fa->set_splines(true);
            fa->set_splines(false);
            break;
    }

    switch (layout_algorithm)
    {
        case MINBACKWARD:
            fa->set_layout_algorithm("minbackward");
            break;
        case MINDEPTHSLOW:
            fa->set_layout_algorithm("mindepthslow");
            break;
        case MAXDEPTHSLOW:
            fa->set_layout_algorithm("maxdepthslow");
            break;
    }

    switch (fine_tuning)
    {
        case YES:
            fa->set_finetuning(true);
            break;
        case NO:
            fa->set_finetuning(false);
            break;
    }

    switch (strip_comments)
    {
        case YES:
            fa->set_strip_comments(true);
            break;
        case NO:
            fa->set_strip_comments(false);
            break;
    }

    fa->run_analysis();
    fa->graph();
    delete fa;
}


/////////////////////////////////////////////////////////////////////////////////////////
// _ida_term()
//
// IDA will call this function when the user asks to exit. this function will not be
// called in the case of emergency exists. usually this callback is empty.
//
// arguments: none.
// returns:   none.
//

void _ida_term (void)
{
}


// include the data structures that describe the plugin to IDA.
#include "plugin_info.h"
