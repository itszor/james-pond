# Static recompilation of James Pond: the game (gen/: JamesCode as LOADER
# unpacks it to &B000, ../tools/unpack.py), its loader LOADER (genload/) and
# MusicModule 1.02 (genmus/), from SuperSpider's analyse and gen, and game.c,
# linked with libroger (the RISC OS runtime).
ROGER   ?= $(HOME)/stuff/libroger
CC      ?= gcc
CFLAGS  ?= -O2 -g -w -fno-strict-aliasing $(if $(TRACE),-DRECOMP_TRACE)
GEN     := $(wildcard gen/code*.c) gen/entries.c
GENLOAD := $(wildcard genload/code*.c) genload/entries.c
GENMUS  := $(wildcard genmus/code*.c) genmus/entries.c
OBJ     := $(GEN:.c=.o) $(GENLOAD:.c=.o) $(GENMUS:.c=.o) game.o
LIBS    := $(shell pkg-config --libs sdl3) -lm -lpthread
INC     := -I$(ROGER)/include

jamespond: $(OBJ) $(ROGER)/libroger.a
	$(CC) $(CFLAGS) -o $@ $(OBJ) $(ROGER)/libroger.a $(LIBS)

$(ROGER)/libroger.a: FORCE
	$(MAKE) -C $(ROGER) libroger.a

%.o: %.c $(ROGER)/include/roger.h $(ROGER)/include/roger_cpu.h
	$(CC) $(CFLAGS) $(INC) -c -o $@ $<

# Code/data analysis: SuperSpider, into ../analysis/<name>.json, then
# superspider gen. Each image's ../analysis/<name>.roots adds entry points
# to the analysis: "make roots" merges the gap entries a --trace-gaps run
# logged (untranslated.log, or LOG=...) into them, so the next "make regen"
# translates them.
SS      ?= $(HOME)/stuff/superspider
SSBIN   := $(SS)/_build/default/bin/main.exe
AN      := ../analysis
APP     := ../game/!JamesPond
GAME    := ../unpacked/jamescode
# where the game ends (LOADER jumps there: --extern for its analysis)
GAME_END = $(shell printf %x $$((0xB000 + $$(stat -c %s $(GAME)))))
LOADER  := $(firstword $(wildcard $(APP)/LOADER,*) $(APP)/LOADER)
MUSMOD  := $(firstword $(wildcard $(APP)/MUSMOD2,*) $(APP)/MUSMOD2)
LOG     ?= untranslated.log

# --roots for ../analysis/<name>.roots, if there is one
roots = $(if $(wildcard $(AN)/$(1).roots),--roots $(AN)/$(1).roots)

# $(call analyse,name,image,base[,--module])
define analyse
	$(SSBIN) analyse "$(2)" $(3) $(4) --full $(call roots,$(1)) -o $(AN)/$(1).json
endef

regen:
	cd $(SS) && dune build
	python3 -I ../tools/unpack.py
	mkdir -p $(AN)
	$(call analyse,game,$(GAME),b000)
	$(call analyse,loader,$(LOADER),8000,--extern b000-$(GAME_END))
	$(call analyse,musmod,$(MUSMOD),1800000,--module)
	rm -rf gen genload genmus
	$(SSBIN) gen "$(GAME)" b000 $(AN)/game.json --c gen --leaders=dispatch
	$(SSBIN) gen "$(LOADER)" 8000 $(AN)/loader.json --c genload --leaders=dispatch \
	    --prefix=P --tabprefix=recompP --files=1 --covtag=P
	$(SSBIN) gen "$(MUSMOD)" 1800000 $(AN)/musmod.json --c genmus --leaders=dispatch \
	    --prefix=M --tabprefix=recompM --files=1 --covtag=M

roots:
	python3 -I $(SS)/tools/gap_roots.py "$(GAME)" b000 $(AN)/game.roots $(LOG)
	python3 -I $(SS)/tools/gap_roots.py "$(LOADER)" 8000 $(AN)/loader.roots $(LOG)
	python3 -I $(SS)/tools/gap_roots.py "$(MUSMOD)" 1800000 $(AN)/musmod.roots $(LOG)

clean:
	rm -f gen/*.o genload/*.o genmus/*.o game.o jamespond

.PHONY: FORCE regen roots clean
