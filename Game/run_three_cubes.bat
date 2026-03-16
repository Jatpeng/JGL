@echo off
setlocal

call "%~dp0run_script.bat" three_cubes.py
exit /b %ERRORLEVEL%
