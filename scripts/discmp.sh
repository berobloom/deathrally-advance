#!/bin/sh
# discmp.sh [REV] [FLAGS]: build REV (default HEAD) and the working tree with
# the same flags and list the object files whose disassembly differs.
# Use it to check that a cleanup leaves the generated code alone.
set -e
cd "$(dirname "$0")/.."
REV=${1:-HEAD}
FLAGS=${2:-"-O2 -Wno-unused-result"}
TMP=$(mktemp -d)
trap 'git worktree remove --force "$TMP/old" >/dev/null 2>&1; rm -rf "$TMP"' EXIT

git worktree add -q "$TMP/old" "$REV"
make -C "$TMP/old" -j"$(nproc)" BUILD="$TMP/obj-old" TARGET="$TMP/bin-old" FLAGS="$FLAGS" >/dev/null
make -j"$(nproc)" BUILD="$TMP/obj-new" TARGET="$TMP/bin-new" FLAGS="$FLAGS" >/dev/null

dis() {
	objdump -dr --no-show-raw-insn "$1" | sed -E 's/^ *[0-9a-f]+:[[:space:]]*//; s/<[^>]*>//g; s/#.*$//' | grep -v -e '^$' -e 'file format' || true
}

same=0
for o in $(cd "$TMP/obj-new" && find . -name '*.o' | sort); do
	[ -f "$TMP/obj-old/$o" ] && dis "$TMP/obj-old/$o" > "$TMP/old.s" || : > "$TMP/old.s"
	dis "$TMP/obj-new/$o" > "$TMP/new.s"
	if [ -f "$TMP/obj-old/$o" ] && cmp -s "$TMP/old.s" "$TMP/new.s"; then
		same=$((same + 1))
	else
		echo "differs: ${o#./}"
	fi
done
echo "identical: $same"
