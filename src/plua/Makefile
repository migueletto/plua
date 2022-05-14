LUA       = ../..
include $(LUA)/config.palmos

MULTILINK = $(HOME)/work/palm/Palm/multilink-linux

PO        = /opt/palmdev/sdk-5/include/PalmOne
INC       = -I$(LUA)/include -I$(LUA)/include/plua -I. -I.. -I$(PO)

DEFS      = -DMULTILINK -DPALMOS -DLUA5

CFLAGS    = -O2 -Wall -Wno-switch $(INC) $(DEFS) -palmos5
LDFLAGS   = -L.

LIBS      = $(LIB)/liblua.a $(LIB)/liblualib.a

PILRC     = pilrc
BUILDPRC  = build-prc

PROG      = plua2

OBJS      = main.o app.o p.o text.o error.o db.o doc.o rsrc.o \
	    memolist.o doclist.o streamlist.o vfslist.o file.o list.o \
            luac.o prefs.o link.o compile.o serial.o scroll.o \
            help.o vsprintf.o hr.o bmp.o fill.o sound.o midi.o editor.o \
            sprite.o gcontrol.o handler.o MathLib.o check.o

LOBJS     = lgraphlib.o lplualib.o

all: $(BIN)/$(PROG).prc

res: resource.tmp
	$(PILRC) resource.tmp .
	touch res

resource.tmp: Makefile resource.rcp
	sed -e "s/@@VERSION@@/$(VERSION)/g" < resource.rcp > resource.tmp

$(BIN)/$(PROG).prc: $(OBJS) $(LOBJS) $(LIBS) res
	$(MULTILINK)/m68k-palmos-multilink -basename PLUA -segmentsize 29k -nostdlib -libdir $(MULTILINK) -fid LuaP $(OBJS) $(LOBJS) $(LIBS) libnfm.a libnfm2.a /opt/palmdev/sdk-5/lib/m68k-palmos-coff/libPalmOSGlue.a
	rm -f pref0000.PLUA.grc
	cp pref0000.dat pref0000.PLUA.grc
	$(BUILDPRC) $(BIN)/$(PROG).prc Plua2 LuaP *.PLUA.grc *.bin

clean:
	rm -f *.[o] *.bin *.PLUA.grc res $(BIN)/$(PROG).prc PLUA*.out resource.tmp
