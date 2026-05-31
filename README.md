# Volume Approximation

C++ project made of library and visualization in Qt6 allowing that allows loading 3d mesh files and approximating their volume using Monte Carlo method. Project generally requires C++23.

 - ApproximationLib - Library which does the approximation itself, currently the only working implementation is using parallel algorithms from STL. There is also a CUDA implementation.
 - UserInterface - Ui layer written using Qt6 to allow loading mesh file and visualizing how it looks.