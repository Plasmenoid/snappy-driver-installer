@echo off
cls
cd torrent
set BOOST_ROOT=%CD%\boost_1_57_0
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

rem Check for webp
if /I not exist ..\webp\mingw\msys\1.0\home\libwebp-0.4.0.tar.gz  (echo ERROR: libwebp-0.4.0.tar.gz not found in webp\mingw\msys\1.0\home & goto EOF)

rem prepare for webp
if /I exist %GCC_PATH%\msys\1.0\home\libwebp-0.4.0.tar.gz (echo Skipping prepwebp & goto skipprepwebp)
xcopy ..\webp\mingw %GCC_PATH% /E /I /Y
echo %GCC_PATH% /mingw > %GCC_PATH%\msys\1.0\etc\fstab
:skipprepwebp

rem Build bjam.exe
if /I not exist %GCC_PATH%\include\wspiapi.h (copy ..\asio\wspiapi.h %GCC_PATH%\include\wspiapi.h /Y)
if /I not exist %GCC_PATH%\include\shobjidl.h (copy ..\shobjidl.h %GCC_PATH%\include\shobjidl.h /Y)
copy ..\asio\socket_types.hpp "%BOOST_ROOT%\boost\asio\detail\socket_types.hpp" /Y
if /I exist "%BOOST_ROOT%\bjam.exe" (echo Skipping Build bjam.exe & goto skipbuildbjam)
cd "%BOOST_ROOT%"
call bootstrap.bat mingw
:skipbuildbjam

rem Build libtorrent.a
cd "%LIBTORRENT_PATH%"
if /I exist "%LIBTORRENT_PATH%\bin\gcc-mngw-%GCC_VERSION%\myrls\excpt-hndl-off\libtorrent.a" (echo Skipping Build libtorrent & goto skipbuildlibtorrent)
cd examples
copy "%LIBTORRENT_PATH%\..\Jamfile_fixed" "%LIBTORRENT_PATH%\examples\Jamfile" /Y
copy "%LIBTORRENT_PATH%\..\lt_trackers.cpp" "%LIBTORRENT_PATH%\src\lt_trackers.cpp" /Y
bjam --abbreviate-paths client_test -j%NUMBER_OF_PROCESSORS% toolset=gcc myrelease exception-handling=off "-sBUILD=<define>BOOST_NO_EXCEPTIONS" "-sBUILD=<define>BOOST_EXCEPTION_DISABLE" "cxxflags=-fexpensive-optimizations -fomit-frame-pointer -D IPV6_TCLASS=30"
cd ..
:skipbuildlibtorrent

rem Copy libtorrent.a
if /I exist %GCC_PATH%\lib\libtorrent.a (echo Skipping copy libtorrent.a & goto skipcopylibtorrent)
copy bin\gcc-mngw-%GCC_VERSION%\myrls\excpt-hndl-off\libtorrent.a %GCC_PATH%\lib /Y
:skipcopylibtorrent

rem Copy libtorrent.a
if /I exist %GCC_PATH%\include\libtorrent (echo Skipping copy %GCC_PATH%\include\libtorrent & goto skipcopylibtorrentinc)
xcopy include %GCC_PATH%\include /E /I /Y
:skipcopylibtorrentinc

rem Copy %GCC_PATH%\include\boost
if /I exist %GCC_PATH%\include\boost (echo Skipping BOOST[include\boost] & goto skipboost1)
echo Copying %GCC_PATH%\include\boost
xcopy "%BOOST_ROOT%\boost" %GCC_PATH%\include\boost /E /I /Y
:skipboost1

rem Copy libboost_system.a
if /I exist %GCC_PATH%\lib\libboost_system.a (echo Skipping BOOST[libboost_system.a] & goto skipboost2)
echo Copying libboost_system.a
xcopy "%BOOST_ROOT%\bin.v2\libs\system\build\gcc-mngw-%GCC_VERSION%\myrls\excpt-hndl-off\*.a" %GCC_PATH%\lib\libboost_system.a
:skipboost2

echo.
echo Everything is done

:EOF
pause