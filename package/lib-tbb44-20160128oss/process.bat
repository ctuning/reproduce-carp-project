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

set PACKAGE_NAME=tbb44_20160128oss_win_0
set PACKAGE_FILE=%PACKAGE_NAME%.zip
set PACKAGE_URL=https://www.threadingbuildingblocks.org/sites/default/files/software_releases/windows/%PACKAGE_FILE%

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

exit /b 0

:err
exit /b 1
