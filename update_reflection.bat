@echo off
if exist ./tools/rynx-codegen/rynx-codegen.exe (
	"./tools/rynx-codegen/rynx-codegen.exe" -I ./src/ --std=c++20 -filename-ends-with components.hpp -gen-for-namespace ::rynx::components -filename-contains components.hpp
) else (
	echo rynx-codegen.exe not found. compile the codegen project first.
	exit 1
)