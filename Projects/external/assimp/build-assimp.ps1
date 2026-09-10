param(
  [ValidateSet("Debug","Release","RelWithDebInfo","MinSizeRel")]
  [string]$Configuration = "Debug",

  [ValidateSet("x64","Win32")]
  [string]$Platform = "x64",

  # Pass $(SolutionDir) from Visual Studio. If not passed, we infer repo root.
  [string]$RootDir = "",

  # Pass $(PlatformToolset) or $(DefaultPlatformToolset)
  # Examples: v145, v143, v143,host=x64
  [string]$Toolset = "",
  [string]$OutDir = "",

  [switch]$Fresh,
  [switch]$Clean
)

$ErrorActionPreference = "Stop"

function Invoke-Checked {
  param(
    [Parameter(Mandatory=$true)][string]$Exe,
    [Parameter(Mandatory=$true)][string[]]$Args
  )
  Write-Host "> $Exe $($Args -join ' ')" -ForegroundColor DarkGray
  & $Exe @Args
  if ($LASTEXITCODE -ne 0) { throw "$Exe failed with exit code $LASTEXITCODE" }
}

function Get-CMakeVersion {
  $line = (& cmake --version)[0]
  if ($line -match 'cmake version ([0-9]+\.[0-9]+\.[0-9]+)') {
    return [version]$matches[1]
  }
  throw "Couldn't parse CMake version from: $line"
}

function Resolve-VSGeneratorFromToolset([string]$ToolsetText) {
  if ([string]::IsNullOrWhiteSpace($ToolsetText)) {
    throw "Toolset not provided. Pass -Toolset `$(PlatformToolset) (or `$(DefaultPlatformToolset)) from Visual Studio."
  }

  # Strip suffix (e.g. 'v143,host=x64' -> base 'v143')
  $base = $ToolsetText
  if ($base -match '^(v\d+)\b') { $base = $matches[1] }

  switch ($base) {
    "v145" { return @{ Generator = "Visual Studio 18 2026"; Base = "v145" } }
    "v143" { return @{ Generator = "Visual Studio 17 2022"; Base = "v143" } }
    default { throw "Unsupported toolset '$ToolsetText' (base '$base'). Expected v145 or v143." }
  }
}

# --- x64-only guard ---
if ($Platform -ne "x64") {
  throw "This repo is intended for x64 only. (Got Platform='$Platform')"
}

# Script folder (where this ps1 lives)
$proj = $PSScriptRoot  # e.g. ...\Projects\assimp (recommended location)

