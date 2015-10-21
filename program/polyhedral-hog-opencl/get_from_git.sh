#!/bin/bash
echo "Updating submodule 'pencil-benchmark'..."
cd ../../
git submodule init
git submodule update
cd program/polyhedral-hog-opencl 

echo "Patching submodule 'pencil-benchmark'... (Run 'revert_patch.sh' to revert.)"
./apply_patch.sh
