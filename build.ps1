param(
    [string]$VSPath,
    [string]$Config = "Release",
    [string]$Platform = "x64"
)

# VS のインストール先に VsDevCmd.bat がない場合、MSBuild を直接探す
if (-not $VSPath) {
    $VSPath = & "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe" -latest -products * -property installationPath
    Write-Host "Found VS at: $VSPath"
}

# MSBuild のパスを構築
$MSBuildPath = Join-Path $VSPath "MSBuild\Current\Bin\MSBuild.exe"

if (-not (Test-Path $MSBuildPath)) {
    Write-Error "MSBuild not found at $MSBuildPath"
    exit 1
}

Write-Host "MSBuild found at: $MSBuildPath"

# Step 1: ビルド
Write-Host "`n=== Step 1: Building foo_gdrive_stream.dll ===" -ForegroundColor Green
& $MSBuildPath "foo_gdrive_stream\foo_gdrive_stream.vcxproj" "/p:Configuration=$Config" "/p:Platform=$Platform" "/nologo"

if ($LASTEXITCODE -ne 0) {
    Write-Error "Build failed"
    exit 1
}

# Step 2: テストのビルドと実行
Write-Host "`n=== Step 2: Building and running tests ===" -ForegroundColor Green
& $MSBuildPath "foo_gdrive_stream\tests\tests.vcxproj" "/p:Configuration=$Config" "/p:Platform=$Platform" "/nologo"

if ($LASTEXITCODE -ne 0) {
    Write-Error "Test build failed"
    exit 1
}

$TestExe = "foo_gdrive_stream\tests\_result\${Platform}_${Config}\tests\foo_gdrive_stream_tests.exe"
if (Test-Path $TestExe) {
    & $TestExe
    if ($LASTEXITCODE -ne 0) {
        Write-Error "Tests failed"
        exit 1
    }
} else {
    Write-Error "Test executable not found at $TestExe"
    exit 1
}

# Step 3: コンポーネント パッケージング
Write-Host "`n=== Step 3: Packaging component ===" -ForegroundColor Green
if (Test-Path "foo_gdrive_stream\scripts\pack_component.py") {
    python "foo_gdrive_stream\scripts\pack_component.py" "--platform" $Platform "--configuration" $Config
    if ($LASTEXITCODE -ne 0) {
        Write-Error "Packaging failed"
        exit 1
    }
} else {
    Write-Warning "pack_component.py not found"
}

Write-Host "`n=== Build Complete ===" -ForegroundColor Green
Write-Host "DLL: foo_gdrive_stream\_result\${Platform}_${Config}\bin\foo_gdrive_stream.dll"
Write-Host "Component: foo_gdrive_stream\_result\foo_gdrive_stream.fb2k-component"
