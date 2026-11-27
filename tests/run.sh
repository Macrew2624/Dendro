#!/bin/sh
# Compare dendro against the real tree on a generated fixture.
BIN="$(pwd)/dendro"
FX="$(mktemp -d)"
trap 'chmod -R u+rwx "$FX" 2>/dev/null; rm -rf "$FX" "$FX.exp" "$FX.act"' EXIT

# --- fixture ---
mkdir -p "$FX/a/b/c" "$FX/empty" "$FX/.hidden/x" "$FX/noperm/inner"
touch "$FX/f0" "$FX/Zed" "$FX/.dotfile" "$FX/a/f1" "$FX/a/b/f2" "$FX/a/b/c/f3"
for i in $(seq 1 40); do touch "$FX/many_$i"; done      # forces array growth
ln -s a "$FX/link_dir"                                  # symlink to a directory
ln -s f0 "$FX/link_file"                                # symlink to a file
ln -s nowhere "$FX/dangling"                            # dangling symlink
chmod 000 "$FX/noperm"                                  # unreadable directory

pass=0
fail=0
check() {
  (cd "$FX" && LC_ALL=C tree --charset=utf-8 $1 .) 2>&1 | sed 's/\xc2\xa0/ /g' > "$FX.exp"
  (cd "$FX" && "$BIN" $1 .)        > "$FX.act" 2>&1
  if diff -u "$FX.exp" "$FX.act" > /dev/null; then
    echo "PASS  dendro $1"
    pass=$((pass + 1))
  else
    echo "FAIL  dendro $1"
    diff -u "$FX.exp" "$FX.act" | head -20
    fail=$((fail + 1))
  fi
}

for flags in "" "-a" "-d" "-L 1" "-L 2" "-a -d" "-a -L 2" "-d -L 2"; do
  check "$flags"
done

echo "$pass passed, $fail failed"
[ "$fail" -eq 0 ]