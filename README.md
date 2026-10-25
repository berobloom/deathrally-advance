# dRally

The main goal of this project is to create a port of Death Rally (1996) running natively on Linux and BSD based operating systems.

#### Linux requirements
* GCC/Clang C compiler
* GNU/Make
* SDL2
* SDL2_net

On Debian/Ubuntu: `sudo apt install build-essential libsdl2-dev libsdl2-net-dev`


#### Building

```sh
make -j$(nproc)
FLAGS="YOUR CFLAGS" make   # custom flags, default is -O3
make check                 # LTO build, fails on mismatched declarations across files
```

#### Installation - needs original game assets

* [Death Rally registered free windows version CHIP](https://www.chip.de/downloads/Death-Rally-Vollversion_38550689.html)

```sh
7z e -o game DeathRallyWin_10.exe
cd game && mkdir CINEM && mv ENDANI* CINEM && mv SANIM* CINEM
echo "./CINEM" > CDROM.INI
```

Only versions including the DR.IDF file are able to use the `-DDR_CDCHECK` define.

    game
    |--CINEM
    |  |--DR.IDF
    |  |--ENDANI.HAF
    |  |--ENDANI0.HAF
    |  |--SANIM.HAF
    |--CDROM.INI        [1]
    |--ENGINE.BPA
    |--IBFILES.BPA
    |--MENU.BPA
    |--MUSICS.BPA
    |--TR[0-9].BPA

    Make sure these file/dir names in the game directory are in uppercase.

    [1] CDROM.INI contains relative location of CINEM directory (./CINEM)

#### Running

```sh
make run            # or ./scripts/drun, ./scripts/drun_gdb for gdb
```

#### Source layout

    src/core          entry point, system/SDL layer, input, memory, math
    src/gfx           display, palettes, fonts, cinematics
    src/sound         audio mixer, S3M/XM players
    src/data          global data/bss, assets, BPA archives
    src/menu          main menu
    src/race          race engine
    src/shop          shop
    src/underground   underground market
    src/net           multiplayer (IPX over UDP)
    src/dfr           __dfr_* functions
    src/decomp        decompiled functions not yet sorted into a module
    include           headers
    tools/ipxnet      standalone IPX relay server
    tools/gbabake     turns game data into GBA layouts
    gba/proto         Game Boy Advance rendering prototype
    scripts           run helpers, discmp.sh to compare generated code

Function files are named after their address in the original executable
(e.g. `___3e720h.c`), keep that name when moving them between modules.

#### Game Boy Advance prototype

`gba/proto` tests whether the GBA can draw a race: a real track as a tile
background streamed from ROM, four cars as sprites with the game's own 96
rotation frames, and shadows through the OBJ window. The driving and the
other cars' autopilot are stand-ins; none of the game's code runs yet.

Needs devkitARM with libgba (`sudo dkp-pacman -S gba-dev`) and the game data
in `game/`:

```sh
make -C gba/proto            # drproto-tr0.gba
make -C gba/proto TRACK=5    # drproto-tr5.gba, any track 0-9
```

D-pad left/right steer, A accelerates, B brakes and reverses, R lets the
autopilot drive, Start goes back to the grid, Select hides the meter. The
meter shows frames per second, the share of each frame used (CPU) and the
part of that spent on graphics (GFX). The ROMs contain game data: don't
share them.

#### Work in progress
*   Multiplayer not available
