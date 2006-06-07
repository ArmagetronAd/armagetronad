@echo off
SETLOCAL
REM This batch file can be used to build the target directories "dist" and "debug" one layer
REM up from here where the release and debug binaries will be compiled to.

SET AA_DIR=..
SET AA_BUILD_DIR=%AA_DIR%\build
SET LIBS_DIR=..\..\winlibs
SET LIBS_BUILD_DIR=%LIBS_DIR%\build
SET INSTALLER_DIR=..\..\build\win32

SET DIST_DIR_BASE=dist
SET DEBUG_DIR_BASE=debug
SET PROFILE_DIR_BASE=profile
SET DIST_DIR=%AA_BUILD_DIR%\%DIST_DIR_BASE%
SET DEBUG_DIR=%AA_BUILD_DIR%\%DEBUG_DIR_BASE%
SET PROFILE_DIR=%AA_BUILD_DIR%\%PROFILE_DIR_BASE%

SET /P MAJOR_VERSION= < %AA_DIR%\major_version
SET /P MINOR_VERSION_TPL= < %AA_DIR%\minor_version
SET HELP_FILE=%AA_BUILD_DIR%/help-%RANDOM%.txt

REM echo making directory...
REM del %DIST_DIR% /S /Q
REM del %DEBUG_DIR% /S /Q
REM del %PROFILE_DIR% /S /Q
REM mkdir %DEBUG_DIR%
REM mkdir %PROFILE_DIR%

echo *** checking python
call python.bat -C 1>nul
IF %ERRORLEVEL%==0 SET HAVE_PYTHON=1
IF *%HAVE_PYTHON%*==*1* (
	echo     PYTHON found
	call python.bat -c "import datetime; print datetime.date.today().strftime('%%%%Y%%%%m%%%%d')" > %HELP_FILE%
	SET /P DATESTAMP= < %HELP_FILE%
	del %HELP_FILE% /Q /F
) else (
	echo !!! PYTHON not found
	SET DATESTAMP=SVN
)

SET MINOR_VERSION=%MINOR_VERSION_TPL:DATE=%%DATESTAMP%
echo *** generating version.h...
echo Detected version: Armagetron Advanced %MAJOR_VERSION%%MINOR_VERSION%
echo #define MAJOR_VERSION %MAJOR_VERSION:.=,%,%DATESTAMP:~4% > %AA_DIR%\src\version.h
echo #define MINOR_VERSION %MINOR_VERSION% >> %AA_DIR%\src\version.h
echo #define VERSION "%MAJOR_VERSION%%MINOR_VERSION%" >> %AA_DIR%\src\version.h

IF EXIST %DIST_DIR% (
	mkdir %DEBUG_DIR%\var
	echo Your personal configuration will be stored here. > %DIST_DIR%\var\README.txt
	echo Updates will never touch this directory. >> %DIST_DIR%\var\README.txt
	call :copy_files %DIST_DIR% %DIST_DIR_BASE%
)

IF EXIST %DEBUG_DIR% (
	call :copy_files %DEBUG_DIR% %DEBUG_DIR_BASE%
)

IF EXIST %PROFILE_DIR% (
	call :copy_files %PROFILE_DIR% %PROFILE_DIR_BASE%
)

echo.
echo. 
echo === Armagetron Advanced %MAJOR_VERSION%%MINOR_VERSION% ===
echo.
IF EXIST %DIST_DIR% call status.bat %DIST_DIR%
IF EXIST %DEBUG_DIR% call status.bat %DEBUG_DIR%
IF EXIST %PROFILE_DIR% call status.bat %PROFILE_DIR%

goto :exit

:copy_files
echo +-------------------------------------------------------
echo ^| updating %2                                           
echo +-------------------------------------------------------
echo *** copying files...
xcopy %AA_DIR%\arenas %1\arenas /I /E /Y /C
xcopy %AA_DIR%\config %1\config /I /E /Y /C
xcopy %AA_DIR%\src\doc %1\doc /I /E /Y /C
xcopy %AA_DIR%\language %1\language /I /E /Y /C
xcopy %AA_DIR%\models %1\models /I /E /Y /C
xcopy %AA_DIR%\music %1\music /I /E /Y /C
xcopy %AA_DIR%\sound %1\sound /I /E /Y /C
mkdir %1\resource
xcopy %AA_DIR%\resource\proto %1\resource\included /I /E /Y /C
xcopy %AA_DIR%\resource\included %1\resource\included /I /E /Y /C
xcopy %AA_DIR%\textures %1\textures /I /E /Y /C
xcopy %AA_DIR%\*.txt %1 /I /Y /C
copy %AA_DIR%\README %1\README.txt /Y
copy %AA_DIR%\README-SDL %1\README-SDL.txt /Y
move /Y %1\config\aiplayers.cfg.in %1\config\aiplayers.cfg
move /Y %1\language\languages.txt.in %1\language\languages.txt

echo *** copying binary only dlls ( WATCH FOR ERRORS! )...
xcopy %LIBS_DIR%\SDL_image\VisualC\graphics\lib\*.dll %1 /C /Y /I
xcopy %LIBS_DIR%\SDL_mixer\VisualC\smpeg\lib\*.dll %1 /C /Y /I
xcopy %LIBS_DIR%\libxml2\lib\*.dll %1 /C /Y /I
xcopy %LIBS_DIR%\iconv\lib\*.dll %1 /C /Y /I
xcopy %LIBS_DIR%\FTGL\win32\freetype\lib\*.dll %1 /C /Y /I
xcopy %LIBS_BUILD_DIR%\%2\*.dll %1 /C /Y /I

echo *** copying installation files...
xcopy %INSTALLER_DIR%\*.nsi %1 /Y
xcopy %INSTALLER_DIR%\*.bmp %1 /Y
xcopy %INSTALLER_DIR%\*.url %1 /Y

echo *** moving resources
IF *%HAVE_PYTHON%*==*1* (
	echo *** Sort resources
	call python.bat "%AA_DIR%/batch/make/sortresources.py" %~1/resource/included
)
goto :return

:exit
IF NOT *%HAVE_PYTHON%*==*1* (
	echo ************************************************************************
	echo *** WARNING: Python was not found - can't sort XML resources         ***
	echo ************************************************************************	
	echo *** Please check your python configuration. You can set the path for ***
	echo *** python in "build_codeblocks\python.bat"                          ***     
	echo ************************************************************************
)
SET AA_DIR=
SET AA_BUILD_DIR=
SET LIBS_DIR=
SET LIBS_BUILD_DIR=
SET INSTALLER_DIR=

REM SET DIST_DIR=
REM SET DIST_DIR_BASE=
REM SET DEBUG_DIR=
REM SET DEBUG_DIR_BASE=
REM SET PROFILE_DIR_BASE=
REM SET PROFILE_DIR=

echo done!
pause
:return