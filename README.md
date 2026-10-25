# Death Rally Advance: _Death Rally_ on the Game Boy Advance

A fork of [**dRally**](https://github.com/urxp/dRally) by Neomalius (urxp)

dRally is an open-source engine for _Death Rally_ (1996), rebuilt from the original executable, that runs natively on Linux and BSD. **Death Rally Advance** reorganizes its source into modules and starts a port to **the Game Boy Advance**, as a cartridge ROM with the game data built in.

This is an early work in progress. The GBA side is a **rendering prototype**: it draws the game's real tracks and cars with the GBA's tile and sprite hardware at 60 frames per second, but the driving is a stand-in. The game's own race code already runs on the GBA in a separate test, which replays a recorded race, but it is still far too slow. See [Status](#status).

## Game Data

No game data is included. You need the files from the original _Death Rally_. The port has been developed with the free registered Windows version, see [Installation](#installation).

The GBA build puts the game data inside the ROM, so a ROM carries the same restrictions as the data files it was built from: **don't share ROMs**.

## Game Boy Advance

### Screenshots

| | |
|:-:|:-:|
| ![The start grid](docs/screenshots/gba-start-grid.png) | ![Racing on the first track](docs/screenshots/gba-racing.png) |
| The start grid of the first track | Racing, with the frame rate meter on top |
| ![Track 3](docs/screenshots/gba-track-2.png) | ![Track 10](docs/screenshots/gba-track-9.png) |
| Track 3, with the other cars on autopilot | Track 10 |

Screenshots are taken in mGBA and shown at twice the GBA's resolution.

### What the Prototype Does

`gba/proto` tests whether the GBA can draw a race:

* The track is a 256-color tile background streamed from ROM through a ring of tiles, so tracks of any size scroll without being loaded into RAM.
* The four cars are hardware sprites, using the game's own 96 rotation frames and body colors.
* Car shadows are drawn by the OBJ window with the darken effect, instead of by drawing pixels.
* Cars stay on the road using the track's mask, the same one the game uses for its walls.

The driving, the collisions and the other cars' autopilot are simple stand-ins. None of the game's race code runs on the GBA yet.

In mGBA all ten tracks run at 60 frames per second using about 12% of each frame, of which at most 9% is graphics. That leaves most of the frame for the game's own race code, which is the next thing to measure. The prototype hasn't been tested on real hardware yet.

### The Race Code on the GBA

`gba/replay` runs the game's own race logic, `race_logic()` in `src/race/race_logic.c`, on the GBA. It replays a race recorded by the desktop game and times every frame:

```sh
make ticklog                 # a desktop build that records the first race to game/TICKLOG.BIN
cd game && ../build/ticklog/drally_ticklog && cd ..   # start one race, the AI drives your car
make -C gba/replay           # builds gba/replay/drreplay.gba from game/TICKLOG.BIN
make -C gba/replay PROFILE=1 # also times each part of the logic and counts the soft-float calls
```

For each frame the recording holds what changed in the logic's globals before it ran (the input, the timer and what the renderer touched) and a hash of the globals after it. The GBA applies the changes, runs the logic and compares hashes. The track maps and car images are read straight from ROM, as they will have to be in the port.

Results in mGBA, for a whole 4-lap race (10,821 frames) with the AI driving all four cars:

* **The GBA computes exactly the same race.** Every frame's hash matches the desktop's, `sin()` and `cos()` included.
* **It is about 16 times too slow.** A tick of the logic takes 3.9 million cycles, 13.7 GBA frames. The game runs its logic 70 times a second, so 60 frames per second needs a tick in about 200,000 cycles, with room left for drawing.
* **Where the time goes:** `sin()` and `cos()` are called 174 times a tick at about 12,000 cycles each, half of the total. The other 2,200 soft-float calls a tick (the code keeps floats but computes in double) take about a quarter, and software integer division about a tenth. The rest is the race code itself, mostly the car physics (`race___4c434h`) and the AI (`race___4b62ch`), which run once per car.

So the race code needs to move to fixed-point math with table-based `sin()` and `cos()`, and its hot loops to ARM code in IWRAM. The replay checks those changes: recorded again with the changed desktop code, a race has to replay with matching hashes.

### Building

Requirements:

* The devkitPro GBA toolchain. Install devkitPro's pacman (see [Getting Started](https://devkitpro.org/wiki/Getting_Started)), then run `sudo dkp-pacman -S gba-dev`.
* A C compiler for Linux, to build `tools/gbabake`, which turns the game data into the layouts the ROM uses.
* The game data in `game/` (or pass `GAME=/path/to/game`).

```sh
make -C gba/proto            # builds gba/proto/drproto-tr0.gba
make -C gba/proto TRACK=5    # any track, 0-9
```

Each ROM holds one track and is about 3 MB.

### Controls

| Button       | Does                                     |
|--------------|------------------------------------------|
| D-pad        | Steer left and right                     |
| A            | Accelerate                               |
| B            | Brake, then reverse                      |
| R            | Let the autopilot drive                  |
| Start        | Back to the start grid                   |
| Select       | Show or hide the frame rate meter        |

The meter shows frames per second, the share of each frame used (CPU) and the part of that spent on graphics (GFX).

### Status

Working: drawing any of the ten tracks with four cars and their shadows at 60 frames per second in mGBA, and running the game's race logic on the GBA with the same results as the desktop.

Not done yet:

* Making the game's race code fast enough: it runs on the GBA and computes the same race as the desktop, but about 16 times too slowly (see [The Race Code on the GBA](#the-race-code-on-the-gba)).
* Skid marks, wrecks and the track under obstacles, which the race code draws into the track image. The port needs a writable overlay for the tiles they touch.
* Menus, shops and the underground market, which are drawn at 640x480.
* Music and sound (the S3M and XM songs, through Maxmod).
* Saving in SRAM.
* Testing on real hardware.

## Desktop

### Requirements

* GCC or Clang
* GNU Make
* SDL2
* SDL2_net

On Debian/Ubuntu: `sudo apt install build-essential libsdl2-dev libsdl2-net-dev`

### Building

```sh
make -j$(nproc)
FLAGS="YOUR CFLAGS" make   # custom flags, default is -O3
make check                 # LTO build, fails on mismatched declarations across files
```

### Installation

The game needs the original game assets, for example the [free registered Windows version on CHIP](https://www.chip.de/downloads/Death-Rally-Vollversion_38550689.html):

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

### Running

```sh
make run            # or ./scripts/drun, ./scripts/drun_gdb for gdb
```

Multiplayer is not available yet.

## Source Layout

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
    gba/replay        runs recorded races through the race logic on the GBA
    include/nosdl     SDL stand-in for building the game's code without SDL
    scripts           run helpers, discmp.sh to compare generated code

Function files are named after their address in the original executable (e.g. `___3e720h.c`). Keep that name when moving them between modules.

## License

MIT, see [LICENSE](LICENSE).

## Credits

- dRally: Neomalius (urxp)
- Game Boy Advance port: Berobloom
- _Death Rally_ (1996): Remedy Entertainment and Apogee Software
