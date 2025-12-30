# ============================================================
# G:\browser\bootstrap_build.ps1
# - VS2022 env
# - real vcpkg at C:\vcpkg
# - Qt via QT_PREFIX_PATH (required)
# - verifies QtWebEngine exists
# - cmake configure/build/run
# ============================================================

$ErrorActionPreference = "Stop"

function Info($m) { Write-Host "[INFO] $m" }
function Warn($m) { Write-Host "[WARN] $m" }
function Fail($m) { throw $m }

# 0) Project root
$ProjectRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
if (!(Test-Path "$ProjectRoot\CMakeLists.txt")) { Fail "CMakeLists.txt not found. Script must be in G:\browser." }
Info "ProjectRoot = $ProjectRoot"

# 1) Git
if (!(Get-Command git -ErrorAction SilentlyContinue)) { Fail "git.exe not found. Install Git for Windows." }

# 2) VS Dev Env
$VsDevCmd = @(
  "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat",
  "C:\Program Files\Microsoft Visual Studio\2022\Professional\Common7\Tools\VsDevCmd.bat",
  "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\Common7\Tools\VsDevCmd.bat"
) | Where-Object { Test-Path $_ } | Select-Object -First 1

if (!$VsDevCmd) { Fail "VS2022 not found. Install Visual Studio 2022 with C++ workload." }

Info "Loading VS environment: $VsDevCmd"
cmd /c "`"$VsDevCmd`" -arch=amd64 -host_arch=amd64 && set" | ForEach-Object {
  if ($_ -match "^(.*?)=(.*)$") { [System.Environment]::SetEnvironmentVariable($matches[1], $matches[2]) }
}

if (!(Get-Command cl.exe -ErrorAction SilentlyContinue)) { Fail "cl.exe missing. Install VS C++ workload." }
Info "MSVC OK"

# 3) REAL vcpkg (ignore VS internal)
$VcpkgRoot = "C:\vcpkg"
if (!(Test-Path $VcpkgRoot)) {
  Info "Cloning vcpkg into C:\vcpkg"
  git clone https://github.com/microsoft/vcpkg.git $VcpkgRoot
} else {
  if (!(Test-Path "$VcpkgRoot\.git")) { Fail "C:\vcpkg exists but is not a vcpkg git repo." }
  Info "Using existing vcpkg at C:\vcpkg"
}

$Bootstrap = "$VcpkgRoot\bootstrap-vcpkg.bat"
if (!(Test-Path $Bootstrap)) { Fail "bootstrap-vcpkg.bat missing in C:\vcpkg" }

if (!(Test-Path "$VcpkgRoot\vcpkg.exe")) {
  Info "Bootstrapping vcpkg..."
  Push-Location $VcpkgRoot
  cmd /c "`"$Bootstrap`""
  Pop-Location
}

if (!(Test-Path "$VcpkgRoot\vcpkg.exe")) { Fail "vcpkg.exe not created." }

$env:VCPKG_ROOT = $VcpkgRoot
Info "VCPKG_ROOT = $env:VCPKG_ROOT"

$Toolchain = "$VcpkgRoot\scripts\buildsystems\vcpkg.cmake"
if (!(Test-Path $Toolchain)) { Fail "vcpkg toolchain not found: $Toolchain" }

# 4) Qt must be provided via QT_PREFIX_PATH
if ([string]::IsNullOrWhiteSpace($env:QT_PREFIX_PATH)) {
  Fail 'QT_PREFIX_PATH not set. Example: $env:QT_PREFIX_PATH="C:\Qt\6.6.1\msvc2022_64"'
}
$QtPrefix = $env:QT_PREFIX_PATH.Trim()
if (!(Test-Path $QtPrefix)) { Fail "QT_PREFIX_PATH points to missing folder: $QtPrefix" }

Info "Qt prefix = $QtPrefix"

# FIX: verify QtWebEngine exists (your current kit is missing it)
$web1 = Join-Path $QtPrefix "lib\Qt6WebEngineCore.lib"
$web2 = Join-Path $QtPrefix "bin\Qt6WebEngineCore.dll"
if (!(Test-Path $web1) -and !(Test-Path $web2)) {
  Fail "QtWebEngine NOT installed in this Qt kit. Run C:\Qt\MaintenanceTool.exe -> Add components -> Qt WebEngine for your Qt version + msvc2022_64."
}

# 5) Configure + Build
$BuildDir = Join-Path $ProjectRoot "build"
if (Test-Path $BuildDir) {
  Info "Removing old build directory"
  Remove-Item $BuildDir -Recurse -Force
}
New-Item -ItemType Directory -Path $BuildDir | Out-Null

Info "Configuring CMake..."
cmake -S $ProjectRoot -B $BuildDir `
  -G "Visual Studio 17 2022" -A x64 `
  -DCMAKE_TOOLCHAIN_FILE="$Toolchain" `
  -DVCPKG_FEATURE_FLAGS=manifests `
  -DCMAKE_PREFIX_PATH="$QtPrefix"

Info "Building Release..."
cmake --build $BuildDir --config Release

# 6) Run
$Exe = Join-Path $BuildDir "Release\NovaBrowse.exe"
if (!(Test-Path $Exe)) { Fail "NovaBrowse.exe not found after build: $Exe" }

Info "Starting NovaBrowse..."
& $Exe
