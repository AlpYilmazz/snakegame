@echo off
SETLOCAL

set COMPILER_FLAGS=

set INCLUDE=-Ivendor/raylib/include
set LIB=-Lvendor/raylib/lib

set LINK=-lraylib -lgdi32 -lwinmm

set TARGET_DIR=target
set OUTPUT=game
set COMPILE=main.c grid.c level.c asset.c gameui.c snake.c fireworks.c util.c

mkdir %TARGET_DIR%

@echo on

gcc %COMPILE% %COMPILER_FLAGS% %INCLUDE% %LIB% %LINK% -o ./%TARGET_DIR%/%OUTPUT%.exe

:: gcc main.c -Ivendor/raylib/include -Lvendor/raylib/lib -lraylib -lgdi32 -lwinmm -o target/game.exe