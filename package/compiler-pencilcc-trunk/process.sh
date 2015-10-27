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

export PACKAGE_NAME=pencilcc

cd ${INSTALL_DIR}

# Clone latest version from GitHub via git
echo ""
echo "Cloning from GitHub via git (unless already cloned) ..."
echo ""
git clone --recursive git@github.com:Meinersbur/${PACKAGE_NAME}.git

# Clone latest version from GitHub via http
echo ""
echo "Cloning from GitHub via http (unless already cloned) ..."
echo ""
git clone --recursive http://github.com/Meinersbur/${PACKAGE_NAME}.git

cd ${PACKAGE_NAME}

# However, if already cloned, try to pull
echo ""
echo "Pulling from GitHub (if already cloned) ..."
echo ""
git pull

# Autoreconf
echo ""
echo "Reconfiguring ..."
echo ""

autoreconf -i

# Creating temporal build dir
mkdir _build
cd _build

# Configure
echo ""
echo "Configuring ..."
echo ""
echo "../configure --prefix=$INSTALL_DIR/_install --with-clang-prefix=${CK_ENV_COMPILER_LLVM}"
echo ""
../configure --prefix=$INSTALL_DIR/_install --with-clang-prefix=${CK_ENV_COMPILER_LLVM}
 if [ "${?}" != "0" ] ; then
  echo "Error: Configure failed in $PWD!" 
  exit 1
 fi

# Build
echo ""
echo "Building ..."
echo ""
make
 if [ "${?}" != "0" ] ; then
  echo "Error: Compilation failed in $PWD!" 
  exit 1
 fi

# Install
echo ""
echo "Installing ..."
echo ""
make install
 if [ "${?}" != "0" ] ; then
  echo "Error: Installation failed in $PWD!" 
  exit 1
 fi
