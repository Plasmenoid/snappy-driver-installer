@echo off
cls
cd torrent
set BOOST_ROOT=%CD%\boost_1_55_0
set GCC_PATH=c:\MinGW_481
set GCC_VERSION=4.8.1

set path=%GCC_PATH%\bin;%BOOST_ROOT%;%path%
set BOOST_BUILD_PATH=%BOOST_ROOT%
set LIBTORRENT_PATH=%CD%

rem Check for MinGW
if /I not exist %GCC_PATH%\bin (echo ERROR: MinGW not found in %GCC_PATH% & goto EOF)
echo MinGW:      %GCC_PATH%

rem Check for libtorrent
if /I exist "%LIBTORRENT_PATH%\examples\enum_if.cpp" (echo ERROR: libtorrent was supposed be 1.0.0-RC2 & goto EOF)
if /I not exist "%LIBTORRENT_PATH%\examples\client_test.cpp" (echo ERROR: libtorrent not found in %LIBTORRENT_PATH% & goto EOF)
echo libtorrent: %LIBTORRENT_PATH%

rem Check for BOOST
if /I not exist "%BOOST_ROOT%\boost.png" (echo ERROR: BOOST not found in %BOOST_ROOT% & goto EOF)
echo BOOST:      %BOOST_ROOT%
echo.

rem prepare for webp
if /I exist %GCC_PATH%\msys\1.0\etc\fstab (echo Skipping prepwebp & goto skipprepwebp)
xcopy ..\webp\mingw %GCC_PATH% /E /I /Y
echo %GCC_PATH% /mingw > %GCC_PATH%\msys\1.0\etc\fstab
:skipprepwebp

rem Build bjam.exe
if /I exist "%BOOST_ROOT%\bjam.exe" (echo Skipping Build bjam.exe & goto skipbuildbjam)
xcopy ..\asio\wspiapi.h %GCC_PATH%\include\wspiapi.h /Y
xcopy ..\asio\socket_types.hpp "%BOOST_ROOT%\boost\asio\detail\socket_types.hpp" /Y
cd "%BOOST_ROOT%"
call bootstrap.bat mingw
:skipbuildbjam

rem Build libtorrent.a
cd "%LIBTORRENT_PATH%"
if /I exist "%LIBTORRENT_PATH%\bin\gcc-mingw-%GCC_VERSION%\myrelease\exception-handling-off\libtorrent.a" (echo Skipping Build libtorrent & goto skipbuildlibtorrent)
cd examples
copy "%LIBTORRENT_PATH%\..\Jamfile_fixed" "%LIBTORRENT_PATH%\examples\Jamfile" /Y
copy "%LIBTORRENT_PATH%\..\lt_trackers.cpp" "%LIBTORRENT_PATH%\src\lt_trackers.cpp" /Y
bjam client_test -j%NUMBER_OF_PROCESSORS% toolset=gcc myrelease exception-handling=off "-sBUILD=<define>BOOST_NO_EXCEPTIONS" "-sBUILD=<define>BOOST_EXCEPTION_DISABLE" "cxxflags=-fexpensive-optimizations -fomit-frame-pointer -D IPV6_TCLASS=30"
cd ..
:skipbuildlibtorrent

rem Copy libtorrent.a
if /I exist %GCC_PATH%\lib\libtorrent.a (echo Skipping copy libtorrent.a & goto skipcopylibtorrent)
xcopy bin\gcc-mingw-%GCC_VERSION%\myrelease\exception-handling-off\libtorrent.a %GCC_PATH%\lib /Y
xcopy include %GCC_PATH%\include /E /I /Y
:skipcopylibtorrent

rem Install BOOST
if /I exist "%BOOST_ROOT%\boost" (echo Skipping BOOST[include\boost] & goto skipboost1)
xcopy "%BOOST_ROOT%\boost" %GCC_PATH%\include\boost /E /I /Y
:skipboost1
if /I exist %GCC_PATH%\lib\libboost_system.a (echo Skipping BOOST[libboost_system.a] & goto skipboost2)
xcopy "%BOOST_ROOT%\bin.v2\libs\system\build\gcc-mingw-%GCC_VERSION%\myrelease\exception-handling-off\*.a" %GCC_PATH%\lib\libboost_system.a /Y
:skipboost2

echo.
echo Everything is done

:EOF
pause