@echo off
if not exist build\ mkdir build
pushd build
if "%1"=="fixed_capacity" (
	cl /W4 /WX /Zi /std:c11 /fsanitize=address /nologo "%~dp0fixed_capacity.c"
) else (
	cl /W4 /WX /Zi /std:c11 /fsanitize=address /nologo "%~dp0chaining.c"
)
popd

