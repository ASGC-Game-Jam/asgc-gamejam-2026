@echo off
echo.

REM Defaults
set UPROJECT=../ProjectAtlantis.uproject
for %%I in ("%UPROJECT%") do set "UPROJECT=%%~fI"
set UAT=C:\Program Files\Epic Games\UE_5.7\Engine\Build\BatchFiles\RunUAT.bat
set EDITOR=C:\Program Files\Epic Games\UE_5.7\Engine\Binaries\Win64\UnrealEditor-Cmd.exe
set BASEDIR=C:\ASGC\GameJam2026\PackagedBuilds
set VER=

REM Get directory and version info
echo *** GET DIRECTORY AND VERSION INFO ***
echo Where does your ProjectAtlantis.uproject live?
echo [default is %UPROJECT%]
set /p UPROJECT="Enter the full path to your ProjectAtlantis.uproject: "
echo UPROJECT IS (%UPROJECT%)
echo.
echo Where does your UAT.bat live?
echo [default is %UAT%]
set /p UAT="Enter the full path to your RunUAT.bat: "
echo UAT IS (%UAT%)
echo.
echo Where does your UnrealEditor-Cmd.exe live?
echo [default is %EDITOR%]
set /p EDITOR="Enter the full path to your UnrealEditor-Cmd.exe: "
echo EDITOR IS (%EDITOR%)
echo.
echo Where do you want to create the package? (must match default below)
echo [default is %BASEDIR%]
set /p BASEDIR="Enter the full path to your package builds directory: "
echo PACKAGE BUILDS DIR IS (%BASEDIR%)
echo.
echo What is the GitHub tag to create/use for this package? (e.g. 0.0.1)
set /p VER="Enter the GitHub tag (i.e. version): "
set PACKAGENAME=ProjectAtlantis-%VER%-Windows
set FULLDIR=%BASEDIR%\%PACKAGENAME%
echo FULL PATH FOR THIS PACKAGE IS (%FULLDIR%)
echo.
echo WARNING! If ANY of the info above does not look correct, close this window!
echo.
pause

REM Create Package Directory, if it doesn't exist
if not exist "%FULLDIR%\" mkdir "%FULLDIR%"
echo.

REM Create the Package using UAT
REM This is just a straight copy of the output log when building from the Editor.  No doubt it can be refined.
echo *** CREATE PACKAGE WITH UAT ***
set CMD=cmd.exe /c ""%UAT%"  -ScriptsForProject="%UPROJECT%" Turnkey -command=VerifySdk -platform=Win64 -UpdateIfNeeded -EditorIO -EditorIOPort=54650  -project="%UPROJECT%" BuildCookRun -nop4 -utf8output -nocompileeditor -skipbuildeditor -cook  -project="%UPROJECT%"  -unrealexe="%EDITOR%" -platform=Win64 -installed -SkipCookingErrorSummary -JsonStdOut -stage -archive -package -build -pak -iostore -compressed -prereqs -archivedirectory="%FULLDIR%\Windows" -clientconfig=Shipping -nodebuginfo" -nocompile -nocompileuat
echo UAT CMD IS: (%CMD%)
%CMD%
echo.

REM Remove nested Windows directory
echo *** REMOVE NESTED WINDOWS DIR ***
set CMD=robocopy "%FULLDIR%\Windows" "%FULLDIR%" /E /MOVE /NJH /NJS
echo (%CMD%)
%CMD%
echo.

REM Add splash screen
echo *** ADD SPLASH SCREEN IMAGE ***
set CMD=xcopy ..\Content\Splash\Splash.png "%FULLDIR%\ProjectAtlantis\Content\Splash\"
echo (%CMD%)
%CMD%
echo.

REM Clear out unneeded files
echo *** REMOVE UNNEEDED FILES ***
set CMD=del "%FULLDIR%\Manifest*_Win64.txt"
echo (%CMD%)
%CMD%
echo.

REM Create Zip
echo *** CREATE ZIP ARCHIVE ***
set CMD=powershell -Command "Compress-Archive -Path '%FULLDIR%\*' -DestinationPath '%BASEDIR%\%PACKAGENAME%.zip' -Force"
echo (%CMD%)
%CMD%
echo.

pause