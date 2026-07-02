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
} finally {
    $bitmap.Dispose()
    $image.Dispose()
}
