@echo off
SETLOCAL

set COMPILER_FLAGS=

set INCLUDE=-Ivendor/raylib-5.0/include -Ilibrary/threadpool/include
set LIB=-Lvendor/raylib-5.0/lib -Llibrary/threadpool/lib

set LINK=^
    -lraylib -lgdi32 -lwinmm ^
    -lthreadpool

set SRC_DIR=src
set TARGET_DIR=target

set OUTPUT=game.exe
set COMPILE=^
    %SRC_DIR%/util.c ^
    %SRC_DIR%/asyncio.c ^
    %SRC_DIR%/grid.c ^
    %SRC_DIR%/level.c ^
    %SRC_DIR%/asset.c ^
    %SRC_DIR%/animation.c ^
    %SRC_DIR%/gameui.c ^
    %SRC_DIR%/snake.c ^
    %SRC_DIR%/fireworks.c ^
    %SRC_DIR%/main.c

mkdir %TARGET_DIR%

@echo on

gcc %COMPILER_FLAGS% %COMPILE% %INCLUDE% %LIB% %LINK% -o ./%TARGET_DIR%/%OUTPUT%

@echo off

set OUTPUT_LOCAL=game-local.exe
copy %TARGET_DIR%\%OUTPUT% %OUTPUT_LOCAL%

:: gcc main.c -Ivendor/raylib/include -Lvendor/raylib/lib -lraylib -lgdi32 -lwinmm -o target/game.exe