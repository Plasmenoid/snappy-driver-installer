@echo off
cd bin
rd logs /S /Q
SDI_R.exe -nogui -nologfile -nosnapshot -verbose:256
for /F %%i in ('dir /b ..\snp\*.snp') do call process_single.bat %%i %2 %3 %4

del logs\log.txt
rename logs\*.snp *.txt

del ..\%1.txt
echo Stitching...
chcp 866>nul
for /F %%f in ('dir /b logs\*.txt') do call attach_single.bat logs\%%f ..\%1
copy /b logs\*.* ..\%1.txt >nul
dubremover.exe ..\%1.txt ..\%1_cl.txt
echo done