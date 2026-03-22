mkdir build\Release
cd build\Release
"C:\Qt\6.7.0\mingw_64\bin\qmake.exe" "CONFIG-=debug_and_release" "CONFIG-=debug" "CONFIG+=release" ..\..\ProcessPinner.pro
"c:\Qt\Tools\mingw1120_64\bin\mingw32-make.exe" -j 4
mkdir release
move ProcessPinner.exe release\ProcessPinner.exe
cd ..\..
