# git-chat
A Git-Based Command-Line Messaging Application

## Compiling
This project is made to be built using cmake.

The included CMakeLists file requires cmake >= 2.6.

Build and install:
```bash
mkdir cmake-build-dir
cd cmake-build-dir
# Build makefile
cmake ..
# Run build
make
# Run install
sudo make install
```

Due to the version number being injected from CMake, compiling from gcc won't have access to the templated version.h file.
