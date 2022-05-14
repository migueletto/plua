#!/bin/sh

B2C=bin2c.exe

#pilot-file -D ../plua/client/client.prc > /dev/null
par x ../plua/client/client.prc > /dev/null

./$B2C PluaVersion < ../plua/tver0001.bin > resources.h
./$B2C code0000 < code0000.bin >> resources.h
./$B2C code0001 < code0001.bin >> resources.h
./$B2C data0000 < data0000.bin >> resources.h
./$B2C pref0000 < pref0000.bin >> resources.h
./$B2C tAIB03e8 < ../plua/client/Tbmp1389.bin >> resources.h

exit 0
