param(
    [string]$AssetDirectory = (Join-Path $PSScriptRoot '..\assets\fonts'),
    [string]$Output = (Join-Path $PSScriptRoot '..\src\embedded_fonts.cpp')
)

$ErrorActionPreference = 'Stop'

$fonts = @(
    @{ Symbol = 'RobotoRegular'; File = 'Roboto-Regular.ttf' },
    @{ Symbol = 'MaterialIcons'; File = 'MaterialIcons-Regular.ttf' },
    @{ Symbol = 'RobotoLight'; File = 'Roboto-Light.ttf' },
    @{ Symbol = 'RobotoMedium'; File = 'Roboto-Medium.ttf' },
    @{ Symbol = 'RobotoBold'; File = 'Roboto-Bold.ttf' }
)

function Write-ByteArray([System.IO.StreamWriter]$Writer, [string]$Symbol,
                         [byte[]]$Bytes) {
    $Writer.WriteLine("alignas(16) const unsigned char ${Symbol}Data[] = {")
    for ($index = 0; $index -lt $Bytes.Length; $index += 16) {
        $last = [Math]::Min($index + 16, $Bytes.Length) - 1
        $values = for ($cursor = $index; $cursor -le $last; ++$cursor) {
            '0x{0:X2}' -f $Bytes[$cursor]
        }
        $Writer.WriteLine('    ' + ($values -join ', ') + ',')
    }
    $Writer.WriteLine('};')
    $Writer.WriteLine("const std::size_t ${Symbol}Size = sizeof(${Symbol}Data);")
}

$parent = Split-Path -Parent $Output
New-Item -ItemType Directory -Force -Path $parent | Out-Null
$writer = [System.IO.StreamWriter]::new($Output, $false, [System.Text.UTF8Encoding]::new($false))
try {
    $writer.WriteLine('#include "embedded_fonts.h"')
    $writer.WriteLine('#include <cstddef>')
    $writer.WriteLine('')
    $writer.WriteLine('namespace ImGuiMD2::EmbeddedFonts {')
    $writer.WriteLine('namespace {')

    foreach ($font in $fonts[0..1]) {
        $path = Join-Path $AssetDirectory $font.File
        if (!(Test-Path -LiteralPath $path -PathType Leaf)) {
            throw "Font asset not found: $path"
        }
        Write-ByteArray $writer $font.Symbol ([System.IO.File]::ReadAllBytes($path))
    }

    $writer.WriteLine('#if defined(IMGUI_MD2_EMBED_LIGHT) || defined(IMGUI_MD2_EMBED_FULL_FONTS)')
    foreach ($font in $fonts[2..2]) {
        $path = Join-Path $AssetDirectory $font.File
        if (!(Test-Path -LiteralPath $path -PathType Leaf)) {
            throw "Font asset not found: $path"
        }
        Write-ByteArray $writer $font.Symbol ([System.IO.File]::ReadAllBytes($path))
    }
    $writer.WriteLine('#endif')
    $writer.WriteLine('#if defined(IMGUI_MD2_EMBED_MEDIUM) || defined(IMGUI_MD2_EMBED_FULL_FONTS)')
    foreach ($font in $fonts[3..3]) {
        $path = Join-Path $AssetDirectory $font.File
        if (!(Test-Path -LiteralPath $path -PathType Leaf)) {
            throw "Font asset not found: $path"
        }
        Write-ByteArray $writer $font.Symbol ([System.IO.File]::ReadAllBytes($path))
    }
    $writer.WriteLine('#endif')
    $writer.WriteLine('#if defined(IMGUI_MD2_EMBED_BOLD) || defined(IMGUI_MD2_EMBED_FULL_FONTS)')
    foreach ($font in $fonts[4..4]) {
        $path = Join-Path $AssetDirectory $font.File
        if (!(Test-Path -LiteralPath $path -PathType Leaf)) {
            throw "Font asset not found: $path"
        }
        Write-ByteArray $writer $font.Symbol ([System.IO.File]::ReadAllBytes($path))
    }
    $writer.WriteLine('#endif')
    $writer.WriteLine('} // namespace')
    $writer.WriteLine('')
    $writer.WriteLine('const unsigned char* RegularData() { return RobotoRegularData; }')
    $writer.WriteLine('std::size_t RegularSize() { return RobotoRegularSize; }')
    $writer.WriteLine('const unsigned char* IconsData() { return MaterialIconsData; }')
    $writer.WriteLine('std::size_t IconsSize() { return MaterialIconsSize; }')
    $writer.WriteLine('#if defined(IMGUI_MD2_EMBED_LIGHT) || defined(IMGUI_MD2_EMBED_FULL_FONTS)')
    $writer.WriteLine('const unsigned char* LightData() { return RobotoLightData; }')
    $writer.WriteLine('std::size_t LightSize() { return RobotoLightSize; }')
    $writer.WriteLine('#else')
    $writer.WriteLine('const unsigned char* LightData() { return nullptr; }')
    $writer.WriteLine('std::size_t LightSize() { return 0; }')
    $writer.WriteLine('#endif')
    $writer.WriteLine('#if defined(IMGUI_MD2_EMBED_MEDIUM) || defined(IMGUI_MD2_EMBED_FULL_FONTS)')
    $writer.WriteLine('const unsigned char* MediumData() { return RobotoMediumData; }')
    $writer.WriteLine('std::size_t MediumSize() { return RobotoMediumSize; }')
    $writer.WriteLine('#else')
    $writer.WriteLine('const unsigned char* MediumData() { return nullptr; }')
    $writer.WriteLine('std::size_t MediumSize() { return 0; }')
    $writer.WriteLine('#endif')
    $writer.WriteLine('#if defined(IMGUI_MD2_EMBED_BOLD) || defined(IMGUI_MD2_EMBED_FULL_FONTS)')
    $writer.WriteLine('const unsigned char* BoldData() { return RobotoBoldData; }')
    $writer.WriteLine('std::size_t BoldSize() { return RobotoBoldSize; }')
    $writer.WriteLine('#else')
    $writer.WriteLine('const unsigned char* BoldData() { return nullptr; }')
    $writer.WriteLine('std::size_t BoldSize() { return 0; }')
    $writer.WriteLine('#endif')
    $writer.WriteLine('#if (defined(IMGUI_MD2_EMBED_LIGHT) || defined(IMGUI_MD2_EMBED_FULL_FONTS)) && (defined(IMGUI_MD2_EMBED_MEDIUM) || defined(IMGUI_MD2_EMBED_FULL_FONTS))')
    $writer.WriteLine('bool HasFullTypeScale() { return true; }')
    $writer.WriteLine('#else')
    $writer.WriteLine('bool HasFullTypeScale() { return false; }')
    $writer.WriteLine('#endif')
    $writer.WriteLine('} // namespace ImGuiMD2::EmbeddedFonts')
} finally {
    $writer.Dispose()
}
