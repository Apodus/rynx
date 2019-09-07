@echo off
cd generate
"./sharpmake/Sharpmake.Application.exe" /sources(@"main.sharpmake.cs")
cd ..
@if %errorlevel% neq 0 goto :error

@goto :exit
:error
@echo "========================= ERROR ========================="
@pause

:exit

@echo Solution generation completed with SUCCESS
