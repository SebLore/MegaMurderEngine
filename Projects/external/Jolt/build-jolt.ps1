param(
  [ValidateSet("Debug","Release","RelWithDebInfo","MinSizeRel")]
  [string]$Configuration = "Debug",

  [ValidateSet("x64","Win32")]
  [string]$Platform = "x64",

  # Should pass $(SolutionDir) from Visual Studio. If omitted, repo root is inferred from script location.
  [string]$RootDir = "",

  # Should pass $(DefaultPlatformToolset) or $(PlatformToolset), e.g. v145 / v143 / v143,host=x64
  [string]$Toolset = "",

  # Should pass $(OutDir) so Jolt.lib is emitted directly to the chosen output folder.
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
    throw "Toolset not provided. Pass -Toolset `$(DefaultPlatformToolset) or `$(PlatformToolset) from Visual Studio."
  }

  # Strip suffix (e.g. 'v143,host=x64' -> 'v143')
  $base = $ToolsetText
  if ($base -match '^(v\d+)\b') { $base = $matches[1] }

  switch ($base) {
    "v145" { return @{ Generator = "Visual Studio 18 2026"; Base = "v145" } }
    "v143" { return @{ Generator = "Visual Studio 17 2022"; Base = "v143" } }
    default { throw "Unsupported toolset '$ToolsetText' (base '$base'). Expected v145 or v143." }
  }
}

