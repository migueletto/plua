# configure the path of plua2c
PLUAC=plua2c

# configure the path of pilrc
PILRC=pilrc

# warning: these creator IDs are not registered
CREATOR1=lGu1
CREATOR2=lGu2
CREATOR3=lLif
CREATOR4=lTur
CREATOR5=lNet

PROG1=GUI
PROG2=GUI2
PROG3=Life
PROG4=Turtle
PROG5=Network

all: $(PROG1).prc $(PROG2).prc $(PROG3).prc $(PROG4).prc $(PROG5).prc

$(PROG1).prc: $(PROG1).lua $(PROG1).rcp
	$(PILRC) $(PROG1).rcp .
	$(PLUAC) -name $(PROG1) -cid $(CREATOR1) -ver 1.0 -s -o $(PROG1).prc $(PROG1).lua *.bin

$(PROG2).prc: $(PROG2).lua
	$(PLUAC) -name $(PROG2) -cid $(CREATOR2) -ver 1.0 -s -o $(PROG2).prc $(PROG2).lua

$(PROG3).prc: $(PROG3).lua
	$(PLUAC) -name $(PROG3) -cid $(CREATOR3) -ver 1.0 -s -o $(PROG3).prc $(PROG3).lua

$(PROG4).prc: $(PROG4).lua
	$(PLUAC) -name $(PROG4) -cid $(CREATOR4) -ver 1.0 -s -o $(PROG4).prc $(PROG4).lua

$(PROG5).prc: $(PROG5).lua
	$(PLUAC) -name $(PROG5) -cid $(CREATOR5) -ver 1.0 -s -o $(PROG5).prc $(PROG5).lua

clean:
	rm -f *.prc *.bin
