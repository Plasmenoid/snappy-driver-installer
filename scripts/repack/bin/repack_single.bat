1@echo off
Set dict=%2
if "%2"=="" Set dict=32
if not "%1"=="" goto continue
echo Run 'repack_all_XX.bat' instead
echo.
pause
exit

:continue
rd temp /s /q
md temp
bin\7za.exe x %1 -o"temp" -r
del out\%1 -y
cd temp
..\bin\7za.exe a ..\out\%1 -mmt=off -m0=BCJ2 -m1=LZMA:d%dict%m:fb273 -m2=LZMA:d512k -m3=LZMA:d512k -mb0:1 -mb0s1:2 -mb0s2:3 -ir!*.inf -ir!*.cat *.ini
..\bin\7za.exe a ..\out\%1 -mmt=off -m0=BCJ2 -m1=LZMA:d%dict%m:fb273 -m2=LZMA:d512k -m3=LZMA:d512k -mb0:1 -mb0s1:2 -mb0s2:3 -xr!*.inf -xr!*.cat -x!*.ini
cd ..
