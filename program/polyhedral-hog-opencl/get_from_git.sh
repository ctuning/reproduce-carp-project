#!/bin/bash

echo "Updating submodule 'pencil-benchmark'..."
cd ../../
git submodule init
git submodule update

echo "Patching submodule 'pencil-benchmark'..."
cd program/polyhedral-hog-opencl/ && ./apply_patch.sh

echo "To revert patch, use 'revert_patch.sh'..."
