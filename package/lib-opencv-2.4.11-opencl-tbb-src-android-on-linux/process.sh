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

export PACKAGE_NAME=opencv-2.4.11
export PACKAGE_FILE=${PACKAGE_NAME}.zip
export PACKAGE_URL=http://fossies.org/linux/misc/${PACKAGE_FILE}

cd ${INSTALL_DIR}

echo ""
echo "In the next step, we will download archive from ${PACKAGE_URL} ..."
echo "However, if you already have it, place it inside this directory:"
echo "${INSTALL_DIR}"
echo ""

read -p "Press [Enter] to continue ..."

if [ ! -f ${PACKAGE_FILE} ]; then

 echo ""
 echo "Downloading archive from ${PACKAGE_URL} ..."
 echo ""

 wget ${PACKAGE_URL}

 if [ "${?}" != "0" ] ; then
  echo "Error: Downloading failed in $PWD!" 
  exit 1
 fi
fi

echo ""
echo "Extracting archive ..."
echo ""

unzip ${PACKAGE_FILE}

if [ "${?}" != "0" ] ; then
 echo "Error: extraction failed in $PWD!" 
 exit 1
fi

echo ""
echo "Configuring with cmake ..."
echo ""

mkdir build
cd build

cmake ../${PACKAGE_NAME}  -DCMAKE_C_COMPILER=${CK_CC} -DCMAKE_CXX_COMPILER=${CK_CXX} -DWITH_CUDA=OFF -DWITH_OPENCL=ON ${CK_PARAM_CMAKE_CONFIG} -DWITH_TBB=ON -DTBB_LIB_DIR=${CK_ENV_LIB_TBB_LIB} -DCMAKE_INSTALL_PREFIX:PATH=$PWD/install

if [ "${?}" != "0" ] ; then
 echo "Error: failed configuring package in $PWD!" 
 exit 1
fi

make ${CK_PARAM_MAKE}

if [ "${?}" != "0" ] ; then
 echo "Error: failed configuring package in $PWD!" 
 exit 1
fi

make install ${CK_PARAM_MAKE}

if [ "${?}" != "0" ] ; then
 echo "Error: failed configuring package in $PWD!" 
 exit 1
fi
