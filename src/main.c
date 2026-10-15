// St. Marrow's - Mode 1 (milestone 6a: audio)
//
// main.c runs the window and the game loop. The real work lives in the modules:
//   map.c      tile map, collision, line of sight     path.c     route finding
//   player.c   input, movement, hiding                stalker.c  the thing that hunts you
//   light.c    lights and shadows                     items.c    keys, notes, the exit
//   audio.c    every sound (made in code)             config.h   every tuning number

#include "raylib.h"
#include "raymath.h"
#include <math.h>
#include <stddef.h>
#include <string.h>
#include "config.h"
#include "map.h"
#include "player.h"
#include "stalker.h"
#include "light.h"
#include "items.h"
#include "audio.h"

#define MAP_FILE   "assets/maps/mode1_ch1.txt"
#define NOTES_FILE "assets/notes/mode1_ch1.txt"

static const Color AMBIENT = { 40, 40, 56, 255 };   // how dark "dark" is (0 = pitch black)

typedef enum { GS_PLAYING, GS_READING, GS_CAUGHT, GS_COMPLETE } GameState;

// Scale of the 640x360 picture inside the window (whole numbers when possible = sharp pixels).
static float GetViewScale(void)
{
    float s = fminf((float)GetScreenWidth() / VIRT_W, (float)GetScreenHeight() / VIRT_H);
    return (s >= 1.0f) ? floorf(s) : s;
}

// Appends src to dst without ever overflowing dst.
static void Cat(char *dst, size_t cap, const char *src)
{
    size_t n = strlen(dst);
    if (n + 1 >= cap) return;
    strncat(dst, src, cap - n - 1);
}

// Draws text wrapped to maxW pixels wide. Returns the y just below the last line.
static int DrawWrapped(const char *text, int x, int y, int maxW, int size, Color col)
{
    char line[400] = "";
    const char *p = text;
    while (*p) {
        if (*p == '\n') {                                   // paragraph break
            DrawText(line, x, y, size, col);
            y += size + 6;
            line[0] = '\0';
            p++;
            continue;
        }
        char word[64];
        int wl = 0;
        while (*p && *p != ' ' && *p != '\n' && wl < 63) word[wl++] = *p++;
        word[wl] = '\0';
        while (*p == ' ') p++;
        if (wl == 0) continue;

        char test[480] = "";
        Cat(test, sizeof test, line);
        if (line[0]) Cat(test, sizeof test, " ");
        Cat(test, sizeof test, word);
        if (line[0] && MeasureText(test, size) > maxW) {    // does not fit: start a new line
            DrawText(line, x, y, size, col);
            y += size + 6;
            line[0] = '\0';
            Cat(line, sizeof line, word);
        } else {
            line[0] = '\0';
            Cat(line, sizeof line, test);
        }
    }
    if (line[0]) { DrawText(line, x, y, size, col); y += size + 6; }
    return y;
}

