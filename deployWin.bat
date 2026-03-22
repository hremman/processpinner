@echo off
setlocal enabledelayedexpansion

cd build
"C:\Program Files\CQtDeployer\1.6\CQtDeployer.exe" -bin .\Release\release\ProcessPinner.exe -qmake "C:\Qt\6.7.0\mingw_64\bin\qmake.exe" -targetDir ".\ProcessPinner"
rem mkdir trs
rem move ".\ProcessPinner\translations\qtbase_ru.qm" .\trs\
rem move ".\ProcessPinner\translations\qtbase_en.qm" .\trs\
del /F /Q .\ProcessPinner\translations
copy ..\changelog .\ProcessPinner\changelog.txt





for /f "tokens=2 delims==" %%a in ('findstr /R "^VERSION" "..\ProcessPinner.pro"') do (
    set "RAW_VER=%%a"
    
    :: Удаляем кавычки
    set "VER=!RAW_VER:"=!"
    :: Удаляем лишние пробелы (trim)
    for /f "tokens=*" %%b in ("!VER!") do set "VER=%%b"
)

echo Found version: %VER%
"C:\Program Files\7-Zip\7z.exe" a "ProcessPinner_%VER%.7z" .\ProcessPinner
del /Q /F Release
