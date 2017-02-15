SET CUR_DR=%cd%
nuget restore .\assetkit.sln

cd ..

git submodule -q update --init --recursive 2> $null

cd lib\libuv
git clone -q https://chromium.googlesource.com/external/gyp.git build/gyp 2> $null
.\vcbuild.bat 2> $null

cd %CUR_DR%

msbuild assetkit.vcxproj /p:Configuration=Release
