SET CUR_DR=%cd%
nuget restore .\assetkit.sln

cd ..

git submodule update --init --recursive

cd lib\libuv
git clone https://chromium.googlesource.com/external/gyp.git build/gyp
.\vcbuild.bat

cd %CUR_DR%

devenv assetkit.sln /Build Release
