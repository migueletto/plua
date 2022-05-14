LUA=../..
YAHM=/home/mar/yahm

include $(LUA)/config.arm

OBJS=main.o
LIBS=$(LIB)/liblua.a $(LIB)/liblualib.a

#AOBJS=pnomain.o pealstub.o pace.o
AOBJS=pnomain.o pealstub.o
ALIBS=$(YAHM)/lib/libarmboot.a $(YAHM)/lib/libarmhal.a $(YAHM)/lib/libarmui.a

all: pno.prc

armc.bin: $(AOBJS) $(OBJS)
	$(CC) $(CFLAGS) $(AOBJS) $(OBJS) $(LIBS) $(ALIBS) -o armc.bin

armc.ro: armc.bin
	peal-postlink -s 1000 -o armc.ro armc.bin

pnostub.o: pnostub.c
	m68k-palmos-gcc -O2 -c pnostub.c

peal.o: peal.c
	m68k-palmos-gcc -O2 -c peal.c

pnostub: pnostub.o peal.o
	m68k-palmos-gcc pnostub.o peal.o -o pnostub

pno.prc: main.def pnostub armc.ro
	build-prc -o pno.prc main.def pnostub armc.ro

clean:
	rm -f *.o armc.bin armc.ro pnostub pno.prc

