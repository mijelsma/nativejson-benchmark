#!/usr/bin/env sh
set -eu

repository=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
machine=${1:-local}
build_directory=${NATIVEJSON_BUILD_DIR:-"$repository/build-selected"}
native_arch=${NATIVEJSON_ENABLE_NATIVE_ARCH:-OFF}

git -C "$repository" submodule update --init -- \
    thirdparty/ArduinoJson \
    thirdparty/cJSON \
    thirdparty/jeson \
    thirdparty/nlohmann \
    thirdparty/rapidjson \
    thirdparty/reflect-cpp

cmake -S "$repository" -B "$build_directory" \
    -DCMAKE_BUILD_TYPE=Release \
    -DNATIVEJSON_MACHINE="$machine" \
    -DNATIVEJSON_ENABLE_NATIVE_ARCH="$native_arch"
cmake --build "$build_directory" --parallel

cd "$build_directory/bin"
./nativejson_release_x64_cmake --verify-only
./nativejson_release_x64_cmake --conformance-only
./nativejson_release_x64_cmake --performance-only
