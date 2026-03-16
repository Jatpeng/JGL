@echo off
setlocal

pushd "%~dp0"
python tools\generate_api_docs.py
set "EXIT_CODE=%ERRORLEVEL%"
popd

if not "%EXIT_CODE%"=="0" pause
exit /b %EXIT_CODE%
