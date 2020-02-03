@echo off


::
:: check if environment variables exist
::

IF "%VS_VCVARSALL_FILE_PATH%"=="" (
    :: path should be something like this:
    :: "D:\Programme\Microsoft Visual Studio Enterprise\VC\Auxiliary\Build\vcvarsall.bat" x64
    ::
    echo.
    echo Could not find environment variable for vcvarsall.bat!!!
    echo.

    set environment_error=true
)

IF "%IML_LIBRARY_INCLUDE_DIR%"=="" (
    echo.
    echo Could not find folder path to iml library, environment variable IML_LIBRARY_INCLUDE_DIR could not be found!!!
    echo.

    set environment_error=true
)

IF "%SYSTEM_IS_LAPTOP%"=="" (
    echo.
    echo Could detect if system is a laptop, environment variable SYSTEM_IS_LAPTOP could not be found!!!
    echo.

    set environment_error=true
)


if "%environment_error%"=="true" (
    ::PAUSE
    goto:EOF
)


::
:: folder paths setup
::

set misc_folder_path=%~dp0

pushd %misc_folder_path%..\
set project_folder_path=%cd%\
popd

set src_folder_path=%project_folder_path%custom\


::
:: vcvarsall
::

call "%VS_VCVARSALL_FILE_PATH%" x64


::
:: add misc folder to PATH
::

set PATH=%misc_folder_path%;%PATH%