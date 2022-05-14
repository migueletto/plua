# The standard names of gcc/prc-tools binaries.

CC       = m68k-palmos-gcc
BUILDPRC = build-prc

# You do not need to change anything below this point.

CFLAGS   = -O2 -Wall -Wno-switch
LDFLAGS  = -L. -e start -nostartfiles
OBJS     = plualib.o plualib-dispatch.o
LIBS     = -lnfm

all: TestLib2.prc 3dlib2.prc

TestLib2.prc: TestLib.def TestLib 
	$(BUILDPRC) -o TestLib2.prc TestLib.def TestLib 

TestLib: $(OBJS) TestLib.o
	$(CC) $(CFLAGS) $(LDFLAGS) $(OBJS) TestLib.o $(LIBS) -o TestLib

3dlib2.prc: 3dlib.def 3dlib 
	$(BUILDPRC) -o 3dlib2.prc 3dlib.def 3dlib 

3dlib: $(OBJS) 3dlib.o
	$(CC) $(CFLAGS) $(LDFLAGS) $(OBJS) 3dlib.o $(LIBS) -o 3dlib

clean:
	rm -f TestLib.o TestLib TestLib2.prc
	rm -f 3dlib.o 3dlib 3dlib2.prc
