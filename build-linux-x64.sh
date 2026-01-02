#!/bin/bash

# Replace the HOST value below with the actual depends host dir you built (e.g. x86_64-pc-linux-gnu)
# export CC=clang-22 CXX=clang++-22 
export CC=gcc-11
export CXX=g++-11
export HOST=x86_64-pc-linux-gnu
export DEPENDS_DIR="$(pwd)/depends/$HOST"

# Make the depends-provided tools and pkg-config visible
export PATH="$DEPENDS_DIR/bin:$PATH"
export PKG_CONFIG_PATH="$DEPENDS_DIR/lib/pkgconfig:$PKG_CONFIG_PATH"

# Tell the compiler/linker where to find includes and libs
export CPPFLAGS="-I$DEPENDS_DIR/include $CPPFLAGS"
export LDFLAGS="-L$DEPENDS_DIR/lib $LDFLAGS"

# Useful at runtime for loading shared libs from depends
export LD_LIBRARY_PATH="$DEPENDS_DIR/lib:$LD_LIBRARY_PATH"

export CC=clang-22
export CXX=clang++-22
export CXXFLAGS="-stdlib=libc++ -std=c++20 $CXXFLAGS"
export LDFLAGS="-stdlib=libc++ $LDFLAGS"


# Generate configure script if needed
./autogen.sh

# Configure (install prefix optionally set to DEPENDS_DIR)
./configure --prefix="$DEPENDS_DIR"

# Build using all CPU cores
make -j"$(nproc)"
