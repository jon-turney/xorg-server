#!/usr/bin/python
import re

r1 = re.compile(r'GLAPI\s(.*)\sAPIENTRY\s(\S*)\((.*)\);')
fh = open('/usr/include/w32api/GL/gl.h')

for line in fh.readlines():
        line = line.strip()
        if line == '/* 1.2 functions */' :
                break
        m1 = r1.search(line)
        if m1 :
                returntype = m1.group(1)
                funcname = m1.group(2)
                arglist = m1.group(3).strip()
                print 'static ' + returntype + ' ' + funcname + 'Wrapper(' + arglist + ')'
                print '{'
#                print '  ErrorF("'+ funcname + ' wrapper\\n");'
                if returntype == 'void' :
                        print '  ' +  funcname + '(',
                else :
                        print '  return ' +  funcname + '(',

                if arglist != 'void' :
                        a =  arglist.split(',')
                        b = []
                        for arg in a :
                                arg  = arg[arg.rfind(' '):]
                                if arg.find('[') > 0 :
                                        arg  = arg[:arg.find('[')]
                                if arg.find('*') > 0 :
                                        arg = arg[arg.find('*')+1:]
                                arg = arg.strip()
                                b.append(arg)
                        print ', '.join(b),

                print ');'
                print "}\n"