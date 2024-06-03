@echo off
setlocal

cd ..
if not exist build\win mkdir build\win
cd build\win
cmake ../..
if %errorlevel% neq 0 exit /b !errorlevel!

msbuild /property:Configuration="%1" firefly_synth.sln
if %errorlevel% neq 0 exit /b !errorlevel!

cd ..\..\dist\"%1"\win
plugin_base.ref_gen.exe firefly_synth_1.vst3\Contents\x86_64-win\firefly_synth_1.vst3 ..\..\..\param_reference.html
if %errorlevel% neq 0 exit /b !errorlevel!

7z a -tzip firefly_synth_1.7.7_windows_vst3_fx.zip firefly_synth_fx_1.vst3\*
7z a -tzip firefly_synth_1.7.7_windows_vst3_instrument.zip firefly_synth_1.vst3\*
7z a -tzip firefly_synth_1.7.7_windows_clap_fx.zip firefly_synth_fx_1.clap\*
7z a -tzip firefly_synth_1.7.7_windows_clap_instrument.zip firefly_synth_1.clap\*
if %errorlevel% neq 0 exit /b !errorlevel!

cd ..\..\..\..\scripts