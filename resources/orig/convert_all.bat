del out\*.* /Q
for /F %%i in ('dir /b *.png') do png2webp.bat %%i out\%%i
move out\*.png *.webp
