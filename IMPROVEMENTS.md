# VolumeApproximation — Improvements Tracker

## Bug Fixes

- [x] **Volume never shown in UI** — `ApproximationResult::Volume` hardcoded `0.0` in both backends. Computed in `ComputeRunnable::run()`, printed to stdout, never surfaced in QML. Fix: populate result, display in UI.
- [x] **`std::terminate()` in CUDA path** — `CudaVolumeApproximation.cu` calls `std::terminate()` mid-computation. Inconsistent with exception-based error handling elsewhere. Replace with thrown exception.
- [x] **`release()` memory management smell** — `SceneController.cpp` calls `.release()` when converting `unique_ptr` → `shared_ptr`. Use `std::make_shared` or `shared_ptr` constructor directly.

- [x] **CUDA offered without a GPU** — UI listed and defaulted to every compiled-in backend; availability was only checked on Calculate. Now engines are probed with `IsAvailable()` at startup and only usable ones are listed. `CreateDefaultStrategy()` falls back to CPU.
- [x] **Engine index mismatch** — UI sorted engine names but mapped the combo-box index onto unsorted `ExecutorNames`. Now a single `m_AvailableEngines` vector of (executor, name) pairs backs both the QML list and index lookup.

## Code Quality

- [x] **Duplicated Moller-Trumbore** — Algorithm copy-pasted verbatim in `ICpuVolumeApproximationStrategy.cpp` and `CudaVolumeApproximation.cu`. Move to shared header (e.g. `RayIntersection.hpp`) using `VECTOR_ATTRIBUTES` like `Vector3` does.
- [x] **Thread-safety TODO** — `ComputeRunnable::run()` has unresolved `// TODO: is this actually thread safe???` on signal emit. Confirm Qt value-type signal copy semantics are sufficient, remove TODO.
- [x] **Only first mesh loaded** — `CustomGeometry` ignores all meshes except `[0]`. Either merge all or expose selection.
- [x] **README outdated** — Says "CUDA will be added in future". CUDA exists now. Update.
- [x] **Typo in CUDA code** — `"avaliable"` → `"available"` in `CudaVolumeApproximation.cu`.

## New Features

- [ ] **Progress reporting** — Long computation freezes UI. Add progress signal from `ComputeRunnable`, display in QML.
- [ ] **Convergence display** — Show running volume estimate as points accumulate. Plot estimate vs. sample count.
- [x] **Multiple mesh support** — Load multi-part models, compute per-part or aggregate volume.
- [ ] **Export results** — Save sample count, computed volume, mesh filename, backend used, timestamp → CSV/JSON.
- [ ] **Adaptive sampling** — Stop when variance drops below user-defined threshold instead of fixed N. More principled than increasing point count manually.
- [ ] **On-device random generation (cuRAND)** — Current CUDA path generates random points on host, transfers to device. Use cuRAND to generate on-device directly — eliminates host→device transfer, major perf win.

## Architecture

- [ ] **Thin `ICpuVolumeApproximation` class** — Only adds `IsAvailable() = true`. Consider removing if hierarchy stays flat.
- [ ] **Backend registration pattern** — Adding new backends requires touching enum, names array, factory, and CMake separately. Consolidate backend list in one place or use a registration pattern.
