import requests
import re

curl = requests.get('https://raw.githubusercontent.com/bagder/curl/master/include/curl/curl.h').text
multi = requests.get('https://raw.githubusercontent.com/bagder/curl/master/include/curl/multi.h').text

offsets1_re = re.compile(r'#define (CURLOPTTYPE_[A-Z_]+)\W+(\d+)')
offsets2_re = re.compile(r'#define ([A-Z_]+)\W+(CURLOPTTYPE_[A-Z_]+)')
cinit_re    = re.compile(r'CINIT\(([A-Z_]+),\W*([A-Z_]+),\W+(\d+)\)')
curlinfo_type_re = re.compile(r'#define (CURLINFO_\w+)\W+0x(\d+)')
curlinfo_re = re.compile(r'(CURLINFO_\w+)\W+=\W+(\w+)\W+(\d+)')

offsets1 = {}
offsets2 = {}
offsets_curlinfo = {}

for long_type,offset in offsets1_re.findall(curl):
    offsets1[long_type] = int(offset)

for short_type,long_type in offsets2_re.findall(curl):
    offsets2[short_type] = offsets1[long_type]
    
for type,hex_offset in curlinfo_type_re.findall(curl):
    offsets_curlinfo[type] = int(hex_offset, 16)
    
with open('enums.txt', 'wb') as f:
    f.write('enum CURLINFO {\r\n')
    for name,type,offset in curlinfo_re.findall(curl):
        f.write('    %s = %i,\r\n' % (name, offsets_curlinfo[type] + int(offset)))
    f.write('};\r\n\r\n')
    
    f.write('enum CURLOPT {\r\n')
    for name,short_type,i in cinit_re.findall(curl):
        if short_type == 'FUNCTIONPOINT':
            continue
        if short_type == 'STRINGPOINT' or short_type == 'SLISTPOINT':
            short_type = 'OBJECTPOINT'
        f.write('    CURLOPT_%s = %i,\r\n' % (name, offsets2[short_type]+int(i)))
    f.write('};\r\n\r\n')
    
    f.write('enum CURLMOPT {\r\n')
    for name,short_type,i in cinit_re.findall(multi):
        if short_type == 'FUNCTIONPOINT':
            continue
        f.write('    CURLMOPT_%s = %i,\r\n' % (name, offsets2[short_type]+int(i)))
    f.write('};\r\n\r\n')




