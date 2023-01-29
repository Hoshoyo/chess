@echo off

if not exist bin\ (
    mkdir bin
)

pushd bin
rem cl /nologo /Zi /I../include ../*.c ../network/*.c /Fe:chess.exe /link user32.lib gdi32.lib kernel32.lib shell32.lib opengl32.lib Ws2_32.lib ../lib/freetype.lib
cl /nologo /O2 /I../include ../*.c ../network/*.c /Fe:chess.exe /link user32.lib gdi32.lib kernel32.lib shell32.lib opengl32.lib Ws2_32.lib ../lib/freetype.lib
copy chess.exe ..
copy ..\lib\freetype.dll ..
popd    