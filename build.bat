@echo off
setlocal
if not exist build\win mkdir build\win
cd build\win
cmake ../..
cd ..\..