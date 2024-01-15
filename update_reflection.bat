@echo off
if not exist ./tools/rynx-codegen/libclang.dll (
	echo libclang.dll not found.
	echo download llvm 17.0.1 prebuilt binaries and
	echo place libclang.dll to ./tools/rynx-codegen/
	pause
	exit 1
)

if exist ./tools/rynx-codegen/rynx-codegen.exe (
	"./tools/rynx-codegen/rynx-codegen.exe" -I ./src/ -custom -fdelayed-template-parsing -DApplicationDLL= -DAudioDLL= -DEcsDLL= -DEditorDLL= -DFileSystemDLL= -DGraphicsDLL= -DInputDLL= -DMathDLL= -DMenuDLL= -DProfilingDLL= -DReflectionDLL= -DRulesetsDLL= -DSchedulerDLL= -DRynxStdDLL= -DSystemDLL= -DTechDLL= -DThreadDLL= --std=c++20 -filename-ends-with components.hpp -gen-for-namespace ::rynx::components -filename-contains components.hpp
) else (
	echo rynx-codegen.exe not found. compile the codegen project first.
	pause
	exit 1
)