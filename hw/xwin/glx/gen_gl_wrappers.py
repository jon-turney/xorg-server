#!/usr/bin/python3
#
# python script to generate cdecl to stdcall wrappers for GL functions
# adapted from genheaders.py
#
# Copyright (c) 2013 The Khronos Group Inc.
#
# Permission is hereby granted, free of charge, to any person obtaining a
# copy of this software and/or associated documentation files (the
# "Materials"), to deal in the Materials without restriction, including
# without limitation the rights to use, copy, modify, merge, publish,
# distribute, sublicense, and/or sell copies of the Materials, and to
# permit persons to whom the Materials are furnished to do so, subject to
# the following conditions:
#
# The above copyright notice and this permission notice shall be included
# in all copies or substantial portions of the Materials.
#
# THE MATERIALS ARE PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
# IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
# CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
# TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
# MATERIALS OR THE USE OR OTHER DEALINGS IN THE MATERIALS.

import sys, time, pdb, string, cProfile
sys.path.append("/usr/share/opengl/spec")
from reg import *

# protect - whether to use #ifndef protections
# registry <filename> - use specified XML registry instead of gl.xml
protect = True

# Default input / log files
errFilename = None
diagFilename = 'diag.txt'
regFilename = 'gl.xml'
outFilename = 'gen_gl_wrappers.c'
dispatchheader=None
prefix="gl"
preresolve=False
staticwrappers=False
nodebugcallcounting=False

#exclude base WGL API
WinGDI={key: 1 for key in [
     "wglCopyContext"
    ,"wglCreateContext"
    ,"wglCreateLayerContext"
    ,"wglDeleteContext"
    ,"wglGetCurrentContext"
    ,"wglGetCurrentDC"
    ,"wglGetProcAddress"
    ,"wglMakeCurrent"
    ,"wglShareLists"
    ,"wglUseFontBitmapsA"
    ,"wglUseFontBitmapsW"
    ,"wglUseFontBitmaps"
    ,"SwapBuffers"
    ,"wglUseFontOutlinesA"
    ,"wglUseFontOutlinesW"
    ,"wglUseFontOutlines"
    ,"wglDescribeLayerPlane"
    ,"wglSetLayerPaletteEntries"
    ,"wglGetLayerPaletteEntries"
    ,"wglRealizeLayerPalette"
    ,"wglSwapLayerBuffers"
    ,"wglSwapMultipleBuffers"
    ,"ChoosePixelFormat"
    ,"DescribePixelFormat"
    ,"GetEnhMetaFilePixelFormat"
    ,"GetPixelFormat"
    ,"SetPixelFormat"
]}

if __name__ == '__main__':
    i = 1
    while (i < len(sys.argv)):
        arg = sys.argv[i]
        i = i + 1
        if (arg == '-noprotect'):
            print('Disabling inclusion protection in output headers', file=sys.stderr)
            protect = False
        elif (arg == '-registry'):
            regFilename = sys.argv[i]
            i = i+1
            print('Using registry ', regFilename, file=sys.stderr)
        elif (arg == '-outfile'):
            outFilename = sys.argv[i]
            i = i+1
        elif (arg == '-preresolve'):
            preresolve=True
        elif (arg == '-staticwrappers'):
            staticwrappers=True
        elif (arg == '-dispatchheader'):
            dispatchheader = sys.argv[i]
            i = i+1
        elif (arg == '-prefix'):
            prefix = sys.argv[i]
            i = i+1
        elif (arg == '-nodbgcount'):
            nodebugcallcounting = True
        elif (arg[0:1] == '-'):
            print('Unrecognized argument:', arg, file=sys.stderr)
            exit(1)

print('Generating ', outFilename, file=sys.stderr)

# Load & parse registry
reg = Registry()
tree = etree.parse(regFilename)
reg.loadElementTree(tree)

# Defaults for generating re-inclusion protection wrappers (or not)
protectFile = protect
protectFeature = protect
protectProto = protect

allVersions = '.*'

