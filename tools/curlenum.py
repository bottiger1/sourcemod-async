import requests
import re

curl = requests.get('https://raw.githubusercontent.com/bagder/curl/master/include/curl/curl.h').text
multi = requests.get('https://raw.githubusercontent.com/bagder/curl/master/include/curl/multi.h').text

offsets_re = re.compile(r'#define (CURLOPTTYPE_[A-Z_]+)\W+(\d+)')
alias_re = re.compile(r'#define ([A-Z_]+)\W+([A-Z_]+)')
cinit_re    = re.compile(r'CURLOPT\(([A-Z_]+),\W*([A-Z_]+),\W+(\d+)\)')
curlinfo_type_re = re.compile(r'#define (CURLINFO_\w+)\W+0x(\d+)')
curlinfo_re = re.compile(r'(CURLINFO_\w+)\W+=\W+(\w+)\W+(\d+)')

offsets = {}
aliases = {}
offsets_curlinfo = {}

for long_type,offset in offsets_re.findall(curl):
    offsets[long_type] = int(offset)

for name,replacement in alias_re.findall(curl):
    aliases[name] = replacement
    
for type,hex_offset in curlinfo_type_re.findall(curl):
    offsets_curlinfo[type] = int(hex_offset, 16)
    
with open('enums.txt', 'w') as f:
    f.write('enum CURLINFO {\r\n')
    for name,type,offset in curlinfo_re.findall(curl):
        f.write('    %s = %i,\r\n' % (name, offsets_curlinfo[type] + int(offset)))
    f.write('};\r\n\r\n')
    
    f.write('enum CURLOPT {\r\n')
    for name,short_type,i in cinit_re.findall(curl):
        short_type = aliases.get(short_type, short_type)
        if short_type == 'FUNCTIONPOINT':
            continue
        if short_type == 'STRINGPOINT' or short_type == 'SLISTPOINT':
            short_type = 'OBJECTPOINT'
        f.write('    %s = %i,\r\n' % (name, offsets[short_type]+int(i)))
    f.write('};\r\n\r\n')
    
    f.write('enum CURLMOPT {\r\n')
    for name,short_type,i in cinit_re.findall(multi):
        short_type = aliases.get(short_type, short_type)
        if short_type == 'FUNCTIONPOINT':
            continue
        f.write('    %s = %i,\r\n' % (name, offsets[short_type]+int(i)))
    f.write('};\r\n\r\n')




