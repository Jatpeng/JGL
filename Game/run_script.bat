@echo off
setlocal

set "WORKSPACE_DIR=%~dp0"
set "DEFAULT_SCRIPT=%WORKSPACE_DIR%script\three_cubes.py"
set "TARGET_SCRIPT=%DEFAULT_SCRIPT%"

if not "%~1"=="" (
    set "ARG_SCRIPT=%~1"
    if exist "%ARG_SCRIPT%" (
        set "TARGET_SCRIPT=%ARG_SCRIPT%"
    ) else (
        if exist "%WORKSPACE_DIR%%ARG_SCRIPT%" (
            set "TARGET_SCRIPT=%WORKSPACE_DIR%%ARG_SCRIPT%"
        ) else (
            if exist "%WORKSPACE_DIR%script\%ARG_SCRIPT%" (
                set "TARGET_SCRIPT=%WORKSPACE_DIR%script\%ARG_SCRIPT%"
            ) else (
                echo Script not found: %~1
                echo.
                echo Usage:
                echo   run_script.bat
                echo   run_script.bat three_cubes.py
                echo   run_script.bat script\three_cubes.py
                echo   run_script.bat D:\path\to\your_script.py
                pause
                exit /b 1
            )
        )
    )
)

if not exist "%TARGET_SCRIPT%" (
    echo Script not found: %TARGET_SCRIPT%
    pause
    exit /b 1
)

set "PYTHON313=%LocalAppData%\Programs\Python\Python313\python.exe"

pushd "%WORKSPACE_DIR%"

if exist "%PYTHON313%" (
    echo Using Python: %PYTHON313%
    echo Running script: %TARGET_SCRIPT%
    "%PYTHON313%" "%TARGET_SCRIPT%"
    set "EXIT_CODE=%ERRORLEVEL%"
    goto end
)

where py >nul 2>nul
if not errorlevel 1 (
    py -3.13 -c "import sys" >nul 2>nul
    if not errorlevel 1 (
        echo Using Python: py -3.13
        echo Running script: %TARGET_SCRIPT%
        py -3.13 "%TARGET_SCRIPT%"
        set "EXIT_CODE=%ERRORLEVEL%"
        goto end
    )
)

where python >nul 2>nul
if not errorlevel 1 (
    python -c "import sys; raise SystemExit(0 if sys.version_info[:2] == (3, 13) else 1)" >nul 2>nul
    if not errorlevel 1 (
        echo Using Python: python
        echo Running script: %TARGET_SCRIPT%
        python "%TARGET_SCRIPT%"
        set "EXIT_CODE=%ERRORLEVEL%"
        goto end
    )
)

echo Python 3.13 was not found.
echo Install Python 3.13 or update this launcher.
set "EXIT_CODE=1"

:end
popd
if not "%EXIT_CODE%"=="0" pause
exit /b %EXIT_CODE%