genOpts = CGeneratorOptions(
        apiname           = prefix,
        profile           = 'compatibility',
        versions          = allVersions,
        emitversions      = allVersions,
        defaultExtensions = prefix,                   # Default extensions for GL
#        addExtensions     = None,
#        removeExtensions  = None,
#        prefixText        = prefixStrings + glExtPlatformStrings + glextVersionStrings,
#        genFuncPointers   = True,
#        protectFile       = protectFile,
#        protectFeature    = protectFeature,
#        protectProto      = protectProto,
#        apicall           = 'GLAPI ',
#        apientry          = 'APIENTRY ',
#        apientryp         = 'APIENTRYP '),
        )

# create error/warning & diagnostic files
if (errFilename):
    errWarn = open(errFilename,'w')
else:
    errWarn = sys.stderr
diag = open(diagFilename, 'w')

#
# look for all the SET_ macros in dispatch.h, this is the set of functions
# we need to generate
#

dispatch = {}

if dispatchheader :
    fh = open(dispatchheader)
    dispatchh = fh.readlines()

    dispatch_regex = re.compile(r'^SET_(\S*)\(')

    for line in dispatchh :
        line = line.strip()
        m1 = dispatch_regex.search(line)

        if m1 :
            dispatch[prefix+m1.group(1)] = 1

    del dispatch['glby_offset']

class PreResolveOutputGenerator(OutputGenerator):
    def __init__(self,
                 errFile = sys.stderr,
                 warnFile = sys.stderr,
                 diagFile = sys.stdout):
        OutputGenerator.__init__(self, errFile, warnFile, diagFile)
        self.wrappers={}
    def beginFile(self, genOpts):
        pass
    def endFile(self):
        self.outFile.write('\nvoid ' + prefix + 'ResolveExtensionProcs(void)\n{\n')
        for funcname in self.wrappers.keys():
            self.outFile.write( '  PRERESOLVE(PFN' + funcname.upper() + 'PROC, "' + funcname + '");\n')
        self.outFile.write('}\n\n')
    def beginFeature(self, interface, emit):
        OutputGenerator.beginFeature(self, interface, emit)
    def endFeature(self):
        OutputGenerator.endFeature(self)
    def genType(self, typeinfo, name):
        OutputGenerator.genType(self, typeinfo, name)
    def genEnum(self, enuminfo, name):
        OutputGenerator.genEnum(self, enuminfo, name)
    def genCmd(self, cmd, name):
        OutputGenerator.genCmd(self, cmd, name)

        if name in WinGDI:
            return

        self.outFile.write('RESOLVE_DECL(PFN' + name.upper() + 'PROC);\n')
        self.wrappers[name]=1

