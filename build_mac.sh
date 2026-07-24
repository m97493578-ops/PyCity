#!/bin/bash
# Install raylib using Homebrew on the cloud machine
brew install raylib

# Compile using Apple's native frameworks instead of Windows libraries
gcc Src/main.c -lraylib -framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo -o PyCity
