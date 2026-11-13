# Death Rally Advance: _Death Rally_ on the Game Boy Advance

A fork of [**dRally**](https://github.com/urxp/dRally) by Neomalius (urxp)

dRally is an open-source engine for _Death Rally_ (1996), rebuilt from the original executable, that runs natively on Linux and BSD. **Death Rally Advance** reorganizes its source into modules and starts a port to **the Game Boy Advance**, as a cartridge ROM with the game data built in.

This is an early work in progress. On the GBA you can now **drive a race run by the game's own race code**, against the game's AI, on any of the ten tracks, forward or (all but the first) reversed, in any of the six cars, with the laps, the rivals' skill, your upgrades and weapons or none chosen at the start: `gba/race`. The game's own code sets the race up on the GBA, and the race is drawn with the GBA's tile and sprite hardware at 60 frames per second, at the game's real speed. Skid marks, mines, power-ups and obstacles show on the track, and spectators, wrecks, explosions, shots, the start lights and the finish flag over it. The game's own HUD box shows turbo, ammo, mines and damage. Each track's music plays, with the engine and the race's other sound effects. A career, the game's own rounds of three races with prize money, points and ranks against the other 19 drivers, up to the race against the Adversary, plays on menu screens drawn the way the desktop's are. See [Status](#status).

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

* **The GBA computes exactly the same race as the desktop build of the same code.** Every frame's hash matches.
* **A tick of the logic takes 167,500 cycles on average, 180,000 in the busiest 1,000 frames.** The original code took 3.9 million. The game runs its logic 70 times a second, so 60 frames per second needs a tick in about 200,000 cycles, with room left for drawing.
* **How it got there:** `sin()` and `cos()` from a table, libgcc's soft-float in IWRAM, a map of where the walls are, single precision instead of double in the physics, AI and skid marks, cheaper searches, the innermost loops as ARM code in IWRAM, word loads instead of byte loads, and no library divisions. Each step was checked against the replay. The steps that changed the math were also checked against how the race comes out (see below).

`tools/replay` replays a recording on the desktop. It prints each car's finish frame and place and a hash of the track image, and it can write the recording again with the current build's hashes, which is what the GBA then checks against. The finish frames tell whether a change to the math changed the driving: rounding one constant differently moves them by about 50 frames.

The details, and what is left to do, are in [docs/gba-race-speed.md](docs/gba-race-speed.md).

### A Race on the GBA

`gba/race` puts the two together: the game's race code runs the race, and the GBA draws it the way `gba/proto` does. For each tick of the game's 70 Hz timer it does what the desktop game's race loop does for a frame:

* The GBA's buttons become the keyboard state that the game's timer interrupt (`race___40164h()`) turns into your car's controls.
* `race_logic()` runs the tick, with the AI driving the other three cars.
* `race_render()` keeps the camera, animations, lap times and other state that the desktop updates while drawing. Built with `DR_RACE_NODRAW`, it skips the drawing itself. A desktop check (`DR_CHECK_NODRAW=1` in a `make ticklog` build) runs every frame of a race both ways and finds no difference in the race.
* The cars' sprites come from their positions and rotation frames in the game's car structs, and the camera from the game's own look-ahead.

A reversed track is the whole track turned round by 180 degrees, as the game's `race___4af3ch()` does it to its buffers, with its own colors (`TRn-FLIP.PAL`) and obstacle pictures (`OBST_REV.BPK`). The game reverses all but TR0. On the GBA the cars' frames are baked turned round. The track's mask and AI maps are read turned (`race_mas_reverse()`, `race_vai()`, `race_lr1()`), and so is the image (`race_ima_reverse()`), and its tiles are shown with the screen's own flip bits. `tools/replay --race 3r BAKEDIR SEED` runs a reversed race from the baked data, and `files:GAMEDIR` instead of BAKEDIR from the game's files. All nine reversed tracks give the same result both ways and on the GBA.

The race code draws skid marks, mines, craters, power-ups and obstacles into the track image, and reads back the pixels under them. The whole image doesn't fit in the GBA's RAM: TR0's alone is 720 KB. So the race code goes through a small API, `race_ima_read()`, `_write()`, `_draw()` and `_lookup()` in `src/race/race_ima.c`. On the desktop it works on `TRX_IMA` directly. Built with `DR_IMA_TILES`, as `gba/race` is, the image is the track's 8x8 tiles in ROM, with each tile the race draws into copied to a pool of 2,048 in EWRAM (128 KB). A whole race draws into at most about 1,800. The vblank interrupt loads the changed tiles that are on screen. `make -C tools/replay IMA_TILES=n` builds `replay-tiles`, the desktop with the GBA's tiles, for checking the two against each other.

What the race draws over the track, the spectators and their splats, wrecks, explosions, smoke, gun flashes, shots, rockets, the start lights and the finish flag, goes through `race_sprite()` in `src/race/race_render.c` as well as into the desktop's back buffer. On the GBA a callback turns each picture into a hardware sprite, in front of the cars or behind them in the order the game draws them, with up to 40 a frame. The start lights and the finish flag, which waves on the leader's last lap, are 80x60 pictures in two sprites. `tools/gbabake` bakes the 16 picture files into `sprites.bin` (360 KB), of which the ROM keeps all but the lights' and the flag's pixels (46 KB): the race code only needs their addresses. It bakes the lights' 3 frames and the flag's 64 into `lamps.dtl`, laid out as the sprites' 96 tiles, each different tile once (89 KB), so a new frame is 96 tile copies.

The sound goes through [Maxmod](https://maxmod.org/). `tools/gbabake` decodes each track's song, `TRn-MUS.CMF` in MUSICS.BPA (an S3M), and the race's sound effects, the instruments of `GEN-EFE.CMF` (an XM), into files that devkitPro's `mmutil` packs into a soundbank. The race code asks for effects through the same `dRally_Sound_pushEffect()` and `_adjustEffect()` calls as on the desktop, and `gba/race/sound.c` plays each one on a Maxmod effect with the game's rate, volume and balance. The engine's pitch follows its speed on every tick. Maxmod mixes 24 channels at 16 kHz in the vblank interrupt, which takes about 17% of the GBA's time.

The HUD box at the top left, with the turbo bar, the damage, and the ammo bar and mines (or the lap time in a race without weapons), is drawn by the game's own `race___40864h()`. The time it keeps the last lap's time up and the damage warnings, which it also did, are now in `race___40864h_state()`, which runs whether anything is drawn or not. On the GBA the box is drawn into a buffer that stands in for the back buffer, copied to 32 tiles of the HUD's background layer, with a black layer under it for the box's colour 0. It's drawn again only when a number or bar in it changes, at most 30 times a second, and not while the logic is behind. `tools/gbabake` bakes its pictures from IBFILES.BPA into `hud.bin`.

The game's frame counter, which times the rocket flames' and other animations, now counts the 70 Hz timer's ticks in `gba/race` and `tools/replay --race`. That changes how a race comes out, so the replay hashes from before don't apply to those two.

That's one tick for each `race_logic()` call, as the original did on its 70 Hz VGA screen. The GBA shows 60 frames a second, so every sixth frame has two ticks to run. The vblank interrupt draws the state after the last finished tick, so a tick can run on into the next frame without holding the screen up.

The race is set up by the game's own setup code, `race_setup()` in `src/race/race_setup.c`, as on the desktop. Instead of reading and decoding the game's files, it uses the track and the car images that `tools/gbabake` decoded and baked into the ROM (`src/race/race_load_baked.c`). The track's maps stay in ROM. The menus' part, `race_quick()` in `src/race/race_quick.c`, sets up a quick race: you in a Camaro against three of the game's drivers, four laps, with a new random seed for every race.

Setting up takes about 30 frames. Checks that it's the same setup as the desktop's:

* A `make ticklog` build run with `DR_CHECK_SETUP=DIR` sets each race up twice in child processes: from the game's files and from the bakes in `DIR/TRn`. It compares everything the race logic uses: its globals, the track maps, which pixels of the cars are solid, and the obstacles. They're the same on every track tried.
* `tools/replay --quick TRACK BAKEDIR SEED` does the GBA's setup on the desktop. Its hash matches the one `gba/race` logs for the same seed, on every track tried. `tools/replay --setup TICKPRE.BIN BAKEDIR TICKLOG.BIN` sets up from the state the desktop menus left, and finds only the AI flag and two timer counters different from the recorded race's first frame.

* `tools/replay --race TRACK BAKEDIR SEED` runs the whole quick race on the desktop the way `gba/race`'s benchmark build does. It prints a hash of the globals and of the track image at the end. Both match the GBA's on all ten tracks, so the desktop and the GBA run the same race from start to finish.

Comparing the two found a bug in the decompiled code: two calls to the random number generator in one statement, whose order C leaves open. The desktop and the GBA compilers placed the race's power-ups differently. They're now called in the desktop's order.

### The Career

The game keeps a career's state between races in its menus' functions, mixed in with their drawing: the sign-up (`___3266ch()`), PREPARE TO RACE (`___33010h_cdecl()`), which also runs the race, the round's results (`___3892ch_cdecl()` and the three race results screens) and the Adversary's challenge (`___31008h()`). The state half of each now lives in `src/career/career.c`, which the desktop's menus call as they did before and the GBA calls from its own screens:

* `career_new()`: a new game, as START A NEW GAME leaves it: your name, face and difficulty, and the 19 others with their random money and upgrades.
* `career_signup_open()`, `_check()`, `_signup()`, `_rest()` and `_order()`: the round's three races and their tracks, the others signing up at random while you choose, and each grid's order.
* `career_race_track()`, `_fill()` and `_after()`: the race's track, laps and grid, the racers into the race's inputs (`___1de7d0h`), and after the race the prize money, the kill bonuses, the damage and the wins.
* `career_results()`: the round's points, the racers sorted and ranked on them, and your lap record.
* `career_leader_round()` and `career_adversary_*()`: once you lead, the race against the Adversary, or chickening out.
* `career_shop_*()`: the shop's prices, what your car and upgrades fetch towards a new one, buying a car, an engine, tires, armor or a repair, and the end of the road when your car is a wreck you can't pay to fix. The desktop's shop (`shop___28e40h()`) calls them too.
* `career_market_*()` and `career_loan_*()`: the underground market's prices for your car, buying mines, spiked bumpers, rocket fuel or sabotage once between races, and the loan shark's loan, paying it back, and his collecting it after three races, or hurting you. The desktop's market (`underground___2e350h()`) and the loan shark's visit (`___168b4h()`) call them too.
* `career_signup_sabotage()`: after the sign-up, the others' cars repaired, and the sabotage you paid for done to the best placed of your rivals (`___32230h()`'s). The GBA and `tools/replay --career` left the others' damage from race to race until this, so their career hashes from before don't apply.

The results screens' functions also hand out the points. Built with `DR_CAREER_NODRAW` they leave their drawing out and keep the rest, and `career_results()` calls them.

On the GBA, `gba/race/career_ui.c` draws its own screens around them: a new game, the hub with your rank, points, money, car, damage and loan, the shop, the underground market, the loan shark's visit, the sign-up and the sabotage it brings with the three races filling up, PREPARE TO RACE, the race, its results, and the standings with what each driver scored in the round. The shop shows the car on sale, or yours, turning in the corner, from the race's car frames, with the game's own words for each car, upgrade and repair (`src/shop/shop_texts.c` and `car_data.c`), and so does the market for its items and loans. A car with spiked bumpers races with the game's spiked picture and shape. The racers' car colors come from MENU.BPA's `CARCOL.PAL` and `COPPER.PAL`, which `tools/gbabake` bakes. A career's races follow one another without resetting the race's globals, as on the desktop.

The screens look like the desktop's menus, made smaller to fit. Each has the desktop's red background with the DEATH RALLY banner (`MENUBG5.BPK`, scaled to 3/8 by `tools/gbabake`, 593 different 8x8 tiles) on one layer, and its text on another. The text is drawn in the desktop's own fonts at half their size, the small one (`F-SMA3A.BPK`) for words and the big one (`F-BIG3A.BPK`) for menus, with the chosen row in white, the spinning cursor (`CURSOR.BPK`) beside it and the others in grey. The fonts are drawn in `MENU.PAL`'s grey ramp, so a tile's palette bank turns them into its other ramps: blue for the points gained, the fire colors for the race chosen in the sign-up. The sign-up shows each race's track as the desktop's does, from `TRSNAP2M.BPK`'s pictures halved to 64x56 (`tracks.obj`), in three sprites. The hub and the sign-up show the desktop's side panel (`___25330h()`) on the right, halved: your car in your color, your name, money, the upgrades bought and not, damage, top speed and rank. `tools/gbabake` halves `STATBAS7.BPK`, `SCENECAR.BPK` and `STATPOP4.BPK` keeping each pixel in the 16-color ramp most of its four are in, so the car's body stays in the colors 64-95 that your color sets (`gba_panel_car_color()`, as `___11378h_cdecl()`). The desktop's boxes (`___13248h_cdecl()`) frame each screen's menu or words on a third layer: `tools/gbabake` draws one at the desktop's size and halves it, and keeps its corners, edges and inside as 22 4bpp tiles (`box.bin`) that `gba_box()` lays out at any size. The text layer has a 4bpp tile for each 8x8 of the screen, which `gba_print()` draws into. The screens line their columns up with spaces, so after two spaces a word starts in its own 8x8 cell. A string already on screen isn't drawn again, so screens that are redrawn every frame don't flicker. The race takes the menus' tiles back when it starts (`use_track()`).

`tools/gbarun` runs a ROM headless in mGBA's library: it presses buttons, writes and reads memory, and saves screenshots at the frames a script gives, and prints what the ROM logs. With `-s FILE` it keeps the cartridge's save memory in FILE between runs, and with `-a FILE.wav` it records the sound. `tools/gbarun/shop.txt` walks a new career into the shop. It needs mGBA's library and headers (`libmgba-dev`).

`tools/replay --career BAKES SEED ROUNDS` plays a career on the desktop as `gba/race`'s career benchmark does (`RACE_CFLAGS="-DRACE_BENCH -DCAREER_BENCH=n -DRACE_SEED=s"`): always the easy race, with the AI driving your car. It prints a hash of the race's globals after each race and of the 20 racers after each round. Four rounds match the GBA's, and so do three races against the Adversary (`CAREER_LEAD=1`, and `-DCAREER_LEAD` on the GBA, give you the lead from the start). In one of those, the engine sound's pitch at the end differs, which nothing else reads.

### Speed

In mGBA, with the AI driving all four cars, a whole race runs at 60.0 frames per second at the real 70 ticks a second, on all ten tracks. At most two seconds a race show 59, which is one frame shown twice. If the logic falls more than two ticks behind, the game slows down rather than catching up. `make TICKS=60` runs one tick a frame, at 6/7 of the game's speed. `RACE_CFLAGS="-DRACE_BENCH -DRACE_SEED=n"` builds a benchmark: one tick a frame however long it takes, with the AI driving your car. At the end it logs the cost of a tick and a hash of the race, which a change that is meant to keep the race the same must not move.

### Building

Requirements:

* The devkitPro GBA toolchain. Install devkitPro's pacman (see [Getting Started](https://devkitpro.org/wiki/Getting_Started)), then run `sudo dkp-pacman -S gba-dev`.
* A C compiler for Linux, to build `tools/gbabake`, which turns the game data into the layouts the ROM uses.
* The game data in `game/` (or pass `GAME=/path/to/game`).

```sh
make -C gba/proto            # builds gba/proto/drproto-tr0.gba
make -C gba/proto TRACK=5    # any track, 0-9
make -C gba/race             # builds gba/race/drrace.gba: the career, or a quick race on any track, in any car
make -C gba/race TRACK=8     # drrace-tr8.gba, track 8 only, which starts straight away
```

Each prototype ROM holds one track and is about 2 MB. The race ROM is about 16.4 MB of the cartridge's 32, with the 4.3 MB soundbank of the ten songs and 44 sound effects. Each track's image is its 8x8 tiles, each different one once (gbabake's `track.dtl`), 5.3 MB for the ten.

#### The track's buildings

The desktop draws each track's buildings, walls, cliffs and trees (`TRn-SCE.BPK`) as 3D triangles over the track every frame, and the track's image has holes where they stand. `tools/gbabake` draws them into the image instead (`bake_buildings()`), seen from straight above: each point where the desktop puts the ground's, 1/256 of its X and Y from the object's place, the faces that look up drawn the lowest first, in their colors and the trees' shades as `race___53464h()` works them out (`race___4a6b4h()` shades them). The textures, which the desktop can leave off, and the shadows, which go through tables it leaves empty, aren't drawn. So the buildings don't lean as the view moves, and they're under the cars, not over them. The race's logic doesn't read those pixels: its results are the same.
 The 13 car images' 96 rotation frames, the seven cars and the first six's spiked ones, both ways round, are kept once, as the race code reads them, in the first car's colors, and cut into the sprite's tiles in the others' colors as they're copied to VRAM. Each track's mask, the 4 bits under each pixel that the race reads for walls and surfaces, is kept in 8x8 blocks, each different block once, with the blocks that are one value throughout as just that value (`DR_MAS_BLOCKS`, `src/race/race_mas.c`): 55 to 140 KB a track, where the mask and its reversed copy took 1.4 MB, since a reversed track reads the same blocks turned round, as it reads the maps of the way round and of left and right (`race_vai()`, `race_lr1()`). The races come out the same: the benchmark's hashes match on every track, both ways, and the race logic takes about 2% longer. Making the reversed cars' frames from the forward ones, instead of keeping them (1.07 MB), was tried and dropped: the turned frames have to be in EWRAM, which the race's pixel reads find slower than ROM, and reversed races took 20 to 25% longer. The shop's animations are LZ77 compressed (`shop.lz`, 223 KB, from 391), and the BIOS unpacks a frame straight into VRAM when it changes. The soundbank's samples are as the game has them: only 86 KB of them are shared between songs, so it would only get smaller by lowering their rates, which costs sound quality.

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

In `gba/race`:

| Button       | Does                                     |
|--------------|------------------------------------------|
| D-pad        | Up accelerates, down brakes, left and right steer |
| A            | Machine gun                              |
| B            | Turbo                                    |
| R            | Drop a mine                              |
| Start        | Let the AI drive your car, or, once the race is over, go on (so does A) |

The title screen chooses the career or a quick race. In the career's screens, the D-pad moves, A chooses and B goes back. A new game asks for your name (A on it, then up and down change the letter and left and right move along), your face, which is one of the drivers' and leaves that driver out, and the difficulty. At the sign-up, left and right pick a race and A signs up, or B sits the round out. The others sign up as you choose, and a race that has its four is full.

The career saves itself in the cartridge's SRAM each time you're back at its hub, in the desktop's saved game's form, and the lap records and hall of fame with it, which start as a new DR.CFG's. The events' speeches show the game's pictures, halved, and the hub and a new game your driver's face. The shop shows the upgrade on offer, the repair and the way out turning as its own animations do, halved, where the car you'd buy turns on its row, and the market each item's picture, on sale, held or sold out. The menus play the game's menu music, jumping in it where the desktop's do. The title screen shows the hall of fame, and the race's screen before it the track's lap record for your car. Choosing the career again offers to continue it or start a new one; a career that's over, won or lost, is no longer offered.

Before a quick race, up and down pick a row and left and right change it: the track, which pans by slowly, forward or reversed (REV), your car, which turns in the middle of the screen, the laps (1-9), the rivals' skill, your car's upgrades, and whether there are weapons. A or Start races, and B goes back to the title. Without weapons nobody has guns or mines, and the HUD box shows the lap time instead. The rivals' skill is the game's three difficulties, from Speed Makes Me Dizzy (1) to Petrol In My Veins (3). Upgrades (0-4) fit your engine, tires and armor to that level, or to the best the car can take. The opponents are always the Deliverator, the Wraith and the Sentinel, with no upgrades.

Its meter, at the bottom right, shows frames per second and the share of the CPU used. The bottom left shows your position and lap.

### Status

Working: drawing any of the ten tracks with four cars, their shadows and their headlights at 60 frames per second in mGBA, setting a race up and running the game's race logic on the GBA with the same results as the desktop, and driving a race against the game's AI with it at the game's real speed, with the track's music and the sound effects. A career on screens like the desktop's menus, with the game's own sign-up, prize money, points, ranks and the Adversary, the shop, the underground market and the random events (the hitman's and the steroids' offers, winning streaks and the rest), the same as the desktop's.

Not done yet:

* In the career: the desktop's side panel in the shop and the market, where the turning car (in the track's colors) and the prices take its place.
* The headlights' exact colors. The game lightens each track color through the track's light table, on seven of the ten tracks; the GBA adds one color, semi-transparent, the closest of the track's palette to what the table adds on average.
* The track's buildings as the desktop draws them: in perspective from the middle of the screen every frame (`race___53464h()`), over the cars, with textures on top unless they're turned off. The GBA has them drawn into the track's image instead, seen from straight above (see [The track's buildings](#the-tracks-buildings)). The tracks have 1,600 to 5,100 triangles in 40 to 84 objects each, and 50 to 380 KB of textures, about 1.5 MB in all. Drawing them over the cars needs a layer of up to 600 8bpp tiles, and the BG VRAM is taken by the track's tiles and the HUD. Drawn into the track's tiles each frame instead, under the cars, a try found the time too much: a view has 130 to 170 triangles of 3 or so objects on screen, covering 150 to 320 tiles, but they overlap, and filling them, before any pixel is written, took 1 to 2.8 million cycles a frame, of the 280,000 there are and the 80,000 or so the race leaves free.
* The end's animation (`ENDANI.HAF`), 3.8 MB of 320x200 frames.
* The narrow view's HUD panel (`race___40f48h()`), which the GBA's screen doesn't use.
* The menus' own screens, which are drawn at 640x480.
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
    gba/race          a race on the GBA, run by the game's race code
    gba/game.mk       builds the game's code for the GBA, for gba/replay and gba/race
    tools/replay      the same on the desktop, and re-hashes recordings after a math change
    include/nosdl     SDL stand-in for building the game's code without SDL
    scripts           run helpers, discmp.sh to compare generated code

Function files are named after their address in the original executable (e.g. `___3e720h.c`). Keep that name when moving them between modules.

## License

MIT, see [LICENSE](LICENSE).

## Credits

- dRally: Neomalius (urxp)
- Game Boy Advance port: Berobloom
- _Death Rally_ (1996): Remedy Entertainment and Apogee Software
