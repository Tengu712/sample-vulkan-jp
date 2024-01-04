@echo off

set tmpdir=%cd%

if not exist %~dp0out mkdir %~dp0out

cd %~dp0out

cl ^
    /O2 /Oi ^
    /EHsc /Gd /GL /Gy ^
    /Fe:sample-vulkan-jp.exe ^
    /DNDEBUG /D_CONSOLE /D_UNICODE /DUNICODE ^
    /permissive- /Zc:inline ^
    /MD ^
    /FC /nologo /utf-8 ^
    /sdl /W4 /WX ^
    ..\src\*.c

del *.obj

cd %tmpdir%
