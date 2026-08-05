@echo off
REM Defaults (for Ricchio)
set BASEDIR=C:\ASGC\GameJam2026\PackagedBuilds

REM Get directory and version info
echo *** GET DIRECTORY AND VERSION INFO ***
echo Where do you want to create the package? (must match default below)
echo [default is C:\ASGC\GameJam2026\PackagedBuilds]
set /p BASEDIR="Enter the full path to your package builds directory: "
echo PACKAGEBUILDS DIR IS (%BASEDIR%)
echo.
echo What is the GitHub tag to create/use for this package? (e.g. 0.0.1)
set /p VER="Enter the GitHub tag (i.e. version): "
set PACKAGENAME=ProjectAtlantis-%VER%-Windows
set FULLDIR=%BASEDIR%\%PACKAGENAME%
echo FULL PATH FOR THIS PACKAGE IS (%FULLDIR%)
echo.
echo WARNING! If the directory above does not look correct, close this window!
echo.
pause

REM Create Package Directory, if it doesn't exist
if not exist "%FULLDIR%\" mkdir "%FULLDIR%"
echo.

REM Create the Package using UAT
echo *** CREATE PACKAGE WITH UAT ***
set CMD=cmd.exe /c ""C:/Program Files/Epic Games/UE_5.7/Engine/Build/BatchFiles/RunUAT.bat"  -ScriptsForProject="C:/GitHub/ASGC-Game-Jam/asgc-gamejam-2026/ProjectAtlantis.uproject" Turnkey -command=VerifySdk -platform=Win64 -UpdateIfNeeded -EditorIO -EditorIOPort=54650  -project="C:/GitHub/ASGC-Game-Jam/asgc-gamejam-2026/ProjectAtlantis.uproject" BuildCookRun -nop4 -utf8output -nocompileeditor -skipbuildeditor -cook  -project="C:/GitHub/ASGC-Game-Jam/asgc-gamejam-2026/ProjectAtlantis.uproject"  -unrealexe="C:\Program Files\Epic Games\UE_5.7\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" -platform=Win64 -installed -SkipCookingErrorSummary -JsonStdOut -stage -archive -package -build -pak -iostore -compressed -prereqs -archivedirectory="C:\ASGC\GameJam2026\PackagedBuilds\ProjectAtlantis-0.0.1-Windows\Windows" -clientconfig=Shipping -nodebuginfo" -nocompile -nocompileuat
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