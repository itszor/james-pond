James Pond (Vectordean for Millennium, 1990; Archimedes conversion by Krisalis, 1991) - static recompilation
=========================================================================================================

A recipe: this repository holds no game data. It rebuilds the game as a
native executable from your own copy, recognised by content (recipe.json),
so it can be shared without distributing the game.

    ./build.sh "James Pond (1990)(Krisalis).adf"
    cd recomp && ./jamespond

build.sh runs recipe.json's steps with SuperSpider's tools/weave.py
(~/stuff/superspider, or $SUPERSPIDER; $SS below): it extracts the disc,
runs LOADER's own decruncher on the game (tools/unpack.py), analyses and
translates everything, and links it with libroger (~/stuff/libroger, or
make ROGER=...). Needs python3, OCaml/dune, gcc and SDL3.

The inputs - any one of these (all hold the same game):

- TOSEC's "James Pond (1990)(Krisalis).adf" or "...[a].adf" (ADFS E,
  800K). They differ only in !run's last *WimpSlot (480K or 552K) and a
  duplicate Graphics.HOME on the first.
- TOSEC's "James Pond (1990)(Krisalis).zip": !JamesPond as plain files
  (RISC OS filetypes in the zip's extra fields, some members imploded).
- "James Pond (1991)(Krisalis Software)(1994 Buzz re-release).hfe", the
  Buzz budget re-release (an HxC flux image): the same LOADER, JamesCode
  and data, with a new !run, a module that runs LOADER (PONDRUN: *Go 8000,
  then MODE 0) and an A3010 joystick module (JOYPATCH, the same SWIs as
  JoyMod).

What it makes:

    extracted/disc        the disc, via weave.py
    game/!JamesPond       libroger's appdir (../game)
    unpacked/jamescode    tools/unpack.py: JamesCode as LOADER unpacks it, at &B000
    analysis/*.json       SuperSpider's analyses (game, loader, musmod)
    recomp/gen*/          their C translations
    recomp/jamespond      the game

Committed: recipe.json, build.sh, tools/unpack.py, recomp/Makefile,
recomp/game.c, recomp/jamespond.ini, this file.

Playing
-------

    ./jamespond                 as !run: the Krisalis logo, the intro, the title
    ./jamespond --level=5       a new game starts at level 5 (1-12)
    ./jamespond --crop          less letterboxing on a widescreen monitor:
                                rows 0-201 once JamesCode starts (the
                                Krisalis logo before it is full screen);
                                recomp/jamespond.ini

Keys (from the game's Read_Me): Z left, X right, P up, L down, Space
blows a bubble (fire); Tab pauses; F1/F2 volume up/down, F3/F4 maximum
and minimum volume. Escape ends the game, and on the title screen quits.
A game controller is Keith Martin's joystick interface (JoyMod's, which
the game reads: Joy_Read0) - the stick or D-pad moves, A fires - or, with
--joystick=off, the keys above (A Space, B P, Start Tab).

How the game is put together
----------------------------

It's the Amiga game, converted: LOADER and JamesCode are crunched with an
Amiga-style backwards cruncher (as Chuck Rock's levels are), and the
title pictures are Amiga bitplanes LOADER converts to Archimedes pixels.

!run sets Pond$Dir, runs MemMod (TequeMemory: *TequeMem, which frees
memory), RMLoads MusicModule 1.02 (MUSMOD2: Krisalis' tracker player, a
different build from Chuck Rock's) and JoyMod (Keith Martin's joystick
module 4.27), *MusSampleRate 3, then `Load LOADER 8000 ; Go 8000`:

- LOADER (absolute, &8000-&9ED0): MODE 9, the Krisalis logo
  (Graphics.KRISALIS, crunched planar data, faded in and out), then it
  loads JamesCode at &AFFC, decrunches it to &B000 (&8284: the data
  backwards from its end, so it unpacks in place over itself; a checksum
  in r5) and branches there.
- JamesCode, unpacked (&B000-&2D8B0): the game. It installs a vsync
  event handler (EventV: the frame counter its main loop waits on, two
  vsyncs a frame, and the screen bank flip, OS_Word 22) and a raw IRQ
  handler at &100: at vsync it starts IOC timer 1 to fire part way down
  the screen, where it loads the status panel's palette into VIDC, and
  sets a flag that the palette fades wait for. It reprograms VIDC's
  display window every frame (a smaller picture than MODE 9's). Its files
  are Graphics.* (the intro, the title, the ending), Maps.Map0-11,
  Sprites.Sprite0-11 and Mask0-11 (one of each per level) and Music.*
  (JPGame1 the title's tune, Samples the game's sound effects, JPScore the
  high score table's), loaded into MusicModule's workspace.

What's translated (recomp/Makefile, make regen): the game (gen/, for
&B000), LOADER (genload/) and MusicModule (genmus/, as a module at
&1800000). libroger selects the game's translation when LOADER loads
JamesCode into its buffer at &AFFC (roger_program.at) and jumps to &B000.

Everything is translated: headless runs of every level (--level=N) with
random play, a game played to game over, the name entry and the high
score table and back to a new game, Escape out of a game and out of the
title, reach no untranslated code. The ending (Graphics.End, after level
12) wasn't reached in testing; its code is translated.

The name entry writes one byte past its alphabet ("ABC...][", &154FC),
into the instruction after it (&1551C): libroger interprets that word as
it now is, as the original would run it.

What SuperSpider and libroger needed
------------------------------------

Nothing in SuperSpider. In libroger:

- the raw IRQ handler at &100 (roger_config.irq1v) is entered in IRQ mode,
  on an empty IRQ stack, as the hardware's IRQ vector enters it, and the
  interrupted mode is restored afterwards. It used to run in whatever mode
  was interrupted, with R14 = the return without mode bits: James Pond's
  returns with movs pc,lr, and an interrupt during one of MusicModule's
  SWIs (SVC mode) left the CPU in USR mode with the SVC stack as R13 -
  the game's stack was lost (it then "returned" to 0 and ran LOADER again).
- &100 starts out holding the OS's IRQ handler (HANDLER_RET), for a
  program's handler that passes each interrupt on to the one it replaced
  (James Pond's does, ldr pc,[pc,#-4], for vsync); it used to be 0.

Regression: the other games' headless runs (30M instructions, and
Lemmings', the other irq1v game, to 400M) are byte-identical with
libroger before and after.

Not supported: the RTFM joystick interface (SWI &A1540, the !RTFMJoyen
module on the TOSEC discs) and the serial port joystick modules; a game
controller stands in for Keith Martin's interface.
