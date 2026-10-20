# St. Marrow's

2D top-down stealth horror in C ([raylib](https://www.raylib.com/)). You are an investigator searching an abandoned hospital while one stalker hunts you. Hide, listen, survive.

![Preview](docs/preview.png)

- **Mode 1** (in progress): 5 chapters, one hospital wing each (Chapters 1-4 playable). More modes later.
- **Lore:** the hospital was made by Mr. A.
- **Platform:** Windows 64-bit.

**Controls:** WASD / arrows move · Shift run (loud) · Mouse aims the flashlight · F flashlight on/off · E use (read a note, unlock the exit, hide in a locker, climb out) · 1-6 or mouse wheel pick an item · Click use the selected item · R restart after being caught · F3 debug (shows the stalker's state) · M mute · Esc pause menu

## Chapter 1 goal

Find the 3 keys scattered through the wards, then unlock the exit at the east end of the main corridor (press E at the door). Notes (`n` tiles) can be read with E; the stalker keeps hunting while you read. The story text is placeholder: rewrite it in `assets/notes/mode1_ch1.txt`.

## Chapter 2 goal (Surgery Wing)

Find the 3 fuses (sterilising room, pharmacy, Theatre 1), put them in the fuse box in the boiler room on the east side (press E), then run to the elevator in the north-west corner and press E. New in this chapter:

- **Flashlight battery.** It drains while the light is on (bar at the bottom left). Under 25% the beam gets shorter, under 12% it flickers, at 0 it dies. Walk over a battery (`B`) to put it in your inventory, then select it and click to load it. When the flashlight is dead, **F** loads a spare for you. Tune it in `src/config.h` (`BATTERY_*`).
- **The fuse box is loud.** When you put the fuses in, the stalker hears it and goes to look at the spot where you stood. The wing switches to red emergency light and the elevator glows.
- **A faster stalker** (12% faster; `stalkerSpeed` in `src/chapters.h`).

**Chapters and progress:** finishing a chapter unlocks the next one (saved on line 2 of `settings.txt`). On the title screen use W/S to pick the *Chapter* row and A/D to change it. Press **F3** (debug) on the title screen to open every chapter for testing.

## Chapter 3 goal (Records Basement)

Find the 3 patient files (the clerks' office, the stacks, the incinerator), then open the archive vault on the east side (press E at the door). New in this chapter:

- **Throwing bottles.** Walk over a bottle (`o`) to put it in your inventory (5 fit in a slot). Select it and **click** to throw it toward the cursor: it flies up to 13 tiles, stops before walls, and shatters. If the stalker is within about 26 tiles it hears that and goes to look. Tune it in `src/config.h` (`BOTTLE_STACK`, `THROW_*`).
- **The archive stacks:** a big hall of shelves with 2-wide aisles and loops, good for pulling the stalker away with a bottle and slipping past.
- **A faster stalker** (20% faster than Chapter 1), and the flashlight battery carries over from Chapter 2.

## Chapter 4 goal (Security Wing)

Find the 3 access cards (the west cells, the server room, the break room), then open the control room door at the top (press E at it). New in this chapter:

- **Security cameras** (`>` `v` `<` `^` in the map file). They hang from the ceiling, swing left and right, and **see in the dark**. The cone of light shows where each one is looking. If a camera keeps you in view for about 0.8 s the alarm rings and the stalker goes to where you were (a red edge closes in on the screen as the camera fills up). After an alarm no camera can ring again for 8 s.
- **Ways past them:** wait for the beam to swing away and cross in the gap, hide in a locker (they cannot see in), or **break a camera**: select a bottle and throw it so it shatters right next to the camera.
- **A faster stalker** (22% faster than Chapter 1). Battery and inventory carry over.
- Tune the cameras in `src/config.h` (`CAM_*`) and the stalker's speed in `src/chapters.h`.

## Inventory

From Chapter 2 on, a row of slots (the hotbar) sits at the bottom of the screen. Walk over an item to pick it up; items of one kind stack in a slot. **1-6** or the **mouse wheel** select a slot, **click** uses the selected item (the line above the hotbar says what a click does). If the inventory is full, the item stays on the floor until you free a slot. It starts empty in every chapter.

| Item    | Map char | Stack | Click does                          |
| ------- | -------- | ----- | ----------------------------------- |
| Battery | `B`      | 3     | loads it into the flashlight (+40%) |
| Bottle  | `o`      | 5     | throws it where you aim             |

**Things you must collect** to open the exit (keys, fuses, patient files, access cards) are a separate table, `COLLECTIBLES` in `src/items.c`: add a name to `Collect` in `src/items.h`, a row to the table, and put its map character on a map.

**Adding a new item** takes three steps (the table in `src/inventory.c` does the rest: picking up, stacking, the hotbar, drawing it on the map):

1. Add a name to `ItemId` in `src/inventory.h` and a row to the `ITEMS` table in `src/inventory.c` (name, hint, map character, stack size, picture, plain-shape colour). A new picture is one more `TileId` in `src/tiles.h` and `src/tiles.c`, plus a number in `assets/tiles/tiles.txt`.
2. Put its map character on a map.
3. Add a `case` for it in `UseSelected()` in `src/main.c`: what happens when you click.

Slots, stack sizes: `INV_SLOTS`, `BATTERY_STACK`, `BOTTLE_STACK` in `src/config.h`.

## How the stalker works

- It patrols the points `1`, `2`, `3`... in the map file, then loops.
- **Sees you:** clear line of sight, inside its field of view, within ~190 px if your flashlight is on, ~55 px if it is off. Once it chases, it watches every direction.
- **Hears you:** walking within ~40 px, running within ~120 px, even through walls. Standing still is silent.
- Hears you: it investigates the noise. Sees you: it chases (faster than you walk, slower than you run). Loses you: it searches the last place it saw you, then goes back to patrolling.
- **Hiding:** press E next to a locker to climb in. Inside, you are invisible and silent. If the stalker watched you climb in, it walks over and opens the locker (game over). If it did not see you, it never checks lockers.
- Every number is in `src/config.h`.

## Set up (Windows, one time)

**Option A: MSYS2** (tested from the MSYS2 terminal)

1. Install [MSYS2](https://www.msys2.org/). Open **MSYS2 UCRT64** (not MINGW64 or MSYS) and run:
   `pacman -S --needed mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-gdb make`
2. In that terminal go to this folder (drive D: is `/d/...`), then run `make` and `./StMarrows.exe`.
3. For VS Code: in `.vscode/tasks.json` set the `PATH` line to `C:\\msys64\\ucrt64\\bin;C:\\msys64\\usr\\bin;${env:PATH}` (use your MSYS2 folder), and fix the gcc and gdb paths in `c_cpp_properties.json` and `launch.json`. Then **Ctrl+Shift+B** builds.

**Option B: w64devkit**

1. Install [VS Code](https://code.visualstudio.com/) and accept the **C/C++** extension it recommends.
2. Download the latest **w64devkit** (`w64devkit-x64-...exe`) from <https://github.com/skeeto/w64devkit/releases> and extract it so `C:\w64devkit\bin\gcc.exe` exists. Use the latest release: the bundled raylib 6.0 needs a recent MinGW (an old one fails with `undefined reference to 'stat64i32'`).
3. Open this folder in VS Code. **Ctrl+Shift+B** builds, **Terminal > Run Task > Run** starts the game, **F5** debugs.

| Command        | Result                                                |
| -------------- | ----------------------------------------------------- |
| `make`         | debug build, `StMarrows.exe`                          |
| `make release` | optimized, static, no console window, no DLLs needed  |
| `make dist`    | release build + `dist/StMarrows-v<version>-win64.zip` |

## Layout

```
src/main.c        window + game loop
src/config.h      every tuning number (speeds, ranges, sizes)
src/map.c/.h      tile map: loading, collision, line of sight, drawing
src/path.c/.h     route finding on the tile grid (breadth-first search)
src/player.c/.h   input, movement, noise, hiding
src/stalker.c/.h  the stalker's state machine
src/items.c/.h    keys, fuses, files, picking up items, notes, the fuse box, the exit door
src/inventory.c/.h  the inventory (hotbar) and the item table
src/cameras.c/.h    security cameras: sweeping, seeing you, the alarm, breaking
src/throw.c/.h    throwing bottles: flight, where it lands, the shatter effect
src/chapters.h    the list of chapters: map, notes, stalker speed, battery on/off
src/light.c/.h    lights and shadows (walls block light)
assets/maps/      levels as plain text: edit them in VS Code
assets/notes/     the text of the notes you find (plain text)
assets/CREDITS.md every third-party asset: author, license, link
libs/raylib/      raylib 6.0 headers + static lib for MinGW (zlib license)
tools/package.ps1 builds the release zip
VERSION           version used in the zip name
```

Map legend: `#` wall · `.` floor · `L` locker · `P` player start · `S` stalker start · `1`-`9` stalker patrol points (visited in order) · `K` key · `A` access card · `>` `v` `<` `^` ceiling camera (the way it looks) · `R` patient file · `o` bottle · `H` archive shelf · `F` fuse · `G` fuse box · `T` table, bed or shelf (blocks movement and light) · `n` note (numbered top to bottom, left to right, matching the notes file) · `X` locked exit door. Lines starting with `;` are comments.

## Architecture

- No engine, no global soup: plain `struct`s (`Map`, `Player`, `Stalker`) passed by pointer. Each module has a `.h` (what it offers) and a `.c` (how).
- Each frame: **update** (input, movement, AI, all scaled by `dt`) then **draw** (world, light mask, scale to window). Keep them separate.
- The game draws at 640x360 and scales up, so pixel art stays crisp at any window size.
- Lighting: each light draws a soft sprite, then black shadow shapes behind every wall edge that faces away from it. The result is added to the darkness mask, which is multiplied over the world.
- Levels are text files. One wing = one chapter map.
- Assets: CC0 only, logged in `assets/CREDITS.md`.

## Milestones

- [x] **M0** Toolchain set up, `StMarrows.exe` opens
- [x] **M1** Map, player, collision, camera
- [x] **M2** Darkness and flashlight
- [x] **M3** Stalker: patrol, sight, hearing, chase, search, game over
- [x] **M3b** Walls and lockers block the flashlight (shadows)
- [x] **M4** Hide in lockers (safe unless the stalker saw you climb in)
- [x] **M5** Keys, locked exit, notes, objective (locked room doors can come later)
- [x] **M6a** Audio: drone, footsteps, heartbeat, pickups (made in code, placeholders)
- [x] **M6b** Title menu, pause, volume (saved in `settings.txt`)
- [x] **M6c** Art pass: Kenney's Tiny Dungeon (CC0), set up in assets/tiles/tiles.txt
- [x] **M7** Chapter 2, Surgery Wing: flashlight battery, fuses and a fuse box, a faster stalker; chapter select and progress
- [x] **M8** Chapter 3, Records Basement: throwable bottles that draw the stalker, the archive stacks
- [x] **M9** Chapter 4, Security Wing: ceiling cameras that see in the dark and ring an alarm
- [ ] **M10** Chapter 5, the finale
- [ ] **M11** Polish: settings, controller, performance. **Release v1.0 (Mode 1 complete)**

## Publish on GitHub

1. On github.com create an empty repo (no README or license; this folder has them).
2. In this folder:
   ```
   git init
   git add .
   git commit -m "St. Marrow's"
   git branch -M main
   git remote add origin https://github.com/<you>/<repo>.git
   git push -u origin main
   ```
3. Release: edit `VERSION`, run `make dist`, then on GitHub go to **Releases > Draft a new release**, create tag `v0.1.0`, attach `dist/StMarrows-v0.1.0-win64.zip`, publish.

Players download that zip from the **Releases** page, extract it, and double-click `StMarrows.exe`. The green **Code > Download ZIP** button only gives source code, not the game. Windows may warn about an unsigned EXE; `HOW_TO_PLAY.txt` in the zip explains the click-through.

## Licenses

Code: MIT (put your name in `LICENSE`). raylib: zlib (`libs/raylib/LICENSE`). Art and audio keep their own licenses (`assets/CREDITS.md`).