int main(void)
{
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
    InitWindow(VIRT_W * 2, VIRT_H * 2, "St. Marrow's");
    SetWindowMinSize(VIRT_W, VIRT_H);
    SetTargetFPS(60);
    AudioInit();                                     // no sound device? the game just runs silently
    ChangeDirectory(GetApplicationDirectory());      // so "assets/..." is found next to the EXE

    Map map;
    if (!LoadMap(&map, MAP_FILE)) {
        while (!WindowShouldClose()) {
            BeginDrawing();
            ClearBackground(BLACK);
            DrawText("Could not load " MAP_FILE, 20, 20, 20, RED);
            DrawText("Run the game from its own folder (the one that contains 'assets').", 20, 50, 20, RAYWHITE);
            EndDrawing();
        }
        CloseWindow();
        return 1;
    }
    Note notes[MAX_NOTES];
    int noteCount = LoadNotes(NOTES_FILE, notes, MAX_NOTES);

    RenderTexture2D scene = LoadRenderTexture(VIRT_W, VIRT_H);   // the game picture
    RenderTexture2D mask  = LoadRenderTexture(VIRT_W, VIRT_H);   // the darkness, multiplied over the picture
    SetTextureFilter(scene.texture, TEXTURE_FILTER_POINT);
    LightKit lights;
    LightKitInit(&lights);

    Player player;
    Stalker stalker;
    Objective obj;
    PlayerInit(&player, map.playerStart);
    StalkerInit(&stalker, &map);
    ObjectiveInit(&obj, &map);
    Camera2D cam = { .offset = { VIRT_W / 2.0f, VIRT_H / 2.0f }, .target = player.pos, .rotation = 0.0f, .zoom = 1.0f };
    GameState state = GS_PLAYING;
    bool debug = false;
    int openNote = -1;                 // which note is being read
    float runTime = 0;                 // seconds spent in this run
    char toast[96] = "";               // short message at the bottom of the screen
    float toastTime = 0;

    while (!WindowShouldClose()) {
        float dt = fminf(GetFrameTime(), 0.05f);
        if (toastTime > 0) toastTime -= dt;

        // ---- Update ----
        if (state == GS_PLAYING || state == GS_READING) {
            runTime += dt;
            if (state == GS_PLAYING) {
                PlayerUpdate(&player, &map, dt);
                if (!player.hidden && PickUpKey(&obj, &map, player.pos)) {
                    TextCopy(toast, TextFormat("Key found  (%d/%d)", obj.keys, obj.keysTotal));
                    toastTime = 2.5f;
                    AudioChime();
                }
                if (IsKeyPressed(KEY_E)) {                   // E does the most useful thing nearby
                    int note;
                    if (player.hidden) {
                        PlayerLeaveLocker(&player);
                        AudioLocker();
                    } else if ((note = NoteNear(&map, player.pos)) >= 0) {
                        openNote = note;
                        state = GS_READING;
                        AudioPaper();
                    } else if (ExitNear(&map, player.pos)) {
                        if (obj.keys >= obj.keysTotal) {
                            state = GS_COMPLETE;
                            AudioComplete();
                        } else {
                            TextCopy(toast, TextFormat("The exit is locked.  Keys: %d/%d", obj.keys, obj.keysTotal));
                            toastTime = 2.5f;
                            AudioLockedDoor();
                        }
                    } else if (PlayerTryHide(&player, &map)) {
                        AudioLocker();
                    }
                }
            } else {                                         // reading: you stand still, the stalker does not
                player.moving = player.running = false;
                if (IsKeyPressed(KEY_E) || IsKeyPressed(KEY_ENTER)) { state = GS_PLAYING; AudioPaper(); }
            }
            if (state != GS_COMPLETE && StalkerUpdate(&stalker, &map, &player, dt)) { state = GS_CAUGHT; AudioSting(); }
        } else if (IsKeyPressed(KEY_R)) {                    // caught or finished: R starts over
            LoadMap(&map, MAP_FILE);                         // reload so the keys are back
            ObjectiveInit(&obj, &map);
            PlayerInit(&player, map.playerStart);
            StalkerInit(&stalker, &map);
            runTime = 0;
            toastTime = 0;
            openNote = -1;
            state = GS_PLAYING;
        }
        if (IsKeyPressed(KEY_F3)) debug = !debug;
        if (IsKeyPressed(KEY_M)) AudioToggleMute();
        AudioUpdate(&player, &stalker, state == GS_PLAYING || state == GS_READING, dt);

        cam.target = (Vector2){ roundf(player.pos.x), roundf(player.pos.y) };   // whole pixels = no shimmer

        // Mouse position: window pixels -> game pixels.
        float scale = GetViewScale();
        Vector2 origin = { (GetScreenWidth() - VIRT_W * scale) / 2, (GetScreenHeight() - VIRT_H * scale) / 2 };
        Vector2 mouse = { (GetMouseX() - origin.x) / scale, (GetMouseY() - origin.y) / scale };
        Vector2 me = GetWorldToScreen2D(player.pos, cam);          // player position on the 640x360 picture
        Vector2 aim = Vector2Subtract(mouse, me);
        float aimDeg = (aim.x == 0 && aim.y == 0) ? 0 : atan2f(aim.y, aim.x) * RAD2DEG;

        // ---- Draw 1: the darkness mask (dark everywhere, light added where the lights are) ----
        ClearMask(mask, AMBIENT);
        if (player.hidden) {
            AddLight(&lights, mask, &map, cam, lights.glow, player.pos, 20.0f, 0);       // only a faint glow inside the locker
        } else {
            AddLight(&lights, mask, &map, cam, lights.glow, player.pos, GLOW_RANGE, 0);
            if (player.flashlight)
                AddLight(&lights, mask, &map, cam, lights.beam, player.pos, BEAM_RANGE, aimDeg);
        }

        // ---- Draw 2: the world, then the mask multiplied on top ----
        BeginTextureMode(scene);
            ClearBackground(BLACK);
            BeginMode2D(cam);
                // You only see the stalker, keys and notes if there is a clear line to them (hiding: you peek out of the locker door).
                Vector2 eyes = player.hidden ? player.exitPos : player.pos;
                DrawMap(&map, cam, eyes);
                if (HasLineOfSight(&map, eyes, stalker.pos)) DrawStalker(&stalker);
                if (!player.hidden) {
                    DrawCircleV(player.pos, player.half, (Color){ 225, 205, 160, 255 });
                    DrawLineEx(player.pos, Vector2Add(player.pos, Vector2Scale(Vector2Normalize(aim), 7)),
                               2, (Color){ 255, 240, 200, 255 });
                }
            EndMode2D();
            BeginBlendMode(BLEND_MULTIPLIED);
                DrawTextureRec(mask.texture, (Rectangle){ 0, 0, VIRT_W, -VIRT_H }, (Vector2){ 0, 0 }, WHITE);
            EndBlendMode();
        EndTextureMode();

        // ---- Draw 3: scale the picture up to the window, then the text on top ----
        BeginDrawing();
            ClearBackground(BLACK);
            DrawTexturePro(scene.texture, (Rectangle){ 0, 0, VIRT_W, -VIRT_H },
                           (Rectangle){ origin.x, origin.y, VIRT_W * scale, VIRT_H * scale },
                           (Vector2){ 0, 0 }, 0, WHITE);
            int sw = GetScreenWidth(), sh = GetScreenHeight();

            DrawText("WASD move  |  Shift run (loud)  |  Mouse aim  |  F flashlight  |  E use / hide  |  M mute  |  R restart  |  F3 debug  |  Esc quit",
                     12, 10, 16, (Color){ 200, 200, 200, 170 });
            const char *goal = (obj.keys < obj.keysTotal)
                ? TextFormat("Find the keys: %d/%d", obj.keys, obj.keysTotal)
                : "All keys found. Reach the exit at the east end of the corridor.";
            DrawText(goal, 12, 34, 20, (Color){ 235, 205, 110, 235 });

            // What E would do right now.
            const char *prompt = NULL;
            if (state == GS_PLAYING) {
                Vector2 lockerAt;
                if (player.hidden)                              prompt = "Hiding...   press E to climb out";
                else if (NoteNear(&map, player.pos) >= 0)       prompt = "E   read note";
                else if (ExitNear(&map, player.pos))            prompt = (obj.keys >= obj.keysTotal) ? "E   unlock the exit" : "The exit is locked";
                else if (PlayerNearLocker(&player, &map, &lockerAt)) prompt = "E   hide in locker";
            }
            if (prompt)
                DrawText(prompt, (sw - MeasureText(prompt, 22)) / 2, sh - 56, 22, (Color){ 230, 230, 230, 220 });
            if (toastTime > 0)
                DrawText(toast, (sw - MeasureText(toast, 24)) / 2, sh - 100, 24, Fade(WHITE, fminf(1.0f, toastTime)));

            if (debug) {
                DrawFPS(12, 62);
                DrawText(TextFormat("tile %d, %d", (int)(player.pos.x / TILE), (int)(player.pos.y / TILE)), 12, 84, 16, GREEN);
                DrawText(TextFormat("stalker: %s   distance %d   %s", StalkerStateName(&stalker),
                                    (int)Vector2Distance(stalker.pos, player.pos),
                                    player.hidden ? (stalker.knowsHiding ? "HIDDEN (it saw you!)" : "HIDDEN (unseen)") : ""),
                         12, 106, 16, ORANGE);
            }

            if (state == GS_READING) {
                DrawRectangle(0, 0, sw, sh, (Color){ 0, 0, 0, 150 });
                int pw = sw - 120; if (pw > 760) pw = 760;
                int ph = sh * 6 / 10, px = (sw - pw) / 2, py = sh / 8;
                DrawRectangle(px, py, pw, ph, (Color){ 24, 22, 20, 238 });
                DrawRectangleLines(px, py, pw, ph, (Color){ 120, 105, 70, 255 });
                if (openNote >= 0 && openNote < noteCount) {
                    DrawText(notes[openNote].title, px + 24, py + 20, 30, (Color){ 235, 205, 110, 255 });
                    DrawWrapped(notes[openNote].body, px + 24, py + 70, pw - 48, 22, (Color){ 225, 220, 200, 255 });
                } else {
                    DrawText("(blank note)", px + 24, py + 20, 30, (Color){ 235, 205, 110, 255 });
                    DrawWrapped("Write this note in assets/notes/mode1_ch1.txt", px + 24, py + 70, pw - 48, 22, (Color){ 225, 220, 200, 255 });
                }
                DrawText("E  close          (the stalker is still out there)", px + 24, py + ph - 34, 18, (Color){ 160, 150, 130, 255 });
            }
            if (state == GS_CAUGHT) {
                DrawRectangle(0, 0, sw, sh, (Color){ 60, 0, 0, 170 });
                const char *title = "IT FOUND YOU";
                const char *hint = "Press R to try again";
                DrawText(title, (sw - MeasureText(title, 48)) / 2, sh / 2 - 40, 48, (Color){ 225, 50, 50, 255 });
                DrawText(hint, (sw - MeasureText(hint, 20)) / 2, sh / 2 + 20, 20, RAYWHITE);
            }
            if (state == GS_COMPLETE) {
                DrawRectangle(0, 0, sw, sh, (Color){ 0, 0, 0, 170 });
                const char *title = "CHAPTER 1 COMPLETE";
                const char *time = TextFormat("Time  %d:%02d", (int)runTime / 60, (int)runTime % 60);
                const char *hint = "Press R to play again";
                DrawText(title, (sw - MeasureText(title, 48)) / 2, sh / 2 - 60, 48, (Color){ 235, 205, 110, 255 });
                DrawText(time, (sw - MeasureText(time, 26)) / 2, sh / 2 + 6, 26, RAYWHITE);
                DrawText(hint, (sw - MeasureText(hint, 20)) / 2, sh / 2 + 48, 20, (Color){ 190, 190, 190, 255 });
            }
        EndDrawing();
    }

    LightKitUnload(&lights);
    UnloadRenderTexture(scene);
    UnloadRenderTexture(mask);
    AudioShutdown();
    CloseWindow();
    return 0;
}