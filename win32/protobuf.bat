@echo off
SETLOCAL
REM This batch file recompiles the .proto files to c++ headers and object files.

cd ..\src\protobuf

protoc *.proto --cpp_out=.

echo.
echo.
echo done (provided there are no error messages...)!

pause
:return