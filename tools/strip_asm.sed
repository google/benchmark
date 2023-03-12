# strip addresses
s/^\s*[0-9a-f]+:(.*)$/\1/                      

# strip labels
s/^\s*[0-9a-f]+\s*<([0-9a-zA-Z_]+)>:\s*$/\1:/  

# strip comments
s/^(.*)#.*$/\1/                                

# register match
s/%rip/RIP/g
s/%[a-z]+[0-9]*/REG/g

# value match
s/\$0x([0-9a-f]+)/\1/g

# offset match
s/-?0x[0-9a-f]+/OFFSET/g

# size suffixes from commands
s/\b([a-z]+)[bwlq]\b/\1/g

# delete junk
/file format/d                                 
/Disassembly of section/d
/_GLOBAL_/d
/^\s*$/d

