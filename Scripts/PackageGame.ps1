param(
	[Parameter(Mandatory = $true)]
	[ValidateSet("Production", "QA")]
	[string] $Channel,

	[ValidateSet("Development", "Test", "Shipping")]
	[string] $Configuration,

	[Parameter(Mandatory = $true)]
	[string] $EnginePath,

	[string] $OutputPath
)

$ErrorActionPreference = "Stop"
$ProjectRoot = Split-Path -Parent $PSScriptRoot
$ProjectFile = Join-Path $ProjectRoot "ProjectAtlantis.uproject"
$UatPath = Join-Path $EnginePath "Engine/Build/BatchFiles/RunUAT.bat"

if (!$Configuration)
{
	$Configuration = if ($Channel -eq "QA") { "Development" } else { "Shipping" }
}
if ($Channel -eq "Production" -and $Configuration -ne "Shipping")
{
	throw "Production requires Shipping so development cheats are disabled."
}
if ($Channel -eq "QA" -and $Configuration -eq "Shipping")
{
	throw "QA requires Development or Test so the debug menu is available."
}
if (!(Test-Path -LiteralPath $UatPath -PathType Leaf))
{
	throw "Unreal Automation Tool not found: $UatPath"
}
if (!$OutputPath)
{
	$OutputPath = Join-Path $ProjectRoot "Saved/Packages/$Channel"
}

Write-Host "Packaging $Channel ($Configuration) to $OutputPath"
$UatArguments = @(
	"BuildCookRun", "-project=$ProjectFile", "-noP4", "-platform=Win64",
	"-clientconfig=$Configuration", "-serverconfig=$Configuration", "-nodebuginfo",
	"-nocompileeditor", "-skipbuildeditor", "-cook", "-allmaps", "-build", "-stage",
	"-pak", "-archive", "-archivedirectory=$OutputPath"
)
& $UatPath @UatArguments
if ($LASTEXITCODE -ne 0)
{
	throw "Packaging failed with exit code $LASTEXITCODE."
}
