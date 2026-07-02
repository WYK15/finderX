param(
    [Parameter(Mandatory = $true)]
    [string]$IconPngPath
)

Add-Type -AssemblyName System.Drawing

$resolved = Resolve-Path -LiteralPath $IconPngPath
$image = [System.Drawing.Image]::FromFile($resolved)
$bitmap = [System.Drawing.Bitmap]::new($image)

try {
    $points = @(
        @{ Name = "top-left"; X = 0; Y = 0 },
        @{ Name = "top-right"; X = $bitmap.Width - 1; Y = 0 },
        @{ Name = "bottom-left"; X = 0; Y = $bitmap.Height - 1 },
        @{ Name = "bottom-right"; X = $bitmap.Width - 1; Y = $bitmap.Height - 1 }
    )

    foreach ($point in $points) {
        $color = $bitmap.GetPixel($point.X, $point.Y)
        if ($color.A -ne 0) {
            throw "FinderX.png has an opaque $($point.Name) corner: A=$($color.A), R=$($color.R), G=$($color.G), B=$($color.B)"
        }
    }

    $minX = $bitmap.Width
    $minY = $bitmap.Height
    $maxX = -1
    $maxY = -1

    for ($y = 0; $y -lt $bitmap.Height; $y++) {
        for ($x = 0; $x -lt $bitmap.Width; $x++) {
            if ($bitmap.GetPixel($x, $y).A -gt 8) {
                $minX = [Math]::Min($minX, $x)
                $minY = [Math]::Min($minY, $y)
                $maxX = [Math]::Max($maxX, $x)
                $maxY = [Math]::Max($maxY, $y)
            }
        }
    }

    $contentWidth = $maxX - $minX + 1
    $contentHeight = $maxY - $minY + 1

    if ($contentWidth -lt 880 -or $contentHeight -lt 840) {
        throw "FinderX.png icon subject is too small: ${contentWidth}x${contentHeight}"
    }
} finally {
    $bitmap.Dispose()
    $image.Dispose()
}
