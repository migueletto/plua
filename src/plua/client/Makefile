CC       = m68k-palmos-gcc

CFLAGS   = -O2 -Wall -Wno-switch -I.. -I../../../include/plua -DLUA5 -palmos5

PILRC    = pilrc
BUILDPRC = build-prc

PROG     = client

all: $(PROG).prc

$(PROG).prc: $(PROG) $(PROG).def res
	$(BUILDPRC) $(PROG).def $(PROG)

res: resource.rcp
	$(PILRC) resource.rcp .
	touch res

$(PROG): $(PROG).o
	$(CC) $(CFLAGS) $(PROG).o -o $(PROG)

clean:
	rm -f *.[oasS] $(PROG) *.bin res $(PROG).prc *.grc
