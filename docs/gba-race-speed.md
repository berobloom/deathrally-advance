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
| Car collisions inside the images' boxes, skid marks per wheel, bitwise rounding | **192,900** | yes |

The busiest 1,000 frames (the start, with all cars close together) take 211,000 cycles a tick. Only one frame that runs a single tick takes longer than a GBA frame (frame 475, 299,000 cycles, near the start). One more frame runs two ticks and takes 349,000 cycles. `gba/replay` lists the five slowest frames, and `PROFILE=1` gives each function's slowest call.

"Exact" means the recording replays with the same hashes. A "new math" step needs a new recording from `tools/replay TICKLOG OUT` and a check that the race still comes out the same. The race-outcome check is the finish frame of each car, printed by `tools/replay`. Any rounding change, even one ulp in one constant, moves the finish frames by about ±50 frames. All the new-math steps stayed inside that range.

## Lessons

- **Bugs in rewritten math can hide in the outcome but not in the numbers.** `(154 << 24)/360` overflowed an int, which put two of the car's corner points at the wrong angle. The race still worked, but every car finished about 450 frames later. A finish-frame check caught it; checking each rewritten formula numerically against the original found the cause.
- **The desktop has to round floats like the GBA.** The desktop builds use `-ffp-contract=off`, so that a CPU with FMA doesn't fuse `a*b+c` into one rounding. Single-precision float on x86-64 (SSE) and ARM soft-float then agree bit for bit.
- **ROM is slow for ARM code.** Thumb code calling libgcc's ARM routines in IWRAM goes through veneers that run as ARM from ROM (about 10% of the logic). `-mlong-calls` replaces them with a `bx` in the caller.
- **Globals get reloaded after every byte store**, because a store through `__BYTE__ *` may alias anything. Loops that draw into the track image need their pointers in locals.
- **`(int)(double)f` is `(int)f`**, and `(f - (int)f) < 0.5` is exact in float. Many double operations could go without changing a bit.
- **Look at the slowest calls, not only the averages.** The machine guns averaged 13,000 cycles per car but took up to 50,000, which made 291 frames take longer than a GBA frame.
- The hash leaves out `TRX_IMA` (the track image), so `tools/replay` prints a hash of it at the end. Drawing changes need that hash to stay the same.

## Next Steps, Most Promising First

The first item below is done: it took a tick from 222,000 to 199,000 cycles. The logic now fits the budget, so the next milestone is running it in `gba/proto`, in place of the stand-in driving.

1. **(Done) Byte-wise struct access.** `struct_35e_t` (the cars) is `#pragma pack(push,1)` in `include/drally_structs_free.h`. Several race files also put `#pragma pack(1)` before their own structs, for example `spectator_t` in `race___52ac4h.c`, and without a pop it applies to the rest of the file. GCC therefore reads every 32-bit car field as 4 byte loads plus 3 shifts and 3 ORs. Checked with the ARM compiler:
   - `pack(2)` gives exactly the same layout as `pack(1)` for all five structs in that header.
   - `pack(4)` (natural) gives the same layout for every struct except `struct_35e_t`. Its `Finished` at 0x10a and its size 0x35e need 2-byte alignment.

   So: `pack(push,2)` for `struct_35e_t`, natural packing for the others, and `spectator_t` with no packing (all dwords). This likely saves a lot across all the race code and stays bit-exact. The replay hashes check it, because raw-offset `D()` accesses would break if the layout moved.
2. **The `D()`/`F32()` macros** (`include/drally.h`) read through a packed `may_alias` struct, which means byte loads on the GBA. For globals known to be 4-aligned (most of the `___xxxxxxh` arrays), a word-access variant would be much cheaper. Do the hottest files first.
3. **The double divisions that are left** (`__divdf3`, about 3% of the logic): `race___4dcach` divides by 1.15 and 1.2 three times per car, and there are others. Each costs hundreds of cycles. Replacing them with float, or with multiplying by the reciprocal, is a new-math step.
4. **`sqrtf`** (newlib, Thumb in ROM, about 2%). Copy `libm`'s `ef_sqrt` object into IWRAM the way `gba/replay/Makefile` does for libgcc.
5. **More `DR_HOT`:** IWRAM ends at about 27.5 KB of 32 KB, so little is left after the stack. Moving `car_boxes` (1.5 KB) and other cold data to EWRAM would make room. Candidates are `helper_machine_gun`, `race___52ac4h`'s spectator loop, the obstacle and skid-mark draw helpers, and `race___55ae0h`. `race___4c434h` is 11 KB of Thumb code, too big to move whole.
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
