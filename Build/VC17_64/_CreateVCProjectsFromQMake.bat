echo on


set COMPILER_EXT=VC17_64
set QMAKESPEC=%QTDIR%\mkspecs\win32-msvc
set path=%path%;%QTDIR%\bin

echo Generating %COMPILER_EXT% projects...
echo %QTDIR%
cd %AGENTINODIR%\Build\QMake

%QTDIR%\bin\qmake AgentinoAll.pro -recursive -tp vc


cd %~dp0\..\..
call %ACFCONFIGDIR%\QMake\CopyVCProjToSubdir.js %COMPILER_EXT%

cd %~dp0\
