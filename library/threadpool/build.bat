@echo off
SETLOCAL

set COMPILER_FLAGS=-c

set INCLUDE=
set LIB=

set LINK=

set SRC_DIR=src
set LIB_DIR=lib

set LIBRARY=libthreadpool.a

set OUTPUT=^
    threadpool.o

set COMPILE=^
    %SRC_DIR%/threadpool.c

mkdir %LIB_DIR%

@echo on

gcc %COMPILER_FLAGS% %COMPILE% %INCLUDE% %LIB% %LINK%
ar -rcs %LIB_DIR%/%LIBRARY% %OUTPUT%

@echo off

for %%o in (%OUTPUT%) do del %%o
