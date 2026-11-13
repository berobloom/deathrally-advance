#!/usr/bin/env python3
# profile.py: names the functions in gbarun -p's output and prints, for
# the slow ticks and the others, the cycles a tick each part of the
# benchmark takes (the race's logic, the interrupt's drawing and sound,
# the rest), then for each part the functions that the slow ticks spend
# the most more on.
#
#   tools/gbarun/profile.py PROFILE ELF [N]
#
# The addresses are where mGBA's PC was, which runs 8 bytes ahead in ARM
# code and 4 in Thumb: a function's first instructions can count to the
# one before it.

import bisect
import os
import subprocess
import sys

NM = os.environ.get('NM', '/opt/devkitpro/devkitARM/bin/arm-none-eabi-nm')
PHASES = ['rest', 'logic', 'drawing', 'sound']


def symbols(elf):
	out = subprocess.run([NM, '-n', '--defined-only', elf], capture_output=True, text=True, check=True).stdout
	addrs, names = [], []
	for line in out.splitlines():
		parts = line.split()
		if len(parts) != 3 or parts[1] not in 'tTwW':
			continue
		addrs.append(int(parts[0], 16) & ~1)
		names.append(parts[2])
	return addrs, names


def main():
	if len(sys.argv) < 3:
		sys.exit('usage: profile.py PROFILE ELF [N]')
	top = int(sys.argv[3]) if len(sys.argv) > 3 else 15
	addrs, names = symbols(sys.argv[2])
	# cycles[kind][phase][function]
	cycles = {k: [dict() for _ in PHASES] for k in ('slow', 'other')}
	with open(sys.argv[1]) as f:
		_, n_slow, n_other = f.readline().split()
		n = {'slow': int(n_slow), 'other': int(n_other)}
		for line in f:
			kind, phase, addr, c = line.split()
			a = int(addr, 16)
			if a == 0:
				name = '(elsewhere)'
			elif a == 2:
				name = '(BIOS)'
			else:
				# PC is ahead of the instruction: look 4 bytes back
				i = bisect.bisect_right(addrs, a-4)-1
				name = names[i] if i >= 0 else '(?)'
			d = cycles[kind][int(phase)]
			d[name] = d.get(name, 0)+int(c)

	def per(kind, p):
		return {m: c/max(n[kind], 1) for m, c in cycles[kind][p].items()}

	print(f"cycles a tick: {n['slow']} slow ticks, {n['other']} others")
	print(f"{'':12s} {'slow':>9s} {'other':>9s} {'more':>9s}")
	for p, name in enumerate(PHASES):
		s, o = sum(per('slow', p).values()), sum(per('other', p).values())
		print(f"{name:12s} {s:9.0f} {o:9.0f} {s-o:+9.0f}")
	for p, name in enumerate(PHASES):
		s, o = per('slow', p), per('other', p)
		print(f"\n{name}:")
		print(f"  {'function':40s} {'slow':>9s} {'other':>9s} {'more':>9s}")
		rows = sorted(set(s) | set(o), key=lambda m: s.get(m, 0)-o.get(m, 0), reverse=True)
		for m in rows[:top]:
			print(f"  {m[:40]:40s} {s.get(m, 0):9.0f} {o.get(m, 0):9.0f} {s.get(m, 0)-o.get(m, 0):+9.0f}")


if __name__ == '__main__':
	main()
