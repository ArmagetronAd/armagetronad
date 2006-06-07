@echo off
SETLOCAL
REM This batch file can be used to build the target directories "dist" and "debug" one layer
REM up from here where the release and debug binaries will be compiled to.

SET AA_DIR=..\armagetronad
SET LIBS_DIR=..\winlibs
SET DIST_DIR_BASE=dist
SET DEBUG_DIR_BASE=debug
SET DIST_DIR=..\%DIST_DIR_BASE%
SET DEBUG_DIR=..\%DEBUG_DIR_BASE%

REM SET PROFILE_DIR_BASE=profile
REM SET PROFILE_DIR=..\%PROFILE_DIR_BASE%

echo making directory...
REM del %DIST_DIR% /S /Q
REM del %DEBUG_DIR% /S /Q
mkdir %DIST_DIR%
mkdir %DIST_DIR%\var
mkdir %DEBUG_DIR%
REM mkdir %PROFILE_DIR%

echo Your personal configuration will be stored here. Updates will never touch this directory. > %DIST_DIR%\var\README.txt

echo *** checking python
call python.bat -C
IF %ERRORLEVEL%==0 SET HAVE_PYTHON=1
IF *%HAVE_PYTHON%*==*1* (
	echo *** generating version.h...
	call python.bat -c "import datetime; print '#define VERSION ""0.3.0_Alpha'+datetime.date.today().strftime('%%%%Y%%%%m%%%%d')+'""' " > %AA_DIR%\version.h
	call python.bat -c "import datetime; print 'Version: Alpha'+datetime.date.today().strftime('%%%%Y%%%%m%%%%d') "
) else (
	echo *** generating pseudo version.h...
	echo #define VERSION "CVS" > %AA_DIR%\version.h
)

echo making dist...
call :copy_files %DIST_DIR%

echo making debug...
REM xcopy %DIST_DIR% %DEBUG_DIR% /I /E /Y /C
call :copy_files %DEBUG_DIR%

REM echo making profile...
REM xcopy %DIST_DIR% %PROFILE_DIR% /I /E /Y /C
REM call :copy_files %PROFILE_DIR%

call status.bat %DIST_DIR%
call status.bat %DEBUG_DIR%
REM call status.bat %PROFILE_DIR%

goto :exit

:copy_files
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
xcopy %LIBS_DIR%\libxml2\lib\*.dll %1 /C /Y /I
xcopy %LIBS_DIR%\iconv\lib\*.dll %1 /C /Y /I
xcopy %LIBS_DIR%\freetype\lib\*.dll %1 /C /Y /I

echo *** copying installation files...
xcopy *.nsi %1 /Y
xcopy *.bmp %1 /Y
xcopy *.url %1 /Y

echo *** moving resources
IF *%HAVE_PYTHON%*==*1* (
	echo ***************************************************
	echo *** Sort resources
	call python.bat "%AA_DIR%/batch/make/sortresources.py" %~1/resource/included
	echo ***************************************************
) else (
	echo ***************************************************
	echo *** WARNING: Python missing - can't sort resources
	echo ***************************************************
)
goto :return

:exit
SET AA_DIR=
SET LIBS_DIR=
SET DIST_DIR=
SET DEBUG_DIR=

echo done!
pause
:return