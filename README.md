# Volume Approximation

C++ project made of library and visualization in Qt6 allowing that allows loading 3d mesh files and approximating their volume using Monte Carlo method. Project generally requires C++23.

 - ApproximationLib - Library which does the approximation itself, currently the only working implementation is using parallel algorithms from STL. There is also a CUDA implementation.
 - UserInterface - Ui layer written using Qt6 to allow loading mesh file and visualizing how it looks.

## Building

Requires CMake 3.30+, a C++23 compiler, Qt 6.9+, fmt, assimp and oneTBB. CUDA 13 is optional.

```bash
git clone --recurse-submodules <repo>   # or: git submodule update --init
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo   # add -DCUDA_ENABLED=ON for the GPU backend
cmake --build build
```

Debug builds enable AddressSanitizer + UndefinedBehaviorSanitizer. For ThreadSanitizer, use a separate
build directory with `-DCMAKE_BUILD_TYPE=Debug -DDEBUG_SANITIZERS="thread;undefined"`.