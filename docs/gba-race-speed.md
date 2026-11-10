# Making the Race Code Fast on the GBA

Notes on getting `race_logic()` (one tick of the race) to fit the GBA's time budget, measured with `gba/replay` in mGBA over one whole recorded race (10,821 frames, red desert track, the AI driving all four cars).

## Budget

The game runs its logic 70 times a second. At 60 frames per second a frame is 280,896 cycles and holds 1.17 ticks, so one tick may take at most 240,000 cycles at 100% CPU. With room left for drawing (the rendering prototype uses about 12% of a frame), the target is about **200,000 cycles a tick**.

## Where It Stands

| Step | Cycles a tick (average) | Exact? |
|---|---:|---|
| Original code | 3,860,000 | |
| `sin()`/`cos()` from a table | 2,040,000 | new math |
| libgcc soft-float and division in IWRAM | 1,060,000 | yes |
| Wall map (skip wall tests where there are no walls) | 834,000 | yes |
| Trig in degrees, the car's index read once | 672,000 | new math |
| Physics (`4c434h`) in single precision, 20 sines from 3 | 537,000 | new math |
| Car-to-car overlap searched from the end, `DR_INLINE_MATH` | 458,000 | yes |
| AI (`4b62ch`) in single precision, controls slot read once | 419,000 | yes, the hashes didn't change |
| Obstacle blink windows in closed form (`5209ch`) | 376,000 | yes |
| Skid marks (`54668h`): wheels from 2 sines, rounding in float | 343,000 | new math (only skid pixels moved) |
| Wall search by wall-map cells, `getCounter()` inline, `-mlong-calls` | 272,000 | yes |
| Spectators/pickups: the car's position converted once | 252,000 | yes |
| Machine guns/AI: the cars' positions converted once | 248,000 | yes |
| Hot loops as ARM code in IWRAM (`DR_HOT`), obstacles in EWRAM (`DR_COLD`) | 222,000 | yes |
| Structs packed to 2 (cars) and 4 (the rest) instead of 1, same layouts | 204,500 | yes |
| Spectators not packed | 198,800 | yes |
| Machine-gun trace as ARM in IWRAM | 196,800 | yes |
| Car collisions inside the images' boxes, skid marks per wheel, bitwise rounding | 192,900 | yes |
| `D()` loads whole words in the logic's files (`DR_ALIGNED`, checked) | 182,200 | yes |
| `rand() % n` and `/0x640` by multiply and shift (Thumb has no division) | 178,900 | yes |
| `/1.15` and `/1.2` in double by a multiply, dividing only near a float's rounding point | 177,300 | yes |
| Spectators, obstacles and tire dust as ARM in IWRAM, `sqrt()` in IWRAM, newlib's file handles and the car boxes out of it | **167,500** | yes |

The busiest 1,000 frames (the start, with all cars close together) take 180,000 cycles a tick. `gba/replay` lists the five slowest frames, and `PROFILE=1` gives each function's slowest call.

"Exact" means the recording replays with the same hashes. A "new math" step needs a new recording from `tools/replay TICKLOG OUT` and a check that the race still comes out the same. The race-outcome check is the finish frame of each car, printed by `tools/replay`. Any rounding change, even one ulp in one constant, moves the finish frames by about ±50 frames. All the new-math steps stayed inside that range.

## Lessons

