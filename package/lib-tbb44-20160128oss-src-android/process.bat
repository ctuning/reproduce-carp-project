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

set PACKAGE_NAME=tbb44_20160128oss_src_0
set LIB_NAME=libtbb
set EXTRA_DIR=tbb44_20160128oss

echo.
echo Extracting archive ...
echo.

cd %INSTALL_DIR%
copy /B %PACKAGE_DIR%\%PACKAGE_NAME%.tgz .
gzip -d %PACKAGE_NAME%.tgz
tar xvf %PACKAGE_NAME%.tar
del /Q %PACKAGE_NAME%.tar

if %errorlevel% neq 0 (
 echo.
 echo Failed extracting package archive!
 goto err
)

cd %EXTRA_DIR%

set PATH=%CK_ANDROID_NDK_ROOT_DIR%;%PATH%

ndk-build target=android tbb tbbmalloc -C %INSTALL_DIR%\%EXTRA_DIR%\jni tbb_os=windows tbb_build_dir=%INSTALL_DIR%\%EXTRA_DIR% tbb_build_prefix=lib

if %errorlevel% neq 0 (
 echo.
 echo Problem building CK package!
 goto err
)

exit /b 0

:err
exit /b 1
