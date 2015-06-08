@echo off

rem
rem Installation script for CK packages.
rem
rem See CK LICENSE.txt for licensing details.
rem See CK Copyright.txt for copyright details.
rem
rem Developer(s): Grigori Fursin, 2015
rem

rem PACKAGE_DIR
rem INSTALL_DIR

set PACKAGE_NAME=tbb43_20150424oss
set PACKAGE_FILE=%PACKAGE_NAME%_src.tgz
set PACKAGE_URL=https://www.threadingbuildingblocks.org/sites/default/files/software_releases/source/%PACKAGE_FILE%

cd %INSTALL_DIR%

echo.
echo In the next step, we will download archive from %PACKAGE_URL% ...
echo However, if you already have it, place it inside this directory:
echo %INSTALL_DIR%
echo.

pause

if not exist %PACKAGE_FILE% (

 echo.
 echo Downloading archive from %PACKAGE_URL% ...
 echo.

 wget %PACKAGE_URL%

 if %errorlevel% neq 0 (
  echo.
  echo Failed downloading package archive!
  goto err
 )
)

echo.
echo Extracting archive ...
echo.

gzip -d %PACKAGE_NAME%_src.tgz
tar xvf %PACKAGE_NAME%_src.tar
rm %PACKAGE_NAME%_src.tar

if %errorlevel% neq 0 (
 echo.
 echo Failed extracting package archive!
 goto err
)

cd %PACKAGE_NAME%

set TBB_INSTALL_DIR=%INSTALL_DIR%/build
set TBB_LIBRARY_RELEASE=%TBB_INSTALL_DIR%/lib
set TBB_LIBRARY_DEBUG=%TBB_INSTALL_DIR%/debug/lib
set TBB_INCLUDE=%TBB_INSTALL_DIR%/include

ndk-build target=android tbb_dir=%INSTALL_DIR%\%PACKAGE_NAME% work_dir=%INSTALL_DIR%\%PACKAGE_NAME%\build\install 
rem CC=%CK_CC% CXX=%CK_CXX% compiler=gcc

if %errorlevel% neq 0 (
 echo.
 echo Failed extracting package archive!
 goto err
)

exit /b 0

:err
exit /b 1