- **Bugs in rewritten math can hide in the outcome but not in the numbers.** `(154 << 24)/360` overflowed an int, which put two of the car's corner points at the wrong angle. The race still worked, but every car finished about 450 frames later. A finish-frame check caught it; checking each rewritten formula numerically against the original found the cause.
- **The desktop has to round floats like the GBA.** The desktop builds use `-ffp-contract=off`, so that a CPU with FMA doesn't fuse `a*b+c` into one rounding. Single-precision float on x86-64 (SSE) and ARM soft-float then agree bit for bit.
- **ROM is slow for ARM code.** Thumb code calling libgcc's ARM routines in IWRAM goes through veneers that run as ARM from ROM (about 10% of the logic). `-mlong-calls` replaces them with a `bx` in the caller.
- **Globals get reloaded after every byte store**, because a store through `__BYTE__ *` may alias anything. Loops that draw into the track image need their pointers in locals.
- **`(int)(double)f` is `(int)f`**, and `(f - (int)f) < 0.5` is exact in float. Many double operations could go without changing a bit.
- **Look at the slowest calls, not only the averages.** The machine guns averaged 13,000 cycles per car but took up to 50,000, which made 291 frames take longer than a GBA frame.
- The hash leaves out `TRX_IMA` (the track image), so `tools/replay` prints a hash of it at the end. Drawing changes need that hash to stay the same.
- **Thumb code on the ARM7 can't divide, not even by a constant**: it has no long multiply, so `x/12` calls a library routine. For values known to be small, like `rand_watcom106()`'s 0-32767, `dRMath_div15()` multiplies and shifts instead, checked against `/` for every value.
- **An exact shortcut can be checked exhaustively.** `dRMath_divdf()` replaces `(float)((double)f/1.15)` with a multiply, and divides only where that could round differently. It was compared with the division for every one of the 2^32 float bit patterns.
- **Word loads need aligned addresses, and nothing says so.** On the ARM7 an unaligned `ldr` reads rotated bytes without an error. `DR_ALIGNED` applies only to the files listed in `src/race/aligned.mk`. Drawing code writes 4 pixels at any x, so it stays with byte loads. `make ALIGNED_CHECK=1` (desktop) and `ALIGNED_CHECK=1` (GBA builds) count every unaligned access in those files, and `race_replay_start()` refuses globals that aren't word-aligned.

## The Real 70 Ticks a Second in `gba/race`

`gba/race` now runs at the game's real speed at 60 frames per second. Measured with its benchmark build (`RACE_CFLAGS="-DRACE_BENCH -DRACE_SEED=1"`: the AI drives all four cars, one tick a frame, and a hash of the race at its end), on TR8:

| Step | Cycles a tick (average) | Slowest tick | Ticks over 225,000 | Race hash |
|---|---:|---:|---:|---|
| Before | 181,700 | 343,700 | 246 | same |
| ROM at 3/1 wait states instead of 3/2 (`WAITCNT` 0x4017) | 152,000 | 308,600 | 59 | same |
| The car's wall test (`4c434h`'s `helper02`) only between each row's first and last solid pixel | 148,500 | 281,700 | 28 | same |
| ...and 4 mask pixels at a time, from the first block with walls to the last | 146,100 | 250,100 | 11 | same |

The race hash stayed the same on TR0, TR1, TR3, TR5, TR8 and TR9.

- **Wait states.** Most commercial cartridges run ROM at 3/1. The race code is Thumb code read from ROM, mostly in sequence, so the sequential wait state counts for nearly everything. This is the one step here that depends on the cartridge, so it's the first thing to check on real hardware.
- **The slowest ticks come from the cars getting unstuck from walls.** When a car hits a wall, `4c434h` moves it by random steps until its 40x40 pixels are clear, and it searches the pixels against the track's mask for every step. Each row of each car image now has its first and last solid column, and a flag for whether everything in between is solid. A full row isn't read from the image at all. Blocks of the wall map without walls have no wall pixels, so a row's mask is searched from the first block with walls to the last, 4 pixels to a word load. The first hit is still the same pixel.
- **The car shapes are baked.** `race_car_shapes()` (`src/race/race_car_shapes.c`) builds the boxes and the row spans of a car's 96 images. `tools/gbabake` writes them for every car image to `cars.shp`, and the desktop builds them at setup. Setting a race up takes about 30 frames instead of 60-80, because the GBA no longer reads all of the car images to build them.
- **Ticks, not frames.** At 70 ticks a second, every sixth frame has two. When `race_logic()` ran both in one call, that frame took about 330,000 cycles and the screen dropped to 50 frames per second. `gba/race` now calls `race_logic()` and `race_render()` once for each tick, as the original did on its 70 Hz VGA screen. The vblank interrupt draws the last finished tick: a snapshot of the camera, the cars' places and frames, and the HUD. So a tick that runs into the next frame still gets drawn, as long as some tick finishes in every frame. On average 7 ticks take about 1,230,000 of a 6-frame 1,685,000 cycles.

With the AI driving, whole races on all ten tracks run at 60.0 frames per second. At most two seconds a race show 59. With the over-track pictures as sprites, all ten tracks still show 60.0 over 140 seconds of racing.

## Drawing Into the Track

Skid marks, mines, craters, power-ups and obstacles go into the track image through `src/race/race_ima.c`. On the GBA that's tiles copied to EWRAM as they're drawn into (`DR_IMA_TILES`). On TR8:

