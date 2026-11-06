#!/usr/bin/env sh
set -eu

repository=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
build_directory=${1:-"$repository/build-jeson-embedded-size"}
jeson_include=${JESON_INCLUDE_DIR:-"$repository/thirdparty/jeson/include"}
temporary_file=$(mktemp "${TMPDIR:-/tmp}/jeson-compact-size.XXXXXX")
trap 'rm -f "$temporary_file"' EXIT HUP INT TERM

cmake -S "$repository" -B "$build_directory" \
    -DCMAKE_BUILD_TYPE=Release \
    -DNATIVEJSON_BUILD_TYPED=OFF \
    -DJESON_INCLUDE_DIR="$jeson_include" \
    -DCMAKE_CXX_FLAGS_RELEASE='-Oz -DNDEBUG -flto=thin -fno-exceptions -fno-rtti' \
    -DCMAKE_EXE_LINKER_FLAGS_RELEASE=-flto=thin
cmake --build "$build_directory" --target jsonstat_compactjesontest --parallel

executable="$build_directory/bin/jsonstat/jsonstat_compactjesontest_release_x64_cmake"
cp "$executable" "$temporary_file"
strip -x "$temporary_file"
printf '%-28s %10s %10s\n' profile raw_bytes stripped_bytes
printf '%-28s %10s %10s\n' jeson-compact-embedded \
    "$(wc -c < "$executable" | tr -d ' ')" \
    "$(wc -c < "$temporary_file" | tr -d ' ')"
