@echo off
find %1 "manager_print[0]" >nul
if '%errorlevel%' == '0' (del %1)
