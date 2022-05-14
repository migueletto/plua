LUA      = ../../..
include $(LUA)/config.palmos

PILRC    = pilrc
BUILDPRC = build-prc
HELP     = plua2help
PARTS    = basic.txt coroutine.txt debug.txt file.txt math.txt os.txt plua.txt string.txt table.txt

all: $(BIN)/$(HELP).prc

$(BIN)/$(HELP).prc: $(HELP).rcp
	$(PILRC) $(HELP).rcp .
	$(BUILDPRC) --no-check-resources -o $(BIN)/$(HELP).prc -t Help -c LuaP -n Plua2Help *.bin

$(HELP).rcp: $(HELP).txt
	./gerahelp.ksh $(HELP).txt

$(HELP).txt: $(PARTS)
	cat $(PARTS) | tr -d '\r' | awk 'NF>1' > $(HELP).txt

clean:
	\rm -f *.bin *.dat index.txt words.txt defs.txt $(HELP).txt $(HELP).rcp $(BIN)/$(HELP).prc
