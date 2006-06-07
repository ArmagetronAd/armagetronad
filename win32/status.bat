@echo off
SETLOCAL
REM This batch file can be used to build the target directories "dist" and "debug" one layer
REM up from here where the release and debug binaries will be compiled to.

SET MY_ERROR=

echo ------------------------------------------------------------------------------
echo --- Status report                                                          ---
echo ------------------------------------------------------------------------------
call :test %1\SDL.dll "check for missing files"
call :test %1\SDL_image.dll "check for missing files"
call :test %1\SDL_mixer.dll "check for missing files"
call :test %1\FTGL.dll "check for missing files"
call :test %1\jpeg.dll "check for missing files"
call :test %1\libpng13.dll "check for missing files"
call :test %1\zlib1.dll "check for missing files"
call :test %1\libxml2.dll "check for missing files"
call :test %1\iconv.dll "check for missing files"
call :test %1\freetype6.dll "check for missing files"
call :test %1\resource\included\Anonymous\polygon\regular\square-1.0.1.aamap.xml "check for python errors"
IF DEFINED MY_ERROR goto :warn
goto exit

:test
IF EXIST %1 (
	echo ok	%~1
) ELSE (
	echo missing	%~1
	SET MY_ERROR=%~2
)
goto exit
:warn
echo.
echo *** WARNING: %MY_ERROR%
echo.

:exit
:return
