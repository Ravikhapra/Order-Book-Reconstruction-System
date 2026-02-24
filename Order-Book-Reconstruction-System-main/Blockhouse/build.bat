build.bat
@echo off
echo Building MBO to MBP-10 Reconstruction Program...

REM Create object directory if it doesn't exist
if not exist obj mkdir obj

REM Compile source files
echo Compiling orderbook.cpp...
g++ -std=c++17 -Wall -Wextra -O3 -march=native -Iinclude -c src/orderbook.cpp -o obj/orderbook.o
if %errorlevel% neq 0 (
    echo Error compiling orderbook.cpp
    exit /b 1
)

echo Compiling main.cpp...
g++ -std=c++17 -Wall -Wextra -O3 -march=native -Iinclude -c src/main.cpp -o obj/main.o
if %errorlevel% neq 0 (
    echo Error compiling main.cpp
    exit /b 1
)

REM Link executable
echo Linking executable...
g++ -std=c++17 -Wall -Wextra -O3 -march=native -o reconstruction.exe obj/orderbook.o obj/main.o
if %errorlevel% neq 0 (
    echo Error linking executable
    exit /b 1
)

echo Build completed successfully!
echo Executable: reconstruction.exe
echo.
echo Usage: reconstruction.exe data\mbo.csv
echo.
