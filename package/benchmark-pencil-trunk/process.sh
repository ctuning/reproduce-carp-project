#! /bin/bash

#
# Installation script for CK packages.
#
# See CK LICENSE.txt for licensing details.
# See CK Copyright.txt for copyright details.
#
# Developer(s): Grigori Fursin, 2015
#

# PACKAGE_DIR
# INSTALL_DIR

export PACKAGE_NAME=pencil-benchmark

cd ${INSTALL_DIR}

# Clone latest version from GitHub (via git)
echo ""
echo "Cloning from GitHub via git (unless already cloned) ..."
echo ""
git clone --recursive git@github.com:pencil-language/${PACKAGE_NAME}.git


# Clone latest version from GitHub (via http)
echo ""
echo "Cloning from GitHub via http (unless already cloned) ..."
echo ""
git clone --recursive http://github.com/pencil-language/${PACKAGE_NAME}.git


cd ${PACKAGE_NAME}

# However, if already cloned, try to pull
echo ""
echo "Pulling from GitHub (if already cloned) ..."
echo ""
git pull

