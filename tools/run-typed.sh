#!/usr/bin/env sh
set -eu

repository=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
build_directory=${1:-"$repository/build-selected"}
iterations=${2:-100000}
temporary_directory=$(mktemp -d "${TMPDIR:-/tmp}/nativejson-typed.XXXXXX")
trap 'rm -rf "$temporary_directory"' EXIT HUP INT TERM

printf '%s\n' 'library,mapping,iterations,parse_us,peak_bytes,alloc_count,checksum,raw_bytes,stripped_bytes'
for executable in "$build_directory"/bin/typed/typed_*; do
    result=$($executable "$iterations" | tail -n 1)
    copied="$temporary_directory/$(basename "$executable")"
    cp "$executable" "$copied"
    strip -x "$copied"
    raw_bytes=$(wc -c < "$executable" | tr -d ' ')
    stripped_bytes=$(wc -c < "$copied" | tr -d ' ')
    printf '%s,%s,%s\n' "$result" "$raw_bytes" "$stripped_bytes"
done
