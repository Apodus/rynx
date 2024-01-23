@echo off
call ./update_reflection.bat
echo yup yup
@if %errorlevel% neq 0 goto :error

pushd %CD%

cd ./tools/Sharpmake/
dotnet build Sharpmake.sln -c Release
set SHARPMAKE_EXECUTABLE=%CD%/Sharpmake.Application\bin\Release\net6.0\Sharpmake.Application.exe
if not exist %SHARPMAKE_EXECUTABLE% echo Cannot find sharpmake executable in %SHARPMAKE_EXECUTABLE% & pause & goto error
echo Using executable %SHARPMAKE_EXECUTABLE%

popd

echo Updating game_one solution file
call %SHARPMAKE_EXECUTABLE% /sources(@'./generate/main.sharpmake.cs') /outputdir(@'./') /remaproot(@'./') /verbose

@if %errorlevel% neq 0 goto :error

@goto :exit
:error
@echo "========================= ERROR ========================="
@pause

:exit

@echo Solution generation completed with SUCCESS