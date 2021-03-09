@echo off
if not exist ./tools/rynx-codegen/libclang.dll (
	echo libclang.dll not found.
	echo download llvm 11.0.0 prebuilt binaries and
	echo place libclang.dll to ./tools/rynx-codegen/
	pause
	exit 1
)

if exist ./tools/rynx-codegen/rynx-codegen.exe (
	"./tools/rynx-codegen/rynx-codegen.exe" -I ./src/ --std=c++20 -filename-ends-with components.hpp -gen-for-namespace ::rynx::components -filename-contains components.hpp
) else (
	echo rynx-codegen.exe not found. compile the codegen project first.
	pause
	exit 1
)