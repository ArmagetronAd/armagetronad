@echo off
SETLOCAL
REM This batch file can be used to build the target directories "dist" and "debug" one layer
REM up from here where the release and debug binaries will be compiled to.

SET AA_DIR=..
SET AA_BUILD_DIR=%AA_DIR%\build\tmp

SET /P MAJOR_VERSION= < %AA_DIR%\major_version
SET /P MINOR_VERSION_TPL= < %AA_DIR%\minor_version
SET HELP_FILE=%AA_BUILD_DIR%\help-%RANDOM%.txt

echo.
echo *** checking python
call python.bat -C 1>nul
SET HAVE_PYTHON=%ERRORLEVEL%

IF "%HAVE_PYTHON%"=="0" (
	echo. - Detected python
	call python.bat -c "import datetime; print datetime.date.today().strftime('%%%%Y%%%%m%%%%d')" > %HELP_FILE%
	SET /P DATESTAMP= < %HELP_FILE%
) else (
	echo !!! PYTHON not found
	SET DATESTAMP=SVN
)

SET MINOR_VERSION=%MINOR_VERSION_TPL:DATE=%%DATESTAMP%

IF EXIST %AA_DIR%\.svn (
	echo.
	echo *** reading SVN repository and revision
	SET URL=
	SET REV=
	FOR /F "tokens=1,2 delims=,>=< " %%i in (%AA_DIR%\.svn\entries) do (
		IF *%%i*==*url* (
			SET URL=%%~j
		)
		IF *%%i*==*revision* (
			SET REV=%%~j
		)
	)
)

	REM Close IF block to copy URL and REV to outer scope

	REM uncomment the following URLs to test various cases
	REM trunk
	REM SET URL=https://svn.sourceforge.net/svnroot/armagetronad/armagetronad/trunk/armagetronad
	REM branch 0.4
	REM SET URL=https://svn.sourceforge.net/svnroot/armagetronad/armagetronad/branches/0.4/armagetronad

	REM tag 0.2.8.2
	REM SET URL=https://svn.sourceforge.net/svnroot/armagetronad/armagetronad/tags/0.2.8.2/armagetronad

IF EXIST %AA_DIR%\.svn (
	echo. - SVN: %URL%
	echo. - SVN: %REV%
	FOR /F " tokens=5,6,7 delims=/ " %%i in ("%URL%") do (
		IF *%%j*==*tags* (
			SET MAJOR_VERSION=%%k
			SET MINOR_VERSION=
		) else (
			IF *%%j*==*branches* (
				SET MAJOR_VERSION=%%k
			)
			SET MINOR_VERSION=_alpha%REV%
		)
	)
)
echo.
echo.

echo %REV%
IF NOT DEFINED REV SET REV=%DATESTAMP:~4%

echo *** generating version.h...
echo #define MAJOR_VERSION %MAJOR_VERSION:.=,%,%REV% > %HELP_FILE%
echo #define MINOR_VERSION %MINOR_VERSION% >> %HELP_FILE%
echo #define VERSION "%MAJOR_VERSION%%MINOR_VERSION%" >> %HELP_FILE%
echo #define BUILD_DATE "%DATESTAMP%" >> %HELP_FILE%

echo. - Detected: Armagetron Advanced %MAJOR_VERSION%%MINOR_VERSION%
echo.
echo.
echo Do you want to update 'version.h' ? (requires full rebuild afterwards)
SET FLAG=/Y
IF EXIST %AA_DIR%\src\version.h SET FLAG=/-Y
copy %HELP_FILE% %AA_DIR%\src\version.h %FLAG%

:exit
IF NOT "%HAVE_PYTHON%"=="0" (
	echo ************************************************************************
	echo *** WARNING: Python was not found	***
	echo ************************************************************************
	echo *** Please check your python configuration. You can set the path for ***
	echo *** python in "build_codeblocks\python.bat"	***
	echo ************************************************************************
	echo *** If python is not available on your machine, you should edit	***
	echo ***		%AA_DIR%\src\version.h	***
	echo *** Replace SVN by the current date (format: YYYYMMDD e.g. 20061231^) ***
	echo ************************************************************************
)
echo.
del %HELP_FILE% /Q /F
echo.
echo.
echo done!

pause
:return