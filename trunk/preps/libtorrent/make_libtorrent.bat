@echo off
set BOOST_ROOT=%CD%\boost_1_55_0
set GCC_PATH=c:\MinGW
set GCC_VERSION=4.8.1

set path=%GCC_PATH%\bin;%BOOST_ROOT%;%path%
set BOOST_BUILD_PATH=%BOOST_ROOT%
set OLDDIR=%CD%
cd %BOOST_ROOT%
call bootstrap.bat mingw
cd %OLDDIR%
cd examples

bjam client_test -j%NUMBER_OF_PROCESSORS% toolset=gcc myrelease exception-handling=off "-sBUILD=<define>BOOST_NO_EXCEPTIONS" "-sBUILD=<define>BOOST_EXCEPTION_DISABLE" "cxxflags=-fexpensive-optimizations -fomit-frame-pointer"

xcopy %BOOST_ROOT%\boost %GCC_PATH%\include\boost /E /I /Y
cd ..
xcopy include %GCC_PATH%\include /E /I /Y
xcopy bin\gcc-mingw-%GCC_VERSION%\myrelease\exception-handling-off\libtorrent.a %GCC_PATH%\lib /Y
xcopy %BOOST_ROOT%\bin.v2\libs\system\build\gcc-mingw-%GCC_VERSION%\myrelease\exception-handling-off\*.a %GCC_PATH%\lib\libboost_system.a /Y
