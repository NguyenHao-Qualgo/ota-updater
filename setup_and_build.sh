#!/usr/bin/env bash
set -euo pipefail

PROFILE_HOST="aarch64-linux-gnu"
PROFILE_BUILD="default"

BUILD_DIR="build/aarch64"

conan profile detect --force >/dev/null 2>&1 || true

rm -rf "$BUILD_DIR"
mkdir -p "$BUILD_DIR"

conan install . \
  -pr:b "$PROFILE_BUILD" \
  -pr:h "$PROFILE_HOST" \
  -of "$BUILD_DIR" \
  -s:h build_type=Release \
  --build=missing

TOOLCHAIN="$PWD/build/aarch64/build/Release/generators/conan_toolchain.cmake"

cmake -S . -B build/aarch64 \
  -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE="$TOOLCHAIN" \
  -DCMAKE_BUILD_TYPE=Release

cmake --build "$BUILD_DIR" -j
