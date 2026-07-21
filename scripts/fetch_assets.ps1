param(
    [switch]$Force
)

$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
$fontDirectory = Join-Path $projectRoot 'assets/fonts'
New-Item -ItemType Directory -Force -Path $fontDirectory | Out-Null

$assets = @(
    @{
        Name = 'Roboto-Regular.ttf'
        Url = 'https://raw.githubusercontent.com/laserpants/qt-material-widgets/master/fonts/Roboto/Roboto-Regular.ttf'
    },
    @{
        Name = 'Roboto-Medium.ttf'
        Url = 'https://raw.githubusercontent.com/laserpants/qt-material-widgets/master/fonts/Roboto/Roboto-Medium.ttf'
    },
    @{
        Name = 'Roboto-Light.ttf'
        Url = 'https://raw.githubusercontent.com/laserpants/qt-material-widgets/master/fonts/Roboto/Roboto-Light.ttf'
    },
    @{
        Name = 'MaterialIcons-Regular.ttf'
        Url = 'https://raw.githubusercontent.com/google/material-design-icons/master/font/MaterialIcons-Regular.ttf'
    },
    @{
        Name = 'LICENSE-Roboto.txt'
        Url = 'https://raw.githubusercontent.com/laserpants/qt-material-widgets/master/fonts/Roboto/LICENSE.txt'
    },
    @{
        Name = 'LICENSE-MaterialIcons.txt'
        Url = 'https://raw.githubusercontent.com/google/material-design-icons/master/LICENSE'
    }
)

foreach ($asset in $assets) {
    $target = Join-Path $fontDirectory $asset.Name
    if ($Force -or -not (Test-Path -LiteralPath $target)) {
        Write-Host "Fetching $($asset.Name)"
        Invoke-WebRequest -Uri $asset.Url -OutFile $target
    }
    if ((Get-Item -LiteralPath $target).Length -eq 0) {
        throw "Downloaded an empty asset: $target"
    }
}

Get-ChildItem -LiteralPath $fontDirectory -File |
    Sort-Object Name |
    Get-FileHash -Algorithm SHA256 |
    Select-Object @{Name='Name'; Expression={ Split-Path -Leaf $_.Path }}, Hash
