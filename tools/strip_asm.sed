# strip addresses
s/^\s*[0-9a-f]+:(.*)$/\1/                      

# make line relative path
s/^\/.*\/(.*):([0-9]+)$/\t---- \1:\2/

# strip labels
s/^\s*[0-9a-f]+\s*<(.+)>:\s*$/\1:/  

# strip own comments
/^\s*\/\/.*:.*$/d

# strip comments
s/^(.*)#.*$/\1/                                

# value match
s/\$0x([0-9a-f]+)/\1/g

# call match
s/(call|callq|\bj[a-z]+\b)\s+[$x0-9a-f]+\s+<(.*)\+(.*)>/\1     \2 \3/

# offset match
s/(-?)0x([0-9a-f]+)\(%rip\)/STATICVAR:\1\2/g
s/(-?)0x([0-9a-f]+)\(%rsp\)/STACK:\1\2/g
s/(-?)0x([0-9a-f]+)/\1\2/g


# register match - instruction and stack pointer are special
s/%rip/IP/g
s/%rsp/SP/g
s/%[a-z]+[0-9]*/REG/g

# size suffixes from instructions
# call explicitly here so it gets shortened to 'cal'
s/\b(call)[wlq]?\b|\b([a-z]+)[wlq]\b/\1\2/

# delete junk
/file format/d             # comments          
/Disassembly of section/d  # comments
/_GLOBAL_/d                # globals
/^\s*$/d                   # empty lines
/\bnop[wlq]?\b/d            # zero data
/endbr64/d                 # gcc shenanigans

