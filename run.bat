@echo off
setlocal

set "config=%~1"
if not defined config set "config=relwithdebinfo"

if /I "%config%"=="debug" (
	set "build_dir=build\debug"
) else if /I "%config%"=="release" (
	set "build_dir=build\release"
) else if /I "%config%"=="relwithdebinfo" (
	set "build_dir=build\relwithdebinfo"
) else (
	echo Unknown configuration: %config%
	echo Valid configurations: debug, release, relwithdebinfo
	exit /b 1-
)

pushd %build_dir%
"Triple Triad Solver.exe"
popd

set "result=%ERRORLEVEL%"
endlocal & exit /b %result%
