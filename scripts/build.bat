@echo off
setlocal

cd ..
if not exist build\win mkdir build\win
cd build\win
cmake ../..
if %errorlevel% neq 0 exit /b !errorlevel!

msbuild /property:Configuration=Debug infernal-synth.sln
if %errorlevel% neq 0 exit /b !errorlevel!
msbuild /property:Configuration=Release infernal-synth.sln
if %errorlevel% neq 0 exit /b !errorlevel!

cd ..\..\scripts
