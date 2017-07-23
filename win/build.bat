@echo off

SET CUR_DR=%cd%
nuget restore .\assetkit.sln

cd ..

git submodule -q update --init --recursive 2> $null

echo.
echo Build libuv
echo.

cd lib\libuv
git clone -q https://chromium.googlesource.com/external/gyp.git build/gyp 2> $null
call .\vcbuild.bat

echo.
echo Build libds
echo.

cd %CUR_DR%\..\lib\libds\win
call .\build.bat

cd %CUR_DR%

msbuild assetkit.vcxproj /p:Configuration=Debug
