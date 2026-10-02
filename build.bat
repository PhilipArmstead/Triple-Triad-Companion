@echo off
setlocal

set "config=%~1"
if not defined config set "config=debug"

if /I "%config%"=="debug" (
	set "build_dir=build\debug"
	set "compile_options=/Zi /Od /FC"
	set "subsystem=/SUBSYSTEM:CONSOLE"
) else if /I "%config%"=="release" (
	set "build_dir=build\release"
	set "compile_options=/O2"
	set "subsystem=/SUBSYSTEM:CONSOLE"
) else if /I "%config%"=="relwithdebinfo" (
	set "build_dir=build\relwithdebinfo"
	set "compile_options=/Zi /O2 /FC"
	set "subsystem=/SUBSYSTEM:CONSOLE"
) else (
	echo Unknown configuration: %config%
	echo Valid configurations: debug, release, relwithdebinfo
	exit /b 1
)

mkdir "%build_dir%" 2>nul

pushd "%build_dir%" || (
	echo Could not enter "%build_dir%"
	exit /b 1
)

cl %compile_options% ^
	..\..\src\*.c ^
	/Fd:"game.pdb" ^
	/Fe:"game.exe" ^
	/link %subsystem% /PDB:"game.pdb"

set "result=%ERRORLEVEL%"
popd
endlocal & exit /b %result%
