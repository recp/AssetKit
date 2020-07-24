@echo off

set vswhere=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe
for /f "usebackq tokens=1* delims=: " %%i in (`"%vswhere%" -latest -requires Microsoft.Component.MSBuild`) do (
  if /i "%%i"=="installationPath" set InstallDir=%%j
)

SET CUR_DR=%cd%

cd ..

git submodule -q update --init --recursive 2> $null

echo.
echo Build libds
echo.

cd %CUR_DR%\..\deps\ds\win
call .\build.bat

cd %CUR_DR%

msbuild assetkit.vcxproj /p:Configuration=Debug
