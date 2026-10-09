@echo off
setlocal

set "config=%~1"
if not defined config set "config=relwithdebinfo"

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

powershell.exe -NoProfile -ExecutionPolicy Bypass -Command "$png = [IO.File]::ReadAllBytes('%~dp0assets\icon.png'); $stream = [IO.File]::Create('Triple Triad Solver.ico'); $writer = [IO.BinaryWriter]::new($stream); $writer.Write([UInt16]0); $writer.Write([UInt16]1); $writer.Write([UInt16]1); $writer.Write([Byte]0); $writer.Write([Byte]0); $writer.Write([Byte]0); $writer.Write([Byte]0); $writer.Write([UInt16]1); $writer.Write([UInt16]32); $writer.Write([UInt32]$png.Length); $writer.Write([UInt32]22); $writer.Write($png); $writer.Dispose()"
if errorlevel 1 (
	echo Could not create the icon resource
	popd
	exit /b 1
)

> app.rc (
	echo #define IDI_APP 101
	echo #define IDR_APP_PNG 102
	echo IDI_APP ICON "Triple Triad Solver.ico"
	echo IDR_APP_PNG RCDATA "../../assets/icon.png"
)
rc.exe /nologo /fo "app.res" "app.rc"
if errorlevel 1 (
	echo Could not compile the icon resources
	popd
	exit /b 1
)

> sources.rsp (
	for /r "%~dp0src" %%f in (*.c) do echo "%%f"
)

cl %compile_options% @sources.rsp ^
	/std:c17 ^
	/TC ^
	/W4 ^
	/I"%~dp0." ^
	/Fd:"Triple Triad Solver.pdb" ^
	/Fe:"Triple Triad Solver.exe" ^
	/link %subsystem% /PDB:"Triple Triad Solver.pdb" "app.res" User32.lib
set "result=%ERRORLEVEL%"

if not "%result%"=="0" goto :cleanup

> test_sources.rsp (
	echo "%~dp0tests\test_runner.c"
	for /r "%~dp0src" %%f in (*.c) do if /I not "%%~nxf"=="main.c" echo "%%f"
	for /r "%~dp0tests" %%f in (*.c) do if /I not "%%~nxf"=="test_runner.c" echo "%%f"
)

cl %compile_options% @test_sources.rsp ^
	/std:c17 ^
	/TC ^
	/W4 ^
	/I"%~dp0." ^
	/Fd:"Triple Triad Solver Tests.pdb" ^
	/Fe:"Triple Triad Solver Tests.exe" ^
	/link %subsystem% /PDB:"Triple Triad Solver Tests.pdb" User32.lib
set "result=%ERRORLEVEL%"

:cleanup
del sources.rsp
del test_sources.rsp 2>nul
popd
endlocal & exit /b %result%
