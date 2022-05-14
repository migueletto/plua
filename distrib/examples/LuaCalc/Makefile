# configure the path of plua2c
PLUAC=plua2c

# warning: this creator ID is not registered
CREATOR=lCa2

PROG=LuaCalc

all: $(PROG).prc

$(PROG).prc: $(PROG).lua
	$(PLUAC) -name $(PROG) -cid $(CREATOR) -ver 1.0 -nt -s -o $(PROG).prc $(PROG).lua

clean:
	rm -f $(PROG).prc
