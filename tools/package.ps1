# Builds dist\StMarrows-v<version>-win64.zip (run via "make dist" after a release build).
$ErrorActionPreference = "Stop"

$root    = Split-Path -Parent $PSScriptRoot
$version = (Get-Content (Join-Path $root "VERSION") -Raw).Trim()
$name    = "StMarrows-v$version-win64"
$dist    = Join-Path $root "dist"
$stage   = Join-Path $dist $name

if (Test-Path $dist) { Remove-Item $dist -Recurse -Force }
New-Item -ItemType Directory -Path $stage | Out-Null

Copy-Item (Join-Path $root "StMarrows.exe") $stage
Copy-Item (Join-Path $root "assets") $stage -Recurse
Copy-Item (Join-Path $root "LICENSE") $stage

@"
St. Marrow's v$version

1. Extract the whole zip first (do not run the EXE from inside the zip).
2. Double-click StMarrows.exe.

If Windows says "Windows protected your PC": click More info, then Run anyway.

Goal: find the 3 keys, then unlock the exit at the east end of the corridor.
Controls: WASD move | Shift run (loud) | Mouse aim flashlight | F flashlight | E read / hide / unlock | M mute | R restart | Esc pause menu
"@ | Set-Content (Join-Path $stage "HOW_TO_PLAY.txt")

Compress-Archive -Path $stage -DestinationPath (Join-Path $dist "$name.zip")
Write-Host "Created dist\$name.zip"
