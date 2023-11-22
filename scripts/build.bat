@echo off
setlocal

cd ..
if not exist build\win mkdir build\win
cd build\win
cmake ../..
if %errorlevel% neq 0 exit /b !errorlevel!

if "%1" == "1" goto nodebug
msbuild /property:Configuration=Debug firefly_synth.sln
if %errorlevel% neq 0 exit /b !errorlevel!

:nodebug
if "%1" == "0" goto norelease
msbuild /property:Configuration=Release firefly_synth.sln
if %errorlevel% neq 0 exit /b !errorlevel!

:norelease
cd ..\..\scripts
