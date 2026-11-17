# CLAUDE.md

## Purpose

A small desktop *app that loads 3D model files and estimates their volume with
the Monte Carlo method. It samples random points inside the mesh's bounding box,
classifies each point as inside or outside by ray-casting (Möller–Trumbore,
odd/even hit count), and scales the box volume by the fraction of inside points.
The Qt Quick 3D UI shows the mesh, the classified points and the computed volume.

## Stack

- **C++23** (GCC 16 is the reference compiler), CMake ≥ 3.30
- **Qt 6.9+**: Core, Quick, Quick3D, QuickControls2, Concurrent (UI only)
- **CUDA** (optional, CUDA 13, C++20 for device code, default arch `sm_89`)
- Other dependencies: `fmt`, `assimp` (mesh loading), `tbb` (backs
  `std::execution::par_unseq`)
- [`aminya/project_options`](https://github.com/aminya/project_options) v0.44.0
  is a **git submodule** in `./project_options/` (`git submodule update --init`).
  It supplies warnings, IPO, `-march=native`, clang-tidy and the `target_link_cuda` macro.
  Sanitizers are **not** taken from project_options, see `cmake/Sanitizers.cmake`.

## Layout

- `cmake/` holds project CMake modules (`Sanitizers.cmake`).
- `tests/` holds the GoogleTest tests for `ApproximationLib`, with mesh and seeded-point helpers in `TestHelpers.hpp`.
- `ApproximationLib/` holds the shared library with all the computation and no Qt dependency.
  - `IVolumeApproximation.hpp` is the backend interface: `IsAvailable()` + `ComputeVolume()`.
  - `ApproximationHelper.{hpp,cpp}` holds the backend list (`ApproximationExecutor` enum,
    `ExecutorNames`) and the factory (`CreateApproximationStrategy`,
    `CreateDefaultStrategy`).
  - `CpuParallelVolumeApproximation`: the STL parallel-algorithms backend (always built).
  - `CudaVolumeApproximation` (`.cu`): the Thrust + kernel backend (only built with CUDA).
  - `PointCalculations.hpp` / `Vector3.hpp` hold the math shared by host and device
    (`__host__ __device__` via the `FUNCTION_ATTRIBUTES` / `VECTOR_ATTRIBUTES`
    macros). They live in headers on purpose so the CPU build never needs nvcc.
- `UserInterface/` is the Qt executable `VolumeApproximation`.
  - `SceneController` is the QML-facing controller (engine list, load and calculate
    slots). `ComputeRunnable` runs a computation on `QThreadPool`.
  - `CustomGeometry` loads all meshes of a file via assimp into a `QQuick3DGeometry`.
  - `PointsInstancing` renders the sampled points. `ui/MainWindow.qml` is the UI.
  - `SceneController.hpp` must stay in `UserInterface/` itself, not `include/`,
    because Qt's QML codegen includes it by bare name.

## Build

All supported configurations are defined once, in `CMakePresets.json`. Each builds into
`build/<preset>/`:
`cpu-debug`, `cpu-release`, `cuda-debug`, `cuda-tsan`, `cuda-relwithdebinfo`
(`cmake --list-presets`). The CPU-only presets must always work.

```bash
cmake --preset cpu-debug && cmake --build --preset cpu-debug && ctest --preset cpu-debug
./build/cpu-debug/UserInterface/VolumeApproximation
```

- `CUDA_ENABLED` (root `option()`, default `OFF`) adds the `.cu` source,
  calls `target_link_cuda` and defines `CUDA_ENABLED` **publicly** on
  `ApproximationLib`, so the UI sees the same value. The GPU arch defaults to
  `89` and can be overridden with `-DCMAKE_CUDA_ARCHITECTURES=<arch|native>`.
- **Debug builds use sanitizers.** `DEBUG_SANITIZERS` defaults to
  `address;undefined` and is only applied to the `Debug` config.
  ASan and TSan can't be combined in one binary, so use a separate build dir for TSan:
  `-DCMAKE_BUILD_TYPE=Debug -DDEBUG_SANITIZERS="thread;undefined"`. Set it to `""` to
  disable sanitizers. The flags come from the `VolumeApproximation_sanitizers`
  INTERFACE target, which forwards them to nvcc's host compiler via `-Xcompiler`.
  Every target links it PRIVATE.
- Project flag targets (`VolumeApproximation_project_options` / `_warnings` /
  `_sanitizers`) are linked PRIVATE. Each target links them itself. Don't make them PUBLIC.
- `FEATURE_CHECKS` (default `ON`) runs clang-tidy when the compiler is Clang.
  Pass `-DFEATURE_CHECKS=OFF` for faster iteration.
- **Tests:** GoogleTest in `tests/` (target `ApproximationLibTests`), registered with CTest
  (`include(CTest)`, `BUILD_TESTING`, default ON; skipped with a warning if GTest is missing).
  They cover Vector3 math, ray intersection, point classification, CPU backend volumes
  (cube, tetrahedron) and the factory fallback. `CudaBackendTests.cpp` is compiled only with
  CUDA and `GTEST_SKIP`s without a usable GPU. Points are seeded, so results are
  deterministic. Backends report volume ×1e6 (`TestHelpers::VolumeScale`).
- To verify a change, run `ctest --preset` for a CPU and a CUDA preset. For UI changes, also run
  the app, load a mesh and calculate on each listed engine.
- `UserInterface/SanitizerDefaults.cpp` bakes in sanitizer runtime defaults. It is compiled into
  both the app and `ApproximationLibTests`:
  - ASan + CUDA needs `protect_shadow_gap=0`, otherwise `cudaGetDeviceCount` fails
    (out-of-memory / no device) and CUDA is reported unavailable.
  - ASan + CUDA suppresses LeakSanitizer reports from the NVIDIA driver (`leak:libcuda.so`).
    `cuInit` never frees its state, which would fail every test that touched CUDA at exit.
  - TSan suppresses races inside uninstrumented Qt and oneTBB (`race:libQt6*`, `race:libtbb.so`).
    TBB runs `std::execution::par_unseq`. This also hides real races whose stack passes through
    these libraries, such as inside a parallel-algorithm lambda.
  - `ASAN_OPTIONS` / `TSAN_OPTIONS` env vars still override these.

## Hard requirements

1. **CUDA is optional.** The whole project must configure, build and run with
   `CUDA_ENABLED=OFF` on a machine with no CUDA toolkit or NVIDIA GPU.
   - Every CUDA include, type and backend reference outside `.cu` files goes
     behind `#ifdef CUDA_ENABLED` (see `ApproximationHelper.hpp/.cpp`).
   - Don't put CUDA-only code in `.cpp` files, and don't add `.cu` files to
     `SOURCE_FILES`. CMake picks the compiler by extension. CUDA sources belong
     in `CUDA_SOURCES` inside the `if(CUDA_ENABLED)` block.
   - Code shared between host and device goes in headers, using the attribute macros.
   - Don't add `CUDA` to `project(... LANGUAGES ...)`. `target_link_cuda` enables
     the language only when CUDA is on.
2. **Detect CUDA at runtime.** A build with CUDA compiled in must still run
   on a machine without a usable GPU or driver. It falls back to the CPU backend
   instead of failing.
   - `IVolumeApproximation::IsAvailable()` is the runtime check. For CUDA it
     uses `cudaGetDeviceCount` and must never throw or terminate.
   - `SceneController`'s constructor probes every compiled-in backend and only
     lists, and only defaults to, the ones whose `IsAvailable()` returns true.
     `CreateDefaultStrategy()` likewise falls back to the CPU backend.
     Keep both behaviours when changing backends.
   - `ExecutorNames` is ordered by preference: the first available entry
     becomes the default, so keep the fastest backend first.
3. **Several computation paths.** Every backend implements `IVolumeApproximation`
   and must return the same `ApproximationResult` semantics (points, volume,
   extents). Results from different backends should agree within Monte Carlo noise.

## Adding a backend

Touch all of these (see `IMPROVEMENTS.md` for the planned consolidation):
the enum + `EnumToString` + `ExecutorNames` in `ApproximationHelper.hpp`, the
factory switch in `ApproximationHelper.cpp`, the CMake source lists, and guard
macros if the backend is optional. `SceneController` keeps the available engines in one
`std::vector<ExecutorNamePair>` (`m_AvailableEngines`), and the QML name list is
built from it. The QML combo-box index points into that vector, so reorder
the vector itself, never only the displayed names.

## Claude Code hooks (`.claude/settings.json`)

- `.claude/hooks/format.sh` (PostToolUse on Edit/Write) runs `clang-format -i` on touched
  `.cpp/.hpp/.h/.cu/.cuh` files, skipping `project_options/` and `build*/`.
- `.claude/hooks/build-check.sh` (Stop) builds the CPU-only config in `build-hook/`
  (Debug, no sanitizers) when C++/CMake/QML files have uncommitted changes. A broken
  build blocks finishing and returns the compiler errors. It blocks at most once per stop
  (`stop_hook_active`).

## Claude Code subagents (`.claude/agents/`)

- `build-matrix` builds, tests (ctest) and smoke-runs every preset in `build/<preset>/` and reports a
  table. Use it to verify cross-cutting or CMake changes, instead of running the matrix inline.
- `reviewer` is a read-only, independent review of a diff against the hard requirements above.

## MCP server (`tools/mcp-server/`, registered in `.mcp.json`)

- `volume-approximation` (Python, official `mcp` 2.x SDK, run with `uv`) exposes the presets as tools:
  - `list_configurations`, `configure`, `build`, `get_diagnostics`
  - `list_tests`, `run_tests` (builds first)
  - `smoke_run`, `environment`
  
  Full logs are the `build://{preset}/build-log|test-log|smoke-log` resources.
- When it's connected, prefer `run_tests` / `build` over raw cmake/ctest shell commands: they return
  parsed diagnostics and test failures instead of long logs.
- It only accepts presets from `CMakePresets.json` (no free-form commands). Adding a configuration
  means adding a preset, nothing else.
- Its own tests: `cd tools/mcp-server && uv run pytest`. The end-to-end run over stdio is `uv run python scripts/e2e.py [preset]`.

## Conventions

- Formatting: `.clang-format` (tabs, Allman braces). The format hook applies it automatically.
- Naming (`.clang-tidy`): types, functions and methods `CamelCase`; locals and
  parameters `camelBack`; private/protected members `m_CamelCase`; constants
  `CamelCase`. Qt/QML-exposed members use Qt's `camelBack` naming inside
  `NOLINTBEGIN/END(readability-identifier-naming)`.
- Library code lives in `namespace VolumeApproximation`, with backends in `::Impl`.
  Export public types with `APPROXIMATIONLIB_EXPORT`.
- Report errors with exceptions (`std::runtime_error`), never `std::terminate`.
  The CUDA path wraps calls in `CHECK_CUDA_ERRORS`.
- Mark everything with `[[nodiscard]]`, `noexcept` and `const` where it applies,
  matching the existing code.
- `IMPROVEMENTS.md` tracks known bugs and planned work. Tick items there when done.
