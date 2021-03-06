/*
    pedram amini
    call count prefixer
    ida script

    when run, this script will parse over the entire IDA database and collect
    statistics on the number of times each function is called. each referenced
    function is then renamed with a prefix containing it's call count.
    example:

        sub_12345678 may become cc17__sub_12345678

    where 17 represents the number of times the sub routine was called.

    note: this script can take some time to run and in the process IDA is
          essentially unusable. the status icon turns red while working and
          green when complete.
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
    auto ea;
    auto idx;
    auto xref, xref_type;
    auto functions, call_count, num_functions;
    auto new_name, num_calls;

    Message("[*] Call Count Prefixer\n");
    Message("[*] Prefixes function names with their call count\n");

    // we're about to start working.
    // change the status, make an announcement and disable dialogs.
    SetStatus(IDA_STATUS_WORK);
    Message("[-] Scanning through database ... \n");
    Batch(1);

    // create arrays for storing our function list and call count.
    CreateArray("functions");
    CreateArray("call_count");
    num_functions = 0;

    // starting with the first address of the first function,
    // process the entire database.
    for (ea = NextFunction(MinEA()); ea != BADADDR; ea = NextNotTail(ea))
    {
        // get the cross reference (if any) from the current address.
        xref = Rfirst0(ea);

        // if no xref exists. move on to the next instruction.
        if (xref == BADADDR)
            continue;

        // determine the xref type.
        xref_type = XrefType();

        // if the xref is either a far or near call.
        if (xref_type == fl_CF || xref_type == fl_CN)
        {
            // locate the xref-ed function within our array.
            idx = FindFuncByAddr(num_functions, xref);

            // if the function can not be found.
            if (idx == -1)
            {
                // add this function to our array.
                AddFunc(num_functions, xref);

                // update the index for the call to AddCall().
                idx = num_functions;

                // update the function count.
                num_functions = num_functions + 1;
            }

            #ifdef DEBUG
                Message("%08x :: call %08x\n", ea, xref);
            #endif

            // add a call to this function.
            AddCall(idx);
        }
    }

    // done scanning through database and recording calls.
    // step through our internal list and rename all appropriate functions.

    Message("[-] Renaming functions ... \n");

    functions  = GetArrayId("functions");
    call_count = GetArrayId("call_count");

    for (idx = 0; idx < num_functions; idx++)
    {
        ea        = GetArrayElement(AR_LONG, functions,  idx);
        num_calls = GetArrayElement(AR_LONG, call_count, idx);
        new_name  = form("cc%d__%s", num_calls, GetFunctionName(ea));

        // ensure a function name was extracted.
        // XXX - i don't know why a simple if (!GetFunctionName(ea)) doesn't
        //       work here.
        if (!strlen(GetFunctionName(ea)))
            continue;

        // update the function name.
        MakeName(ea, new_name);

        #ifdef DEBUG
            Message("[%d] 0x%08x %s\n", idx, ea, new_name);
        #endif
    }

    // we're done working.
    // change the status, make an announcement and enable dialogs.
    SetStatus(IDA_STATUS_READY);
    Message("[-] done. \n");
    Batch(0);
}


////////////////////////////////////////////////////////////////////////////////
// AddCall()
//
// update the call count for a given function.
//
// arguments: idx - index into our array to add the call.
// returns:   none.
//

static AddCall (idx)
{
    auto num_calls;
    auto functions;
    auto call_count;

    functions  = GetArrayId("functions");
    call_count = GetArrayId("call_count");

    num_calls = GetArrayElement(AR_LONG, call_count, idx);
    SetArrayLong(call_count, idx, num_calls + 1);

    #ifdef DEBUG
        Message("Adding call to [%d] 0x%08x = %d\n",
                idx,
                GetArrayElement(AR_LONG, functions, idx),
                num_calls + 1);
    #endif
}


////////////////////////////////////////////////////////////////////////////////
// AddFunc()
//
// record the addition of a new function. set the starting call count to 0.
//
// arguments: num_functions - current number of functions.
//            ea            - effective address of function we are adding.
// returns:   none.
//

static AddFunc (num_functions, ea)
{
    auto functions;
    auto call_count;

    functions  = GetArrayId("functions");
    call_count = GetArrayId("call_count");

    SetArrayLong(functions,  num_functions, ea);
    SetArrayLong(call_count, num_functions, 0);

    #ifdef DEBUG
        Message("Adding Function [%d] 0x%08x\n", num_functions, ea);
    #endif
}


////////////////////////////////////////////////////////////////////////////////
// FindFuncByAddr()
//
// search through our internal list and locate a given function by it's address.
//
// arguments: num_functions - current number of functions.
//            ea            - effective address of function we are looking for.
// returns:   index of function if found, -1 otherwise.
//

static FindFuncByAddr (num_functions, ea)
{
    auto idx;
    auto this_ea;
    auto functions;

    functions  = GetArrayId("functions");

    for (idx = 0; idx < num_functions; idx++)
    {
        this_ea = GetArrayElement(AR_LONG, functions, idx);

        if (this_ea == ea)
            return idx;
    }

    return -1;
}
