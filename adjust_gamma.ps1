$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Drawing

function Adjust-Gamma($path, $gamma) {
    Write-Host "Adjusting $path with gamma $gamma"
    $bmp = New-Object System.Drawing.Bitmap $path
    for ($y=0; $y -lt $bmp.Height; $y++) {
        for ($x=0; $x -lt $bmp.Width; $x++) {
            $c = $bmp.GetPixel($x, $y)
            $r = [int]([Math]::Pow($c.R / 255.0, $gamma) * 255)
            $g = [int]([Math]::Pow($c.G / 255.0, $gamma) * 255)
            $b = [int]([Math]::Pow($c.B / 255.0, $gamma) * 255)
            if ($r -gt 255) { $r = 255 }
            if ($g -gt 255) { $g = 255 }
            if ($b -gt 255) { $b = 255 }
            $bmp.SetPixel($x, $y, [System.Drawing.Color]::FromArgb($c.A, $r, $g, $b))
        }
    }
    $newPath = $path.Replace('_original.png', '.png')
    $bmp.Save($newPath, [System.Drawing.Imaging.ImageFormat]::Png)
    $bmp.Dispose()
    Write-Host "Saved to $newPath"
}

Adjust-Gamma 'c:\Users\K024G\source\repos\TD3\GAME\resources\TitleScene\tutorial1_original.png' 1.1
Adjust-Gamma 'c:\Users\K024G\source\repos\TD3\GAME\resources\TitleScene\tutorial2_original.png' 1.1
