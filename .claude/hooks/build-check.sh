#!/usr/bin/env bash
# Stop hook: before Claude finishes a turn, make sure the CPU-only build still compiles
# (hard requirement: the project must always build with CUDA_ENABLED=OFF).
#
# Exit 0 -> fine, Claude may stop.
# Exit 2 -> build broken; stderr (the compiler errors) is sent back to Claude, which keeps working.

input=$(cat)

# Already continuing because of this hook once: don't loop forever on an error Claude can't fix
[[ $(jq -r '.stop_hook_active // false' <<<"$input") == "true" ]] && exit 0

cd "$CLAUDE_PROJECT_DIR" || exit 0

# Nothing build-relevant changed since the last commit -> skip
if [[ -z $(git status --porcelain -- '*.cpp' '*.hpp' '*.h' '*.cu' '*.cuh' '*.qml' '*CMakeLists.txt' '*.cmake' 2>/dev/null) ]]; then
  exit 0
fi

build_dir=build-hook # gitignored by the *build-* pattern

if [[ ! -f $build_dir/build.ninja ]]; then
  # Debug without sanitizers: fastest config to compile
  if ! out=$(cmake -S . -B "$build_dir" -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCUDA_ENABLED=OFF \
      -DDEBUG_SANITIZERS= 2>&1); then
    { echo "CPU-only CMake configure failed:"; tail -n 30 <<<"$out"; } >&2
    exit 2
  fi
fi

if ! out=$(cmake --build "$build_dir" 2>&1); then
  {
    echo "The CPU-only build (CUDA_ENABLED=OFF, $build_dir/) is broken. Fix it before finishing:"
    # Keep the diagnostics, drop the (very long) compiler command lines
    grep -E '^FAILED|error|undefined reference' -A3 <<<"$out" \
      | grep -vE '^[^ ]*(c\+\+|g\+\+|clang\+\+|nvcc) ' | head -n 60
  } >&2
  exit 2
fi

exit 0
