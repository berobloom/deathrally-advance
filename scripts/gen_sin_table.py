#!/usr/bin/env python3
# Writes src/core/drmath_sin.h: sin() over one turn in 4096 steps, plus one
# so that interpolation never wraps, as 4.28 fixed point.
import math

N = 4096
vals = [round(math.sin(2*math.pi*i/N)*(1 << 28)) for i in range(N+1)]
with open("src/core/drmath_sin.h", "w") as f:
    f.write("// sin() over one turn in %d steps, 4.28 fixed point. Made by scripts/gen_sin_table.py\n" % N)
    f.write("static const int dRMath_sin_tab[%d] = {\n" % (N+1))
    for i in range(0, N+1, 8):
        f.write("\t" + " ".join("%d," % v for v in vals[i:i+8]) + "\n")
    f.write("};\n")