$RootDir = $RootDir.Trim().Trim('"')
if ($RootDir.EndsWith("\")) { $RootDir += "." }  # avoid trailing \" quoting issues

# Repo root
if ([string]::IsNullOrWhiteSpace($RootDir)) {
  $root = (Resolve-Path (Join-Path $proj "..\..")).Path
} else {
  $root = (Resolve-Path $RootDir).Path
}

# Assimp source (submodule)
$src = Join-Path $root "external\assimp"

$OutDir = $OutDir.Trim().Trim('"')
if ($OutDir.EndsWith("\")) { $OutDir += "." }

if ([string]::IsNullOrWhiteSpace($OutDir)) {
  throw "OutDir not provided. Pass -OutDir `"$(OutDir)`" from the vcxproj."
}

# Resolve relative or absolute OutDir safely
if ([System.IO.Path]::IsPathRooted($OutDir)) {
  $finalLibDir = [System.IO.Path]::GetFullPath($OutDir)
} else {
  $finalLibDir = [System.IO.Path]::GetFullPath((Join-Path $root $OutDir))
}

$finalLibName = if ($Configuration -eq "Debug") { "assimp_d.lib" } else { "assimp.lib" }
$finalLib     = Join-Path $finalLibDir $finalLibName

# Where we want generated headers to live (stable, not under external/)
$genDstDir = Join-Path $root "Projects\assimp\include\assimp"
$finalCfg  = Join-Path $genDstDir "config.h"

# Decide VS generator/toolset base
$sel = Resolve-VSGeneratorFromToolset $Toolset
$vsGen       = $sel.Generator
$toolsetBase = $sel.Base

# Build tree isolated by platform+toolset
$bld = Join-Path $proj "build\assimp\$Platform\$toolsetBase"

# Build artifacts (don’t pollute final lib folder with assimp-vcXXX-*.lib)
$artDebug         = Join-Path $bld "_artifacts\Debug"
$artRelease       = Join-Path $bld "_artifacts\Release"
$artRelWithDebInfo= Join-Path $bld "_artifacts\RelWithDebInfo"
$artMinSizeRel    = Join-Path $bld "_artifacts\MinSizeRel"

# CMake 4.2+ required for "Visual Studio 18 2026"
$cmakeVer = Get-CMakeVersion
if ($vsGen -eq "Visual Studio 18 2026" -and $cmakeVer -lt [version]"4.2.0") {
  throw "CMake $cmakeVer is too old for generator '$vsGen'. Install CMake 4.2+."
}

# Ensure submodule exists
if (!(Test-Path (Join-Path $src "CMakeLists.txt"))) {
  Push-Location $root
  & git submodule update --init --recursive
  if ($LASTEXITCODE -ne 0) { throw "git submodule update failed with exit code $LASTEXITCODE" }
  Pop-Location
}

# --- Clean mode ---
if ($Clean) {
  Write-Host "Cleaning assimp build + generated outputs..." -ForegroundColor Yellow
  Remove-Item -Recurse -Force -ErrorAction SilentlyContinue $bld
  Remove-Item -Recurse -Force -ErrorAction SilentlyContinue (Join-Path $root "Projects\assimp\include")
  Remove-Item -Force -ErrorAction SilentlyContinue $finalLib
  return
}

# --- Fast skip if up-to-date (based on submodule SHA + settings) ---
# This prevents even running CMake/MSBuild when nothing changed.
$stampFile = Join-Path $bld "assimp.stamp.txt"
& git -C $src rev-parse HEAD | Out-Null
if ($LASTEXITCODE -ne 0) { throw "git rev-parse failed in $src (exit $LASTEXITCODE)" }
$assimpSha = (& git -C $src rev-parse HEAD).Trim()

$stampText = @(
  "sha=$assimpSha"
  "cfg=$Configuration"
  "plat=$Platform"
  "toolset=$Toolset"
  "gen=$vsGen"
) -join "`n"

if (-not $Fresh) {
  if ((Test-Path $finalLib) -and (Test-Path $finalCfg) -and (Test-Path $stampFile)) {
    $oldStamp = Get-Content $stampFile -Raw
    if ($oldStamp -eq $stampText) {
      Write-Host "Assimp up-to-date; skipping." -ForegroundColor Green
      return
    }
  }
}

$inject = Join-Path $proj "assimp_inject.cmake"
if (!(Test-Path $inject)) { throw "Missing inject file: $inject" }
$injectCmake = $inject.Replace('\','/')

# Ensure output dirs exist
Invoke-Checked cmake @("-E","make_directory",$finalLibDir)
Invoke-Checked cmake @("-E","make_directory",$genDstDir)
Invoke-Checked cmake @("-E","make_directory",$artDebug)
Invoke-Checked cmake @("-E","make_directory",$artRelease)
Invoke-Checked cmake @("-E","make_directory",$artRelWithDebInfo)
Invoke-Checked cmake @("-E","make_directory",$artMinSizeRel)

# Configure args
$cfgArgs = @(
  "-S", $src,
  "-B", $bld,
  "-G", $vsGen,
  "-A", $Platform,
  "-T", $Toolset,

  "-DBUILD_SHARED_LIBS=OFF",
  "-DASSIMP_BUILD_ASSIMP_TOOLS=OFF",
  "-DASSIMP_BUILD_SAMPLES=OFF",
  "-DASSIMP_BUILD_TESTS=OFF",
  "-DASSIMP_INSTALL=OFF",

  "-DCMAKE_PROJECT_Assimp_INCLUDE:FILEPATH=$injectCmake",

  # Send actual build artifacts to internal folder (keeps final lib folder clean)
  "-DCMAKE_ARCHIVE_OUTPUT_DIRECTORY=$artDebug",                # fallback
  "-DCMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG=$artDebug",
  "-DCMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE=$artRelease",
  "-DCMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELWITHDEBINFO=$artRelWithDebInfo",
  "-DCMAKE_ARCHIVE_OUTPUT_DIRECTORY_MINSIZEREL=$artMinSizeRel"
)

if ($Fresh) {
  $cfgArgs = @("--fresh") + $cfgArgs
}

# Configure
Invoke-Checked cmake $cfgArgs

# Copy generated headers (from build tree -> Projects\assimp\include\assimp)
$genSrcDir = Join-Path $bld "include\assimp"
$genConfig = Join-Path $genSrcDir "config.h"
$genRev    = Join-Path $genSrcDir "revision.h"

if (!(Test-Path $genConfig)) {
  throw "Assimp didn't generate config.h at '$genConfig'. Configure failed or build dir is wrong."
}

Invoke-Checked cmake @("-E","copy_if_different",$genConfig,(Join-Path $genDstDir "config.h"))
if (Test-Path $genRev) {
  Invoke-Checked cmake @("-E","copy_if_different",$genRev,(Join-Path $genDstDir "revision.h"))
}

Write-Host "Generated headers copied to: $genDstDir"

# Build (incremental; will do nothing if up-to-date)
Invoke-Checked cmake @("--build",$bld,"--config",$Configuration,"--target","assimp")
# Find the produced assimp library in the configuration’s artifacts dir
$cfgArtDir = Join-Path $bld "_artifacts\$Configuration"
$built = Get-ChildItem $cfgArtDir -Filter "assimp*.lib" -ErrorAction SilentlyContinue |
         Sort-Object LastWriteTime -Descending |
         Select-Object -First 1
if (-not $built) {
  throw "No assimp*.lib found in $cfgArtDir. Build output wasn't produced."
}

# Copy to canonical name without timestamp churn
Invoke-Checked cmake @("-E","copy_if_different",$built.FullName,$finalLib)

# Copy zlibstatic libs (preserve original filenames, e.g. zlibstaticd.lib)
$zlibs = Get-ChildItem $cfgArtDir -Filter "zlibstatic*.lib" -ErrorAction SilentlyContinue
if ($zlibs -and $zlibs.Count -gt 0) {
  foreach ($zl in $zlibs) {
    $dst = Join-Path $finalLibDir $zl.Name
    Invoke-Checked cmake @("-E","copy_if_different",$zl.FullName,$dst)
    Write-Host "Also copied: $dst"
  }
} else {
  Write-Host "Note: no zlibstatic*.lib found in $cfgArtDir (assimp may not be using bundled zlib in this config)."
}

# Write stamp (after outputs are in place)
Invoke-Checked cmake @("-E","make_directory",$bld)
Set-Content -Path $stampFile -Value $stampText -Encoding ASCII

Write-Host "Done: $finalLib"

