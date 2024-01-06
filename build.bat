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
    ^
    /IC:\VulkanSDK\1.3.268.0\Include ^
    ..\src\vulkan\*.c ^
    ..\src\vulkan\offscreen\*.c ^
    ..\src\vulkan\windows\*.c ^
    ..\src\*.c ^
    ^
    /link ^
    /LIBPATH:C:\VulkanSDK\1.3.268.0\Lib ^
    vulkan-1.lib ^
    user32.lib

del *.obj

cd %tmpdir%
