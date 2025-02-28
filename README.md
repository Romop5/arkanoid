# Arkanoid

## How to compile (Win32)

Create build folder with generated 3rd party import scripts
> conan install -if build .

Generate solution
> cd build && cmake .. -DCMAKE_TOOLCHAIN_FILE="generators/conan_toolchain.cmake"

Open akranoid.sln and compile