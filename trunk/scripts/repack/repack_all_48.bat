for /F %%i in ('dir /b *.7z') do call bin\repack_single.bat %%i 48
rd temp /s /q
