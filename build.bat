@echo off

pushd bin
cl /nologo /Zi /I../include ../*.c ../network/*.c /Fe:chess.exe /link user32.lib gdi32.lib kernel32.lib shell32.lib opengl32.lib Ws2_32.lib ../lib/freetype.lib
popd    