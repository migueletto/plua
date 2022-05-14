# configure the path of plua2c
PLUAC=plua2c

# warning: this creator ID is not registered
CREATOR=lPa2

PROG=LuaPad

all: $(PROG).prc

$(PROG).prc: $(PROG).lua
	$(PLUAC) -name $(PROG) -cid $(CREATOR) -ver 1.0 -s -o $(PROG).prc $(PROG).lua

clean:
	rm -f $(PROG).prc
