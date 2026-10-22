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

Goal: get out of each wing alive.
  Chapter 1 (Ward A): find the 3 keys, then unlock the exit.
  Chapter 2 (Surgery Wing): find the 3 fuses, put them in the fuse box to restore the power, then take the elevator. Your flashlight battery runs down: find spares.
  Chapter 3 (Records Basement): find the 3 patient files, then open the archive vault. Pick up bottles and throw them to pull the stalker away from you.
  Chapter 4 (Security Wing): find the 3 access cards, then open the control room door. Ceiling cameras see in the dark and ring an alarm if they watch you for too long: cross while they swing away, hide in a locker, or smash one with a thrown bottle.
  Chapter 5 (The Roof): take Mr. A's ledger from his office, then run to the roof door before the 90 second countdown ends. The stalker is told where you are every few seconds.
Inventory (Chapters 2-3): walk over items to pick them up. 1-6 or the mouse wheel select a slot, click uses the selected item (load a spare battery, throw a bottle).
Controls: WASD move | Shift run (loud) | Mouse aim flashlight | F flashlight (or load a spare battery when it is dead) | E read / hide / use | 1-6 / wheel select item | Click use item | M mute | F11 fullscreen | R restart | Esc pause menu
"@ | Set-Content (Join-Path $stage "HOW_TO_PLAY.txt")

Compress-Archive -Path $stage -DestinationPath (Join-Path $dist "$name.zip")
Write-Host "Created dist\$name.zip"
