@echo off
./update_reflection.bat
"./tools/sharpmake/bootstrap.bat" "../../generate/main.sharpmake.cs"
@if %errorlevel% neq 0 goto :error

@goto :exit
:error
@echo "========================= ERROR ========================="
@pause

:exit

@echo Solution generation completed with SUCCESS