function Get-FullPathSafe([string]$PathText, [string]$BaseDir) {
  if ([string]::IsNullOrWhiteSpace($PathText)) { return $null }

  $trimmed = $PathText.Trim().Trim('"')
  if ([string]::IsNullOrWhiteSpace($trimmed)) { return $null }

  # Avoid trailing \" quoting issues from MSBuild/NMake
  if ($trimmed.EndsWith("\")) {
    $trimmed += "."
  }

  if ([System.IO.Path]::IsPathRooted($trimmed)) {
    return [System.IO.Path]::GetFullPath($trimmed)
  }

  return [System.IO.Path]::GetFullPath((Join-Path $BaseDir $trimmed))
}

function Resolve-JoltRoot([string]$RepoRoot) {
  $candidates = @(
    (Join-Path $RepoRoot "external\JoltPhysics"),
    (Join-Path $RepoRoot "external\Jolt"),
    (Join-Path $RepoRoot "external\jolt")
  )

  foreach ($cand in $candidates) {
    $cmakeFile = Join-Path $cand "Build\CMakeLists.txt"
    if (Test-Path $cmakeFile) {
      return (Resolve-Path $cand).Path
    }
  }

  throw "Could not find JoltPhysics source. Expected one of:`n - external\JoltPhysics`n - external\Jolt`n - external\jolt`n...with Build\CMakeLists.txt present."
}

# x64-only guard
if ($Platform -ne "x64") {
  throw "This wrapper is configured for x64 only. (Got Platform='$Platform')"
}

# Script folder (e.g. ...\Projects\Jolt)
$proj = $PSScriptRoot

# Resolve repo root
$RootDir = $RootDir.Trim().Trim('"')
if ($RootDir.EndsWith("\")) { $RootDir += "." }  # avoid trailing \" quoting issues

if ([string]::IsNullOrWhiteSpace($RootDir)) {
  $root = (Resolve-Path (Join-Path $proj "..\..")).Path
} else {
  $root = (Resolve-Path $RootDir).Path
}

# Resolve Jolt source root and CMake source dir
$joltRoot = Resolve-JoltRoot $root
$src      = Join-Path $joltRoot "Build"

# Decide final output directory (where Jolt.lib should be produced)
# Prefer -OutDir (from VS); otherwise use a sane default under repo root.
$outLibDir = Get-FullPathSafe $OutDir $root
if ([string]::IsNullOrWhiteSpace($outLibDir)) {
  $outLibDir = Join-Path $root "lib\$Configuration\$Platform"
}
$finalLibName = if ($Configuration -eq "Debug") { "Jolt_d.lib" } else { "Jolt.lib" }
$finalLib     = Join-Path $outLibDir $finalLibName

# Decide VS generator/toolset base
$sel = Resolve-VSGeneratorFromToolset $Toolset
$vsGen       = $sel.Generator
$toolsetBase = $sel.Base

# Build tree isolated by platform+toolset+configuration (keep generated CMake VS files here)
$bld = Join-Path $proj "build\jolt\$Platform\$toolsetBase\$Configuration"

# CMake 4.2+ required for "Visual Studio 18 2026"
$cmakeVer = Get-CMakeVersion
if ($vsGen -eq "Visual Studio 18 2026" -and $cmakeVer -lt [version]"4.2.0") {
  throw "CMake $cmakeVer is too old for generator '$vsGen'. Install CMake 4.2+."
}

# Ensure Jolt submodule/source exists (optional convenience)
if (!(Test-Path (Join-Path $src "CMakeLists.txt"))) {
  Push-Location $root
  & git submodule update --init --recursive
  if ($LASTEXITCODE -ne 0) { throw "git submodule update failed with exit code $LASTEXITCODE" }
  Pop-Location
  if (!(Test-Path (Join-Path $src "CMakeLists.txt"))) {
    throw "Jolt source still missing after submodule update: $src"
  }
}

# --- Clean mode ---
if ($Clean) {
  Write-Host "Cleaning Jolt build tree and outputs..." -ForegroundColor Yellow

  # Remove generated CMake build tree (generated .sln/.vcxproj live here)
  Remove-Item -Recurse -Force -ErrorAction SilentlyContinue $bld

  # Remove final artifacts in chosen output folder
  Remove-Item -Force -ErrorAction SilentlyContinue $finalLib
  Remove-Item -Force -ErrorAction SilentlyContinue (Join-Path $outLibDir "Jolt.pdb")
  Remove-Item -Force -ErrorAction SilentlyContinue (Join-Path $outLibDir "Jolt.ilk")

  return
}


# Flags based on configuration
# Floating point exceptions only enabled for debug
$fpe = if ($Configuration -eq "Debug") { "ON" } else { "OFF" }

# --- Up-to-date stamp (fast skip) ---
$stampFile = Join-Path $bld "jolt.stamp.txt"

# SHA of submodule/worktree HEAD (fast, stable)
& git -C $joltRoot rev-parse HEAD | Out-Null
if ($LASTEXITCODE -ne 0) { throw "git rev-parse failed in $joltRoot (exit $LASTEXITCODE)" }
$joltSha = (& git -C $joltRoot rev-parse HEAD).Trim()


$stampText = @(
  "sha=$joltSha"
  "cfg=$Configuration"
  "plat=$Platform"
  "toolset=$Toolset"
  "gen=$vsGen"
  "out=$outLibDir"
  "fpe=$fpe",
  # Important CMake options affecting target graph / outputs:
  "target=Jolt"
  "BUILD_SHARED_LIBS=OFF"
  "TARGET_UNIT_TESTS=OFF"
  "TARGET_HELLO_WORLD=OFF"
  "TARGET_PERFORMANCE_TEST=OFF"
  "TARGET_SAMPLES=OFF"
  "TARGET_VIEWER=OFF"
  "ENABLE_INSTALL=OFF"
) -join "`n"

if (-not $Fresh) {
  if ((Test-Path $finalLib) -and (Test-Path $stampFile)) {
    $oldStamp = Get-Content $stampFile -Raw
    if ($oldStamp -eq $stampText) {
      Write-Host "Jolt up-to-date; skipping." -ForegroundColor Green
      return
    }
  }
}



# Ensure final output dir exists
Invoke-Checked cmake @("-E","make_directory",$outLibDir)

# Configure args
# We configure output directories so the built Jolt.lib lands directly in $outLibDir (no post-copy).
$cfgArgs = @(
  "-S", $src,
  "-B", $bld,
  "-G", $vsGen,
  "-A", $Platform,
  "-T", $Toolset,

  "-DUSE_STATIC_MSVC_RUNTIME_LIBRARY=OFF",
  "-DBUILD_SHARED_LIBS=OFF",
  "-DENABLE_INSTALL=OFF",
  "-DFLOATING_POINT_EXCEPTIONS_ENABLED=$fpe",

  "-DTARGET_UNIT_TESTS=OFF",
  "-DTARGET_HELLO_WORLD=OFF",
  "-DTARGET_PERFORMANCE_TEST=OFF",
  "-DTARGET_SAMPLES=OFF",
  "-DTARGET_VIEWER=OFF",

  "-DCMAKE_DEBUG_POSTFIX=_d",
  "-DCMAKE_RELEASE_POSTFIX=",
  "-DCMAKE_RELWITHDEBINFO_POSTFIX=",
  "-DCMAKE_MINSIZEREL_POSTFIX=",

  "-DCMAKE_ARCHIVE_OUTPUT_DIRECTORY=$outLibDir",
  "-DCMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG=$outLibDir",
  "-DCMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE=$outLibDir",
  "-DCMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELWITHDEBINFO=$outLibDir",
  "-DCMAKE_ARCHIVE_OUTPUT_DIRECTORY_MINSIZEREL=$outLibDir",

  "-DCMAKE_RUNTIME_OUTPUT_DIRECTORY=$outLibDir",
  "-DCMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG=$outLibDir",
  "-DCMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE=$outLibDir",
  "-DCMAKE_RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO=$outLibDir",
  "-DCMAKE_RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL=$outLibDir"
)

if ($Fresh) {§
  $cfgArgs = @("--fresh") + $cfgArgs
}

# Configure (regenerates generated .sln/.vcxproj under $bld)
Invoke-Checked cmake $cfgArgs

# Build ONLY the Jolt target (not ALL_BUILD)
Invoke-Checked cmake @("--build", $bld, "--config", $Configuration, "--target", "Jolt")

# Validate output exists where we expect it
if (!(Test-Path $finalLib)) {
  throw "Build succeeded but '$finalLib' was not found. Check target output name/path in generated Jolt.vcxproj."
}

# Write stamp
Invoke-Checked cmake @("-E","make_directory",$bld)
Set-Content -Path $stampFile -Value $stampText -Encoding ASCII

Write-Host "Done: $finalLib" -ForegroundColor Green