| | Cycles a tick (average) | Slowest tick |
|---|---:|---:|
| Drawing nowhere, as before | 143,100 | 249,700 |
| Through the tiles, a span at a time in Thumb | 159,500 | 281,400 |
| A tile at a time, as ARM in IWRAM, nothing copied or reloaded where no pixel changes | 152,300 | 255,300 |

Obstacles cost the most. While they blink, they're drawn and put back 16 rows at a time on every tick. Writes that change nothing now cost a comparison and no more. That also leaves about 30 tiles a race out of the pool.

To keep the stack's margin with the new IWRAM code, newlib's malloc state, `_impure_data` and locale (`gba/game.mk`) and the 768-byte palette `___1a51d0h` (`DR_COLD`) moved to EWRAM. A race leaves about 1.6 KB of the stack untouched.

## Pictures Over the Track

The spectators, wrecks, explosions, shots and start lights are hardware sprites, collected through `race_sprite()` while `race_render()` runs. Their tiles are copied to the sprite memory only when a picture changes. The list of sprites and the vblank snapshot moved to EWRAM, which leaves 1.4 KB of the stack untouched on all ten tracks.

With the frame counter now counting ticks, the races come out differently, so the logic's cost isn't comparable to the table above. With seed 1, races on the ten tracks average 137,800 to 148,200 cycles a tick, 147,400 on TR8. The desktop, `replay-tiles` and the GBA give the same globals and track-image hashes on all ten.

## Next Steps, Most Promising First

Items 1-4 below are done, and the race runs at full speed. The rest would make room for what the race doesn't do yet (skid marks, explosions, sound).

1. **(Done) Byte-wise struct access.** `struct_35e_t` (the cars) is `#pragma pack(push,1)` in `include/drally_structs_free.h`. Several race files also put `#pragma pack(1)` before their own structs, for example `spectator_t` in `race___52ac4h.c`, and without a pop it applies to the rest of the file. GCC therefore reads every 32-bit car field as 4 byte loads plus 3 shifts and 3 ORs. Checked with the ARM compiler:
   - `pack(2)` gives exactly the same layout as `pack(1)` for all five structs in that header.
   - `pack(4)` (natural) gives the same layout for every struct except `struct_35e_t`. Its `Finished` at 0x10a and its size 0x35e need 2-byte alignment.

   So: `pack(push,2)` for `struct_35e_t`, natural packing for the others, and `spectator_t` with no packing (all dwords). This likely saves a lot across all the race code and stays bit-exact. The replay hashes check it, because raw-offset `D()` accesses would break if the layout moved.
2. **(Done) The `D()`/`F32()` macros** (`include/drally.h`) read through a packed `may_alias` struct, which means byte loads on the GBA. For globals known to be 4-aligned (most of the `___xxxxxxh` arrays), a word-access variant would be much cheaper. Do the hottest files first.
3. **(Done for `4dcach`) The double divisions that are left** (`__divdf3`, about 3% of the logic): `race___4dcach` divides by 1.15 and 1.2 three times per car, and there are others. Each costs hundreds of cycles. Replacing them with float, or with multiplying by the reciprocal, is a new-math step.
4. **(Done) `sqrtf`** (newlib, Thumb in ROM, about 2%). Copy `libm`'s `ef_sqrt` object into IWRAM the way `gba/replay/Makefile` does for libgcc.
5. **More `DR_HOT`:** the stack, which the vblank interrupt adds to, has about 1.6 KB that a race never touches. Moving more cold data to EWRAM would make room, for example newlib's `__sf` (360 bytes) and the race's `TRX_OHI_DAT` (1 KB). Candidates are `race___55ae0h`, the skid-mark helpers in `race___54668h`, and `race___4b62ch`'s inner loops. `race___4c434h` is 11 KB of Thumb code, too big to move whole.
6. The rest of the AI and physics float math into fixed point, if the steps above don't reach the target.

## How to Measure

```sh
S=/path/to/scratch
cd tools/replay && make && ./replay $S/tl.bin [$S/tl-new.bin]   # "all hashes match", or write new hashes
make -C gba/replay TICKLOG=$S/tl-new.bin [PROFILE=1|2]
ojgbarun gba/replay/drreplay.gba 200000 | grep -v "Bad memory"   # "all hashes match the desktop", cycles a tick
ojgbarun gba/replay/drreplay.gba 6000 sample=10:997c:samples.txt # PC samples, map them with nm
```

`PROFILE=1` times each function. `PROFILE=2` also counts soft-float calls, but it slows everything down. For PC sampling, keep the frame count small: sampling every 200 cycles for the whole run wrote 280 million lines.