class WrapperOutputGenerator(OutputGenerator):
    def __init__(self,
                 errFile = sys.stderr,
                 warnFile = sys.stderr,
                 diagFile = sys.stdout):
        OutputGenerator.__init__(self, errFile, warnFile, diagFile)
        self.wrappers={}
    def beginFile(self, genOpts):
        pass
    def endFile(self):
        pass
    def beginFeature(self, interface, emit):
        OutputGenerator.beginFeature(self, interface, emit)
        self.OldVersion = self.featureName.startswith('GL_VERSION_1_0') or self.featureName.startswith('GL_VERSION_1_1')
    def endFeature(self):
        OutputGenerator.endFeature(self)
    def genType(self, typeinfo, name):
        OutputGenerator.genType(self, typeinfo, name)
    def genEnum(self, enuminfo, name):
        OutputGenerator.genEnum(self, enuminfo, name)
    def genCmd(self, cmd, name):
        OutputGenerator.genCmd(self, cmd, name)

        # Avoid generating wrappers which aren't referenced by the dispatch table
        if dispatchheader and not name in dispatch :
            self.outFile.write('/* No wrapper for ' + name + ', not in dispatch table */\n')
            return

        if name in WinGDI:
            return

        self.wrappers[name]=1

        proto=noneStr(cmd.elem.find('proto'))
        rettype=noneStr(proto.text)
        if rettype.lower()!="void ":
            plist = ([t for t in proto.itertext()])
            rettype = ''.join(plist[:-1])
        rettype=rettype.strip()
        if staticwrappers: self.outFile.write("static ")
        self.outFile.write("%s %sWrapper("%(rettype, name))
        params = cmd.elem.findall('param')
        plist=[]
        for param in params:
            paramlist = ([t for t in param.itertext()])
            paramtype = ''.join(paramlist[:-1])
            paramname = paramlist[-1]
            plist.append((paramtype, paramname))
        Comma=""
        if len(plist):
            for ptype, pname in plist:
                self.outFile.write("%s%s%s_"%(Comma, ptype, pname))
                Comma=", "
        else:
            self.outFile.write("void")

        self.outFile.write(")\n{\n")

        # for GL 1.0 and 1.1 functions, generate stdcall wrappers which call the function directly
        if self.OldVersion:
            if not nodebugcallcounting:
                self.outFile.write('  if (glxWinDebugSettings.enable%scallTrace) ErrorF("%s\\n");\n'%(prefix.upper(), name))
                self.outFile.write("  glWinDirectProcCalls++;\n")
                self.outFile.write("\n")

            if rettype.lower()=="void":
                self.outFile.write("  %s( "%(name))
            else:
                self.outFile.write("  return %s( "%(name))

            Comma=""
            for ptype, pname in plist:
                self.outFile.write("%s%s_"%(Comma, pname))
                Comma=", "

        # for GL 1.2+ functions, generate wrappers which use wglGetProcAddress()
        else:
            if rettype.lower()=="void":
                self.outFile.write('  RESOLVE(PFN%sPROC, "%s");\n'%(name.upper(), name))

                if not nodebugcallcounting:
                    self.outFile.write("\n")
                    self.outFile.write('  if (glxWinDebugSettings.enable%scallTrace) ErrorF("%s\\n");\n'%(prefix.upper(), name))
                    self.outFile.write("\n")

                self.outFile.write("  RESOLVED_PROC(PFN%sPROC)( """%(name.upper()))
            else:
                self.outFile.write('  RESOLVE_RET(PFN%sPROC, "%s", FALSE);\n'%(name.upper(), name))

                if not nodebugcallcounting:
                    self.outFile.write("\n")
                    self.outFile.write('  if (glxWinDebugSettings.enable%scallTrace) ErrorF("%s\\n");\n'%(prefix.upper(), name))
                    self.outFile.write("\n")

                self.outFile.write("  return RESOLVED_PROC(PFN%sPROC)("%(name.upper()))

            Comma=""
            for ptype, pname in plist:
                self.outFile.write("%s%s_"%(Comma, pname))
                Comma=", "
        self.outFile.write(" );\n}\n\n")

def genHeaders():
    outFile = open(outFilename,"w")

    outFile.write('/* Automatically generated from %s - DO NOT EDIT */\n'%regFilename)
    outFile.write('\n')

    if preresolve:
        gen = PreResolveOutputGenerator(errFile=errWarn,
                                        warnFile=errWarn,
                                        diagFile=diag)
        gen.outFile=outFile
        reg.setGenerator(gen)
        reg.apiGen(genOpts)

    gen = WrapperOutputGenerator(errFile=errWarn,
                                 warnFile=errWarn,
                                 diagFile=diag)
    gen.outFile=outFile
    reg.setGenerator(gen)
    reg.apiGen(genOpts)

    # generate function to setup the dispatch table, which sets each
    # dispatch table entry to point to it's wrapper function
    # (assuming we were able to make one)

    if dispatchheader :
        outFile.write('void glWinSetupDispatchTable(void)\n')
        outFile.write('{\n')
        outFile.write('  static struct _glapi_table *disp = NULL;\n')
        outFile.write('\n')
        outFile.write('  if (!disp)\n')
        outFile.write('    {\n')
        outFile.write('      disp = calloc(sizeof(void *), _glapi_get_dispatch_table_size());\n')
        outFile.write('      assert(disp);\n')

        for d in sorted(dispatch.keys()) :
                if d in gen.wrappers :
                        outFile.write('      SET_'+ d[len(prefix):] + '(disp, (void *)' + d + 'Wrapper);\n')
                else :
                        outFile.write('#warning No wrapper for ' + d + ' !\n')

        outFile.write('    }\n')
        outFile.write('\n')
        outFile.write('  _glapi_set_dispatch(disp);\n')
        outFile.write('}\n')

    outFile.close()

genHeaders()
