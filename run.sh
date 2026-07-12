#!/usr/bin/env sh

set -eu

cd "$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"

# Remove logbench processes left by an interrupted previous run.
pkill -x logbench 2>/dev/null || true

build_options=""

case "${1-}" in
  "")
    ;;
  --rebuild)
    build_options="--clean-first"
    shift
    ;;
  *)
    echo "Usage: ./run.sh [--rebuild]" >&2
    exit 2
    ;;
esac

if [ "$#" -ne 0 ]; then
  echo "Usage: ./run.sh [--rebuild]" >&2
  exit 2
fi

if [ ! -f build/release/CMakeCache.txt ]; then
  cmake -S . -B build/release -DCMAKE_BUILD_TYPE=Release -DUSE_FMT=ON
fi

# Intentional word splitting: build_options is either empty or --clean-first.
# shellcheck disable=SC2086
cmake --build build/release --config Release --target run $build_options
