
common_prologs = (
    '8b ff 55 8b ec', 
    '56 8b 74 24 08',
    '55 8b ec 53', 
    '55 8b ec 81 ec',
    '55 8b ec 51 83', 
    '55 8b ec 51 51', 
    '55 8b ec 51 53',
    '55 8b ec 83 ec',
	'55 89 e5', # GCC/LCC's prolog, Quite common apparently
    '6a 70 68' # push 70h, push ... typical entry point
    )
    
print 'Start_'

function_starts = list()

# Iterate through the list of common function
# prologs and for each that is found, undefine
# whatever is defined there and try to make
# code out of it, saving the address
#
for prolog in common_prologs:
    address = SegStart(ScreenEA())
    end_address = SegEnd(address)

    while True:
        address = FindBinary(address, SEARCH_DOWN, prolog)
        
        if address==BADADDR or address>=end_address:
            break
            
        print '%08x: Prolog found [%s], making code' % (
            address, prolog)
        MakeUnkn(address, DOUNK_EXPAND)
        MakeCode(address)
        function_starts.append(address)
        
        address += 1

# Wait for IDA to finish its analysis
#
Wait()

# Create functions in all the locations where a function
# prolog was found
#
for address in function_starts:        
    print '%08x: Making function' % address
    MakeFunction(address, BADADDR)


agressive_prologs = ('55 8b ec', '53 55', '56 57')

# Now a more agressive round is made, shorter prologs are
# sought and only defined as functions if there are references
# from other parts of the code. This is in order to avoid
# trying to make functions out of data matching such short
# sequences
#
for prolog in agressive_prologs:

    address = SegStart(ScreenEA())
    end_address = SegEnd(address)

    while True:
        address = FindBinary(address, SEARCH_DOWN, prolog)
        if address==BADADDR or address>=end_address:
            break
        refs = len(CodeRefsTo(address, 0))+len(DataRefsTo(address))
        if refs > 0:
            print '%08x: Agressively making function (%d incoming references)' % (address, refs)
            MakeFunction(address, BADADDR)
        address += 1

# Make IDA reanalyze
#
print 'Trying to reanalyze area'
AnalyzeArea(SegStart(ScreenEA()), SegEnd(ScreenEA()))
Wait()
print 'End_'
