# configure the path of plua2c
PLUAC=plua2c

# configure the path of pilrc
PILRC=pilrc

# warning: this creator ID is not registered
CREATOR=Aqu2

PROG=Aquario

all: $(PROG).prc

$(PROG).prc: $(PROG).lua $(PROG).rcp
	$(PILRC) $(PROG).rcp .
	$(PLUAC) -name $(PROG) -cid $(CREATOR) -ver 1.0 -nt -s -o $(PROG).prc $(PROG).lua *.bin

clean:
	rm -f $(PROG).prc *.bin
