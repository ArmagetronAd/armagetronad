@echo off
SETLOCAL
REM This batch file can be used to build the target directories "dist" and "debug" one layer
REM up from here where the release and debug binaries will be compiled to.

SET AA_DIR=..

SET /P MAJOR_VERSION= < %AA_DIR%\major_version
SET /P MINOR_VERSION_TPL= < %AA_DIR%\minor_version
SET HELP_FILE=%AA_BUILD_DIR%\help-%RANDOM%.txt

echo *** checking python
call python.bat -C 1>nul
IF %ERRORLEVEL%==0 SET HAVE_PYTHON=1
IF *%HAVE_PYTHON%*==*1* (
	echo     PYTHON found
	call python.bat -c "import datetime; print datetime.date.today().strftime('%%%%Y%%%%m%%%%d')" > %HELP_FILE%
	SET /P DATESTAMP= < %HELP_FILE%
) else (
	echo !!! PYTHON not found
	SET DATESTAMP=SVN
)

SET MINOR_VERSION=%MINOR_VERSION_TPL:DATE=%%DATESTAMP%
echo *** generating version.h...
echo #define MAJOR_VERSION %MAJOR_VERSION:.=,%,%DATESTAMP:~4% > %HELP_FILE%
echo #define MINOR_VERSION %MINOR_VERSION% >> %HELP_FILE%
echo #define VERSION "%MAJOR_VERSION%%MINOR_VERSION%" >> %HELP_FILE%
echo #define BUILD_DATE "%DATESTAMP%" >> %HELP_FILE%


echo.
echo. 
echo === Armagetron Advanced %MAJOR_VERSION%%MINOR_VERSION% ===
echo.
echo Do you want to update 'version.h' ? (requires full rebuild afterwards)
SET FLAG=/Y
IF EXIST %AA_DIR%\src\version.h SET FLAG=/-Y
copy %HELP_FILE% %AA_DIR%\src\version.h %FLAG%

:exit
IF NOT *%HAVE_PYTHON%*==*1* (
	echo ************************************************************************
	echo *** WARNING: Python was not found                                    ***
	echo ************************************************************************
	echo *** Please check your python configuration. You can set the path for ***
	echo *** python in "build_codeblocks\python.bat"                          ***
	echo ************************************************************************
	echo *** If python is not available on your machine, you should edit      ***
	echo ***       %AA_DIR%\src\version.h                                     ***
	echo *** Replace SVN by the current date (format: YYYYMMDD e.g. 20061231) ***
	echo ************************************************************************
)

del %HELP_FILE% /Q /F

echo done!
pause
:return