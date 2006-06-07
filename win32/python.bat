@echo off
SETLOCAL
REM This batch file can be used to build the target directories "dist" and "debug" one layer
REM up from here where the release and debug binaries will be compiled to.

REM Set your python path here - only required if it is not found
SET PYTHON=python.exe

%PYTHON% -c "import sys; sys.exit(222)" 2>nul 1>nul
if *%1*==*-C* (
	if %ERRORLEVEL%==222 (
		echo --- found %PYTHON% --- 
		%PYTHON% -V
		echo ----------------------
	) ELSE (
		echo ------------------------------------------------------------------------------ 
		echo --- WARNING python was not found                                           ---
		echo ------------------------------------------------------------------------------
		echo --- Please check your python configuration. You can set the path for       ---
		echo --- python in "build_codeblocks\python.bat"                                ---
		echo ------------------------------------------------------------------------------
	)
	goto :exit	
)
IF %ERRORLEVEL%==222 goto :found
goto :exit

:found
%PYTHON% %*
:exit
:return