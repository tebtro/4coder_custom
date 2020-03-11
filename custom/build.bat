@echo off

:: usage: <script> [target [mode]]
:: 4coder_default_bindings.cpp release
:: If compiling in debug mode, add /DBUILD_DEBUG to %debug% in buildsuper_x64.bat

pushd ..

call custom\bin\buildsuper_x64-win.bat custom\tebtro_bindings.cpp
:: call custom/bin/buildsuper_x64.bat custom/vim_bindings.cpp

popd
