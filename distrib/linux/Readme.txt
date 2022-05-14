Plua2 is a port of Lua 5.0.3 (plus a small IDE) for the Palm Computing platform.
Lua is a programming language designed at TeCGraf, the Computer Graphics
Technology Group of PUC-Rio, Brazil.

More information on Lua can be found at http://www.lua.org
More information on Plua can be found at http://meusite.uai.com.br/~mmand/plua/plua.htm
There is a Plua discussion group at http://groups.yahoo.com/group/plua (registration required)

THIS SOFTWARE COMES WITH ABSOLUTELY NO WARRANTY, SO USE IT AT YOUR OWN RISK.

Plua is Copyright (C) Marcio Migueletto de Andrade.

---

Plua2 Compiler, or plua2c, is a desktop application that compiles Plua source
code into a PRC executable. This version runs on Linux X86 architecture.
It is statically linked, so there should be no problems with library dependencies.

plua2c is a command line application, which means it has no GUI and runs on a
console window. For a description of the available options, run plua2c with no
arguments:

plua2c: no input files given
usage: plua2c [options] filenames.  Available options are:
  -l       list
  -o name  output to file `name' (default is "PluaApp.prc")
  -p       parse only
  -s       strip debug information
  -v       show version information
  -lib     compiles a library instead of a full application
  -nt      application will not have a title frame
  -name    application name in launcher
  -cid     application creator ID as a four letter string
  -ver     application version string
  --       stop handling options


Options:
    -lib: compiles a library instead of a full application
     -nt: application will not have a title frame
Arguments:
    name: application name
     cid: application creator ID as a four letter string
     ver: application version string (ex.: 1.0)
file.prc: PRC output file
file.lua: Lua source code file
 res.bin: optional PilRC-compatible binary resource file (ex.: tAIB03e8.bin)

Suppose your Plua application is named "My Test Application", the source code
is stored in the file test.lua, and you have registered the creator ID "Test".
The command line to build test.prc is:

plua2c -name "My Test Application" -cid Test -ver 1.0 -o test.prc test.lua

The generated test.prc may now be installed on your Palm device and run as any
other application (provided you have either Plua or Plua runtime installed).

plua2c also supports embedding of resource files, a feature not available on
the onboard Plua application. You may create, for example, an icon bitmap for
your application. All arguments after the Lua source file are expected to be
binary resource files, and are compiled into the PRC.

plua2c is not a resource compiler, so you will need a third party application
to create the binary resource files expected by plua2c. A common choice is
PilRC, and in fact plua2c expects resource file names like the ones produced by
PilRC.

All resource file names must adhere to the form: ttttnnnn.bin, where tttt is
the four letter resource type and nnnn is the four digit reource ID in 
hexadecimal. A icon resource has type tAIB and ID 1000 (decimal). PilRC will
create a file named tAIB03e8.bin for an icon resource, since is uses hexadecimal
for IDs.

The modified command line to build test.prc with an icon resource would be:

plua2c -name "My Test Application" -cid Test -ver 1.0 -o test.prc test.lua tAIB03e8.bin
