@echo off

set tmpdir=%cd%

if not exist %~dp0bin mkdir %~dp0bin

cd %~dp0bin

node .\model\converter.js .\model\square.json .\model\square.raw
node .\model\converter.js .\model\utah.json .\model\utah.raw

glslc -o .\shader\ui.vert.spv .\shader\ui.vert
glslc -o .\shader\ui.frag.spv .\shader\ui.frag

cl ^
    /Fe:sample-vulkan-jp.exe ^
    ^
    /O2 /Oi ^
    /EHsc /Gd /GL /Gy ^
    /DNDEBUG /D_CONSOLE /D_UNICODE /DUNICODE ^
    /permissive- /Zc:inline ^
    /MD ^
    /FC /nologo /utf-8 ^
    /sdl /W4 /WX ^
    ^
    /IC:\VulkanSDK\1.3.268.0\Include ^
    ..\src\vulkan\util\*.c ^
    ..\src\vulkan\util\memory\*.c ^
    ..\src\vulkan\pipelines\*.c ^
    ..\src\vulkan\*.c ^
    ..\src\apps\offscreen\*.c ^
    ..\src\apps\windows\*.c ^
    ..\src\*.c ^
    ^
    /link ^
    /LIBPATH:C:\VulkanSDK\1.3.268.0\Lib ^
    vulkan-1.lib ^
    user32.lib

del *.obj

cd %tmpdir%
