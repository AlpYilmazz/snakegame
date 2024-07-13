@echo off
SETLOCAL

set COMPILER_FLAGS=

set INCLUDE=-Ivendor/raylib/include
set LIB=-Lvendor/raylib/lib

set LINK=-lraylib -lgdi32 -lwinmm

set TARGET_DIR=target
set OUTPUT=game
set COMPILE=^
    src/util.c ^
    src/asyncio.c ^
    src/grid.c ^
    src/level.c ^
    src/asset.c ^
    src/animation.c ^
    src/gameui.c ^
    src/snake.c ^
    src/fireworks.c ^
    src/main.c

mkdir %TARGET_DIR%

@echo on

gcc %COMPILE% %COMPILER_FLAGS% %INCLUDE% %LIB% %LINK% -o ./%TARGET_DIR%/%OUTPUT%.exe

@echo off

copy %TARGET_DIR%\%OUTPUT%.exe %OUTPUT%-local.exe

:: gcc main.c -Ivendor/raylib/include -Lvendor/raylib/lib -lraylib -lgdi32 -lwinmm -o target/game.exe