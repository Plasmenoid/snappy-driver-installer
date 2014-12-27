@echo off
echo %1
SDI_R.exe -ls:..\snp\%1 -nostamp -nogui -nosnapshot -keepunpackedindex -filters:%2 -verbose:%3 %4 >nul
echo -------------------------------------------------------------------------- >logs\%1
chcp 1251>nul
echo %1>>logs\%1
type logs\log.txt >> logs\%1
chcp 866>nul
