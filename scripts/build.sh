#!/bin/sh
set -e

cd "$(dirname "$0")/.."

BUILD_TYPE="Debug"
if [ "${1:-}" = "--release" ]; then
  BUILD_TYPE="Release"
fi

cmake -S . -B build \
  -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

cmake --build build --target VulkanEngine --parallel
