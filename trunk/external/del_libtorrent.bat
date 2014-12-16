@echo off
cls
set GCC_PATH=c:\MinGW_481

rd /S /Q %GCC_PATH%\include\boost
del /F /S /Q %GCC_PATH%\lib\libboost_system.a
del /F /S /Q %GCC_PATH%\lib\libtorrent.a

pause