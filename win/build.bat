@echo off

set vswhere=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe
for /f "usebackq tokens=1* delims=: " %%i in (`"%vswhere%" -latest -requires Microsoft.Component.MSBuild`) do (
  if /i "%%i"=="installationPath" set InstallDir=%%j
)

SET CUR_DR=%cd%
nuget restore .\assetkit.sln

cd ..

git submodule -q update --init --recursive 2> $null

echo.
echo Build libds
echo.

cd %CUR_DR%\..\lib\libds\win
call .\build.bat

echo.
echo Build libuv
echo.

cd %CUR_DR%\..\lib\libuv
git clone -q https://chromium.googlesource.com/external/gyp.git build/gyp 2> $null
.\vcbuild.bat

if exist "%InstallDir%\VC\Auxiliary\Build\vcvars64.bat" (
  call "%InstallDir%\VC\Auxiliary\Build\vcvars64.bat"
)

cd %CUR_DR%

msbuild assetkit.vcxproj /p:Configuration=Debug
