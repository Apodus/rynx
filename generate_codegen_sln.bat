@echo off
pushd %CD%

cd ./tools/Sharpmake/
dotnet build Sharpmake.sln -c Release
set SHARPMAKE_EXECUTABLE=%CD%/Sharpmake.Application\bin\Release\net6.0\Sharpmake.Application.exe
if not exist %SHARPMAKE_EXECUTABLE% echo Cannot find sharpmake executable in %SHARPMAKE_EXECUTABLE% & pause & goto error
echo Using executable %SHARPMAKE_EXECUTABLE%

popd

echo Updating codegen solution file
call %SHARPMAKE_EXECUTABLE% /sources(@'./generate/codegen.sharpmake.cs') /outputdir(@'./') /remaproot(@'./') /verbose

@goto :exit
:error
@echo "========================= ERROR ========================="
@pause

:exit

@echo Solution generation completed with SUCCESS
