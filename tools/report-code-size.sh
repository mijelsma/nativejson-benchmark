#!/usr/bin/env sh
set -eu

repository=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
build_directory=${1:-"$repository/build-selected"}
temporary_directory=$(mktemp -d "${TMPDIR:-/tmp}/nativejson-size.XXXXXX")
trap 'rm -rf "$temporary_directory"' EXIT HUP INT TERM

printf '%-44s %12s %12s\n' adapter raw_bytes stripped_bytes
for executable in "$build_directory"/bin/jsonstat/jsonstat_*_release_x64_cmake; do
    name=$(basename "$executable")
    stripped="$temporary_directory/$name"
    cp "$executable" "$stripped"
    strip -x "$stripped"
    raw_bytes=$(wc -c < "$executable" | tr -d ' ')
    stripped_bytes=$(wc -c < "$stripped" | tr -d ' ')
    printf '%-44s %12s %12s\n' "$name" "$raw_bytes" "$stripped_bytes"
done
