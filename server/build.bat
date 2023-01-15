@echo off

if not exist bin\ (
    mkdir bin
)

pushd bin
cl /Zi /nologo /I../include /I../common ../src/*.c ../src/win64/*.c /link ws2_32.lib
popd