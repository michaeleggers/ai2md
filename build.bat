
@echo off




set assimp_inc= ..\src\dependencies\assimp\include
set assimp_lib= ..\src\dependencies\assimp\libs\x64

set flags_debug=   -std=c99 -Wall -Wextra -pedantic-errors -fextended-identifiers -g -D DEBUG
set flags_release=   -std=c99 -Wall -Wextra -pedantic-errors -fextended-identifiers

set clang_flags_debug= /TC /Z7 /DDEBUG /W4 /WX /MDd -Qunused-arguments
set clang_flags_debug_easy= /TC /Z7 /DDEBUG /W4 /MDd -Qunused-arguments -Wno-unused-variable
set clang_flags_release= /TC /O2 /W4 /MD -Qunused-arguments -Wno-unused-variable

set tcc_flags_debug= -Wall -g
set tcc_flags_release= -Wall

pushd "%~dp0"
mkdir build
pushd build

REM gcc %flags_debug%   ..\src\main.c -o gcc_dbg_main
REM gcc %flags_release% ..\src\main.c -o gcc_release_main

REM clang-cl %clang_flags_debug%  ..\src\main.c -o clang_dbg_main.exe
clang-cl %clang_flags_debug_easy% ..\src\main.c ..\src\tr_math.c ..\src\mestack.c -o clang_dbg_easy_main.exe -I%assimp_inc% /link %assimp_lib%\debug\assimp-vc141-mtd.lib
REM clang-cl %clang_flags_release% ..\src\main.c -o clang_rel_main.exe

REM tcc %tcc_flags_debug% ..\src\main.c -o tcc_dbg_main.exe
REM tcc %tcc_flags_release% ..\src\main.c -o tcc_rel_main.exe


popd
popd


