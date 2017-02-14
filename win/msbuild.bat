SET CUR_DR=%cd%
Update-Package -ProjectName assetkit

cd ..\lib\libuv\
git clone https://chromium.googlesource.com/external/gyp.git build/gyp
.\vcbuild.bat

cd %CUR_DR%

msbuild assetkit.vcxproj /p:Configuration=All
