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

set PACKAGE_NAME=opencv-2.4.11
set PACKAGE_FILE=%PACKAGE_NAME%.zip
set PACKAGE_URL=http://fossies.org/linux/misc/%PACKAGE_FILE%

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

unzip %PACKAGE_FILE%

if %errorlevel% neq 0 (
 echo.
 echo Failed extracting package archive!
 goto err
)

echo.
echo Configuring with cmake ...
echo.

mkdir build
cd build

cmake ../%PACKAGE_NAME% -DWITH_CUDA=OFF -DWITH_OPENCL=ON %CK_PARAM_CMAKE_CONFIG% -DWITH_TBB=ON -DTBB_LIB_DIR=%CK_ENV_LIB_TBB_LIB% -DTBB_INCLUDE_DIR=%CK_ENV_LIB_TBB_INCLUDE% -G "Visual Studio 12 2013 Win64"

if %errorlevel% neq 0 (
 echo.
 echo Failed configuring package!
 goto err
)

exit /b 1

echo.
echo Building with msbuild ...
echo.

msbuild ALL_BUILD.vcxproj

if %errorlevel% neq 0 (
 echo.
 echo Failed building package!
 goto err
)

echo.
echo Installing ...
echo.

msbuild INSTALL.vcxproj

if %errorlevel% neq 0 (
 echo.
 echo Failed installing package!
 goto err
)


pause

exit /b 0

:err
exit /b 1
