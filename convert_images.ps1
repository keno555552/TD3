$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Drawing

function Convert-ToLinear($path) {
    Write-Host "Converting $path"
    $bmp = New-Object System.Drawing.Bitmap $path
    for ($y=0; $y -lt $bmp.Height; $y++) {
        for ($x=0; $x -lt $bmp.Width; $x++) {
            $c = $bmp.GetPixel($x, $y)
            $r = [Math]::Pow($c.R / 255.0, 2.2) * 255
            $g = [Math]::Pow($c.G / 255.0, 2.2) * 255
            $b = [Math]::Pow($c.B / 255.0, 2.2) * 255
            $bmp.SetPixel($x, $y, [System.Drawing.Color]::FromArgb($c.A, $r, $g, $b))
        }
    }
    $newPath = $path.Replace('.png', '_linear.png')
    $bmp.Save($newPath, [System.Drawing.Imaging.ImageFormat]::Png)
    $bmp.Dispose()
    Write-Host "Saved to $newPath"
}

Convert-ToLinear 'c:\Users\K024G\source\repos\TD3\GAME\resources\TitleScene\tutorial1.png'
Convert-ToLinear 'c:\Users\K024G\source\repos\TD3\GAME\resources\TitleScene\tutorial2.png'
