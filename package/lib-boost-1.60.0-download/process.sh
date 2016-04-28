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

export PACKAGE_NAME=tbb43_20150424oss
export PACKAGE_FILE=${PACKAGE_NAME}_src.tgz
export PACKAGE_URL=https://www.threadingbuildingblocks.org/sites/default/files/software_releases/source/${PACKAGE_FILE}

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

gzip -d ${PACKAGE_NAME}_src.tgz
tar xvf ${PACKAGE_NAME}_src.tar
rm ${PACKAGE_NAME}_src.tar

if [ "${?}" != "0" ] ; then
 echo "Error: extraction failed in $PWD!" 
 exit 1
fi

cd ${PACKAGE_NAME}

export TBB_INSTALL_DIR=${INSTALL_DIR}/build
export TBB_LIBRARY_RELEASE=${TBB_INSTALL_DIR}/lib
export TBB_LIBRARY_DEBUG=${TBB_INSTALL_DIR}/debug/lib
export TBB_INCLUDE=${TBB_INSTALL_DIR}/include

make tbb_dir=$PWD work_dir=$PWD/build/install
#ndk-build target=android tbb_dir=${INSTALL_DIR}/${PACKAGE_NAME} work_dir=${INSTALL_DIR}/${PACKAGE_NAME}/build/install TBB_VERSION_STRINGS=4.3
# CC=%CK_CC% CXX=%CK_CXX% compiler=gcc

if [ "${?}" != "0" ] ; then
 echo "Error: extraction failed in $PWD!" 
 exit 1
fi
