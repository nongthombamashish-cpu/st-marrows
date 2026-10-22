// St. Marrow's - Mode 1 (milestone 7: chapters, and Chapter 2 - the Surgery Wing)
//
// main.c runs the window and the game loop. The real work lives in the modules:
//   map.c      tile map, collision, line of sight     path.c     route finding
//   player.c   input, movement, hiding                stalker.c  the thing that hunts you
//   light.c    lights and shadows                     items.c    keys, fuses, batteries, notes, the exit
//   audio.c    every sound (made in code)             config.h   every tuning number
//   chapters.h the list of chapters (map, notes, stalker speed, battery)

#include "raylib.h"
#include "raymath.h"
#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "config.h"
#include "map.h"
#include "player.h"
#include "stalker.h"
#include "light.h"
#include "items.h"
#include "audio.h"
#include "tiles.h"
#include "chapters.h"
#include "throw.h"
#include "inventory.h"
#include "cameras.h"
#include "hunt.h"

#define SETTINGS_FILE "settings.txt"                // saved next to the game: the volume (0-10), unlocked chapters, fullscreen
#define TITLE_CREDIT  "Made by Mr. A   |   Art: Kenney (CC0)"

// The credits after THE END. A line that starts with # is a heading; "" is a gap. Edit them here.
static const char *CREDITS[] = {
    "#ST. MARROW'S", "",
    "A game by Mr. A", "",
    "#Made with", "C and raylib 6.0",
    "raylib by Ramon Santamaria and contributors (zlib license)", "",
    "#Art", "Tiny Dungeon by Kenney (CC0)", "",
    "#Sound", "Every sound is made in code", "",
    "#Thank you for playing",
};
#define CREDITS_LINES ((int)(sizeof CREDITS / sizeof CREDITS[0]))
#define CREDITS_LINE_H 34.0f
static float CreditsEndY(int screenH) { return screenH * 0.42f - (CREDITS_LINES - 1) * CREDITS_LINE_H; }   // where the scroll stops

static const Color AMBIENT = { 40, 40, 56, 255 };   // how dark "dark" is (0 = pitch black)

typedef enum { GS_MENU, GS_PLAYING, GS_READING, GS_PAUSED, GS_CAUGHT, GS_COMPLETE, GS_CREDITS } GameState;

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

// settings.txt (next to the game): line 1 = volume 0-10, line 2 = how many chapters are unlocked, line 3 = fullscreen (0 or 1).
static void LoadSettings(int *volume, int *unlocked, bool *fullscreen)
{
    int v = 8, u = 1, f = 0;                                // defaults
    char *text = LoadFileText(SETTINGS_FILE);
    if (text) {
        sscanf(text, "%d %d %d", &v, &u, &f);               // an older file with fewer lines keeps the defaults
        UnloadFileText(text);
    }
    if (v < 0) v = 0;
    if (v > 10) v = 10;
    if (u < 1) u = 1;
    if (u > CHAPTER_COUNT) u = CHAPTER_COUNT;
    *volume = v;
    *unlocked = u;
    *fullscreen = (f != 0);
}

static void SaveSettings(int volume, int unlocked, bool fullscreen)
{
    SaveFileText(SETTINGS_FILE, (char *)TextFormat("%d\n%d\n%d", volume, unlocked, fullscreen ? 1 : 0));
}

// The yellow line at the top: what to do next.
static const char *GoalText(const Objective *o, const Map *m, const Chapter *ch)
{
    for (int i = 0; i < COLLECT_COUNT; i++)                            // keys, fuses, files, cards: whatever is still missing
        if (o->got[i] < o->total[i]) return TextFormat("Find the %s: %d/%d", CollectGet((Collect)i)->plural, o->got[i], o->total[i]);
    if (m->hasFuseBox && !m->powered) return "Put the fuses in the fuse box to restore the power.";
    if (m->hasFuseBox)                return "The power is on. Get to the elevator before it finds you.";
    return ch->allDone;
}

// Why the exit will not open yet: the long version (a message) and the short one (the prompt near the door).
static const char *LockedText(const Objective *o, const Chapter *ch, bool shortForm)
{
    if (shortForm) return ch->locked;
    for (int i = 0; i < COLLECT_COUNT; i++)
        if (o->got[i] < o->total[i]) return TextFormat("%s.  (%d/%d %s)", ch->locked, o->got[i], o->total[i], CollectGet((Collect)i)->plural);
    return TextFormat("%s.  Restore the power first.", ch->locked);    // only the fuse box can still be missing
}

// Left click: use whatever is in the selected slot. A new item = a new 'case' here (see inventory.h).
static void UseSelected(Player *player, Throw *thrown, const Map *map, Vector2 aim, char *toast, float *toastTime)
{
    int s = player->inv.selected;
    switch (player->inv.slot[s].id) {
        case ITEM_BATTERY:
            if (PlayerLoadBattery(player, s)) {
                TextCopy(toast, TextFormat("Battery loaded  (+%d%%)", (int)BATTERY_CHARGE));
                *toastTime = 2.0f;
                AudioBattery();
            } else {
                TextCopy(toast, "The flashlight battery is still full.");
                *toastTime = 1.8f;
            }
            break;
        case ITEM_BOTTLE:
            if (ThrowStart(thrown, map, player->pos, Vector2Add(player->pos, aim))) {
                InvRemove(&player->inv, s);
                AudioThrow();
            } else if (!thrown->flying) {
                TextCopy(toast, "No room to throw.");
                *toastTime = 1.5f;
            }
            break;
        default:
            break;                                           // an empty slot: nothing happens
    }
}

// What a chapter sounds like: the drone's pitch and the floor under your feet.
static void ApplyChapterSound(const Chapter *ch)
{
    AudioSetScene(ch->dronePitch, ch->stepPitch, ch->stepVolume);
}

// Loads a chapter's map and notes, and puts the player and the stalker at the start. False if the map file is missing.
static bool LoadChapter(int chapter, Map *map, Note *notes, int *noteCount, Objective *obj, Player *player, Stalker *stalker, Cams *cams)
{
    const Chapter *ch = &CHAPTERS[chapter];
    if (!LoadMap(map, ch->map)) return false;
    *noteCount = LoadNotes(ch->notes, notes, MAX_NOTES);
    ObjectiveInit(obj, map);
    PlayerInit(player, map->playerStart);
    player->usesBattery = ch->battery;
    StalkerInit(stalker, map);
    StalkerSetSpeed(stalker, ch->stalkerSpeed);
    CamsInit(cams, map);
    return true;
}

// Behind the title screen: a searchlight sweeping the dark, and a ghost that shows up when the light passes over it.
static void DrawTitleBackdrop(float t)
{
    int sw = GetScreenWidth(), sh = GetScreenHeight();
    Vector2 lamp = { sw / 2.0f, -30.0f };                           // the light hangs just above the top edge
    float beam = 90.0f + 42.0f * sinf(t * 0.45f);                   // degrees: 90 = straight down

    // The ghost, big, low on the right. It is only clearly visible while the beam is near it.
    Vector2 ghost = { sw * 0.80f, sh * 0.62f };
    float toGhost = atan2f(ghost.y - lamp.y, ghost.x - lamp.x) * RAD2DEG;
    float lit = 1.0f - fminf(fabsf(toGhost - beam) / 14.0f, 1.0f);   // 1 = right in the beam, 0 = out of it
    unsigned char alpha = (unsigned char)(12.0f + 200.0f * lit);
    int k = (int)(fminf((float)sw, (float)sh) * 0.36f / TILE);       // whole-number scale keeps the pixels even
    if (k < 1) k = 1;
    float size = (float)(TILE * k);
    TileDrawEx(T_STALKER, (Rectangle){ ghost.x - size / 2, ghost.y - size / 2, size, size },
               (Color){ 255, 255, 255, alpha }, false);

    // The beam: three overlapping wedges look like a soft edge.
    float reach = sh * 1.5f;
    DrawCircleSector(lamp, reach, beam - 14.0f, beam + 14.0f, 24, (Color){ 190, 190, 215, 20 });
    DrawCircleSector(lamp, reach, beam - 9.0f,  beam + 9.0f,  24, (Color){ 190, 190, 215, 24 });
    DrawCircleSector(lamp, reach, beam - 4.0f,  beam + 4.0f,  24, (Color){ 190, 190, 215, 28 });
}

// A centred menu: a red title with a faint glow and an occasional flicker, and a dark-red bar behind the selected row.
static void DrawMenu(const char *title, const char *subtitle, const char **items, int count, int selected, float t)
{
    int sw = GetScreenWidth(), sh = GetScreenHeight();

    float flicker = 1.0f;
    if (fmodf(t, 6.0f) > 5.5f && GetRandomValue(0, 2) == 0) flicker = 0.3f;
    Font font = GetFontDefault();
    float size = 72.0f, spacing = 10.0f;
    Vector2 dim = MeasureTextEx(font, title, size, spacing);
    Vector2 pos = { (sw - dim.x) / 2, sh / 7.0f };
    for (int i = 0; i < 4; i++) {                                    // glow: the same text a few pixels off, very faint
        Vector2 off = { 0.0f, 0.0f };
        if (i == 0) off.x = -3.0f;
        if (i == 1) off.x = 3.0f;
        if (i == 2) off.y = -3.0f;
        if (i == 3) off.y = 3.0f;
        DrawTextEx(font, title, (Vector2){ pos.x + off.x, pos.y + off.y }, size, spacing,
                   (Color){ 150, 20, 20, (unsigned char)(70 * flicker) });
    }
    DrawTextEx(font, title, pos, size, spacing, (Color){ 225, 50, 50, (unsigned char)(255 * flicker) });
    DrawText(subtitle, (sw - MeasureText(subtitle, 22)) / 2, (int)(pos.y + size + 18), 22, (Color){ 150, 150, 160, 255 });

    for (int i = 0; i < count; i++) {
        int y = sh * 48 / 100 + i * 52;
        int w = MeasureText(items[i], 30);
        if (i == selected) {
            DrawRectangle((sw - w) / 2 - 40, y - 8, w + 80, 46, (Color){ 110, 20, 20, 90 });
            DrawRectangle((sw - w) / 2 - 40, y + 36, w + 80, 2, (Color){ 225, 50, 50, 170 });
        }
        DrawText(items[i], (sw - w) / 2, y, 30, (i == selected) ? (Color){ 240, 215, 130, 255 } : (Color){ 130, 130, 140, 255 });
    }
}

int main(void)
{
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
    InitWindow(VIRT_W * 2, VIRT_H * 2, "St. Marrow's");
    SetWindowMinSize(VIRT_W, VIRT_H);
    SetExitKey(KEY_NULL);                            // Esc opens the pause menu instead of closing the game
    SetTargetFPS(60);
    AudioInit();                                     // no sound device? the game just runs silently
    ChangeDirectory(GetApplicationDirectory());      // so "assets/..." is found next to the EXE
    int volume, unlocked;
    bool fullscreen;
    LoadSettings(&volume, &unlocked, &fullscreen);   // volume 0..10, unlocked chapters and fullscreen, remembered between runs
    if (fullscreen) ToggleBorderlessWindowed();
    AudioSetVolume(volume / 10.0f);
    TilesLoad();                                     // sprites from assets/tiles (plain shapes if not set up)

    Map map;
    Note notes[MAX_NOTES];
    int noteCount = 0;
    Player player;
    Stalker stalker;
    Objective obj;
    Throw thrown;                      // the bottle in the air (Chapter 3)
    Cams cams;                         // the security cameras (Chapter 4)
    Hunt hunt;                         // the final hunt (Chapter 5)
    const char *endReason = NULL;      // the title on the "caught" screen when it is not the stalker (NULL = IT FOUND YOU)
    int chapter = 0;                   // the chapter selected on the title screen / being played (0 = chapter 1)
    if (!LoadChapter(chapter, &map, notes, &noteCount, &obj, &player, &stalker, &cams)) {
        while (!WindowShouldClose() && !IsKeyPressed(KEY_ESCAPE)) {
            BeginDrawing();
            ClearBackground(BLACK);
            DrawText(TextFormat("Could not load %s", CHAPTERS[chapter].map), 20, 20, 20, RED);
            DrawText("Run the game from its own folder (the one that contains 'assets').", 20, 50, 20, RAYWHITE);
            EndDrawing();
        }
        AudioShutdown();
        CloseWindow();
        return 1;
    }
    ThrowInit(&thrown);
    HuntInit(&hunt);
    ApplyChapterSound(&CHAPTERS[chapter]);

    RenderTexture2D scene = LoadRenderTexture(VIRT_W, VIRT_H);   // the game picture
    RenderTexture2D mask  = LoadRenderTexture(VIRT_W, VIRT_H);   // the darkness, multiplied over the picture
    SetTextureFilter(scene.texture, TEXTURE_FILTER_POINT);
    LightKit lights;
    LightKitInit(&lights);

    Camera2D cam = { .offset = { VIRT_W / 2.0f, VIRT_H / 2.0f }, .target = player.pos, .rotation = 0.0f, .zoom = 1.0f };
    GameState state = GS_MENU;         // the game opens on the title screen
    int menuSel = 0;                   // which menu row is selected
    char menuMsg[128] = "";            // an error shown on the title screen (e.g. a missing map file)
    bool quit = false;
    bool debug = false;
    bool tileBrowser = false;          // F5: shows every tile of the sheet with its number
    int openNote = -1;                 // which note is being read
    float runTime = 0;                 // seconds spent in this run
    char toast[96] = "";               // short message at the bottom of the screen
    float toastTime = 0;
    float shake = 0.0f;                // screen shake: 0..1, fades out by itself
    float creditsY = 0.0f;             // where the credits are scrolled to

    while (!quit && !WindowShouldClose()) {
        float dt = fminf(GetFrameTime(), 0.05f);
        if (toastTime > 0) toastTime -= dt;
        shake = fmaxf(0.0f, shake - dt * 2.2f);

        bool up     = IsKeyPressed(KEY_W) || IsKeyPressed(KEY_UP);
        bool down   = IsKeyPressed(KEY_S) || IsKeyPressed(KEY_DOWN);
        bool left   = IsKeyPressed(KEY_A) || IsKeyPressed(KEY_LEFT);
        bool right  = IsKeyPressed(KEY_D) || IsKeyPressed(KEY_RIGHT);
        bool accept = IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE);
        bool startRun = false;                               // set by: menu Play, pause Restart, R after dying
        int selectable = debug ? CHAPTER_COUNT : unlocked;   // chapters the title screen lets you pick (F3 opens them all, for testing)

        // ---- Update ----
        if (IsKeyPressed(KEY_ESCAPE)) {                      // Esc: pause, close a note, or go back to the title
            if (state == GS_PLAYING) {
                state = GS_PAUSED;
                menuSel = 0;
            } else if (state == GS_READING) {
                state = GS_PLAYING;
                AudioPaper();
            } else if (state == GS_PAUSED) {
                state = GS_PLAYING;
            } else if (state == GS_CAUGHT || state == GS_COMPLETE || state == GS_CREDITS) {
                if (state == GS_COMPLETE && chapter + 1 < unlocked) chapter++;   // the title screen offers the next chapter
                state = GS_MENU;
                menuSel = 0;
            }
        }

        if (state == GS_PLAYING || state == GS_READING) {
            runTime += dt;
            if (state == GS_PLAYING) {
                bool lightWasOn = player.flashlight;
                float chargeBefore = player.battery;
                for (int i = 0; i < INV_SLOTS; i++)                      // keys 1-6 pick a slot, and so does the mouse wheel
                    if (IsKeyPressed(KEY_ONE + i)) InvSelect(&player.inv, i);
                float wheel = GetMouseWheelMove();
                if (wheel > 0.0f) InvScroll(&player.inv, -1);
                if (wheel < 0.0f) InvScroll(&player.inv, 1);
                PlayerUpdate(&player, &map, dt);
                if (player.battery > chargeBefore + 1.0f) {              // F on a dead flashlight loaded a spare battery
                    TextCopy(toast, "Spare battery loaded.");
                    toastTime = 2.0f;
                    AudioBattery();
                }
                if (player.usesBattery && !player.hidden && player.battery <= 0.0f) {
                    if (lightWasOn && !player.flashlight) {              // it just ran out
                        TextCopy(toast, InvFind(&player.inv, ITEM_BATTERY) >= 0 ? "The flashlight died.  Press F to load a spare battery."
                                                                                : "The flashlight died.");
                        toastTime = 3.0f;
                        AudioFlashlightDead();
                    } else if (!lightWasOn && IsKeyPressed(KEY_F)) {     // pressing F on a dead flashlight with no spare
                        TextCopy(toast, "No battery left.");
                        toastTime = 2.0f;
                        AudioFlashlightDead();
                    }
                }
                if (!player.hidden) {                                    // keys, fuses, files, cards: walk over one to collect it
                    Collect found;
                    if (PickUpCollectible(&obj, &map, player.pos, &found)) {
                        TextCopy(toast, TextFormat("%s found  (%d/%d)", CollectGet(found)->name, obj.got[found], obj.total[found]));
                        toastTime = 2.5f;
                        AudioChime();
                    }
                }
                if (CHAPTERS[chapter].hunt > 0.0f && !hunt.active && ObjectiveDone(&obj, &map)) {   // the last thing is taken: the building turns on you
                    HuntStart(&hunt, CHAPTERS[chapter].hunt);
                    StalkerSetSpeed(&stalker, CHAPTERS[chapter].stalkerSpeed * HUNT_SPEED);
                    StalkerAlert(&stalker, &map, player.pos);
                    shake = fminf(1.0f, shake + 0.8f);
                    TextCopy(toast, "The building knows.  RUN to the roof door!");
                    toastTime = 4.0f;
                    AudioAlarm();
                }
                if (!player.hidden) {                                    // batteries, bottles ...: walk over an item to pick it up
                    ItemId got = PickUpItem(&player.inv, &map, player.pos);
                    if (got != ITEM_NONE) {
                        TextCopy(toast, TextFormat("%s  (you have %d)", ItemGet(got)->name, InvCount(&player.inv, got)));
                        toastTime = 2.0f;
                        if (got == ITEM_BOTTLE) AudioBottle();
                        else if (got == ITEM_BATTERY) AudioBattery();
                        else AudioChime();
                    }
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
                    } else if (FuseBoxNear(&map, player.pos)) {
                        if (map.powered) {
                            TextCopy(toast, "The power is already on.");
                            toastTime = 2.0f;
                        } else if (obj.got[COLLECT_FUSE] >= obj.total[COLLECT_FUSE]) {   // the breaker catches: loud. The stalker goes to look.
                            map.powered = true;
                            shake = fminf(1.0f, shake + 0.6f);
                            StalkerAlert(&stalker, &map, player.pos);
                            TextCopy(toast, "Power restored.  The whole wing heard that.");
                            toastTime = 4.0f;
                            AudioPowerOn();
                        } else {
                            TextCopy(toast, TextFormat("The fuse box needs %d fuses.  You have %d.", obj.total[COLLECT_FUSE], obj.got[COLLECT_FUSE]));
                            toastTime = 2.5f;
                            AudioLockedDoor();
                        }
                    } else if (ExitNear(&map, player.pos)) {
                        if (ObjectiveDone(&obj, &map)) {
                            state = GS_COMPLETE;
                            AudioComplete();
                            if (chapter + 2 > unlocked && chapter + 1 < CHAPTER_COUNT) {   // the next chapter opens
                                unlocked = chapter + 2;
                                SaveSettings(volume, unlocked, fullscreen);
                            }
                        } else {
                            TextCopy(toast, LockedText(&obj, &CHAPTERS[chapter], false));
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
            Vector2 landed;                                          // a bottle in the air: when it shatters, the stalker may hear it
            if (ThrowUpdate(&thrown, dt, &landed)) {
                float dist = Vector2Distance(landed, player.pos);
                shake = fminf(1.0f, shake + 0.3f * Clamp(1.0f - dist / 220.0f, 0.0f, 1.0f));     // a bottle breaking close by makes you flinch
                AudioGlass(Clamp(1.0f - dist / 700.0f, 0.25f, 1.0f), Clamp((landed.x - player.pos.x) / 160.0f, -1.0f, 1.0f));
                if (Vector2Distance(stalker.pos, landed) < THROW_NOISE_RADIUS) StalkerAlert(&stalker, &map, landed);
                if (CamsBreakNear(&cams, landed)) {                  // a bottle shattered right next to a camera
                    TextCopy(toast, "Camera destroyed.");
                    toastTime = 2.5f;
                    AudioCameraBreak();
                }
            }
            Vector2 seenAt;                                          // security cameras: if one rings the alarm, the stalker goes to where it saw you
            switch (HuntUpdate(&hunt, dt)) {                         // the hunt: every few seconds the stalker is told where you are
                case HUNT_PING_NOW: StalkerAlert(&stalker, &map, player.pos); break;
                case HUNT_TIMEOUT:  if (state != GS_COMPLETE) { state = GS_CAUGHT; endReason = "THE DOORS SEALED"; shake = 1.0f; AudioSting(); } break;
                default: break;
            }
            if (state != GS_COMPLETE && CamsUpdate(&cams, &map, player.pos, player.hidden || hunt.active, dt, &seenAt)) {
                StalkerAlert(&stalker, &map, seenAt);
                shake = fminf(1.0f, shake + 0.5f);
                TextCopy(toast, "ALARM!  A camera saw you.");
                toastTime = 3.0f;
                AudioAlarm();
            }
            StalkerState wasState = stalker.state;
            if (state != GS_COMPLETE && state != GS_CAUGHT && StalkerUpdate(&stalker, &map, &player, dt)) { state = GS_CAUGHT; endReason = NULL; shake = 1.0f; AudioSting(); }
            if (wasState != ST_CHASE && stalker.state == ST_CHASE) shake = fminf(1.0f, shake + 0.35f);   // it has seen you
        } else if (state == GS_MENU || state == GS_PAUSED) {
            int count = 4;                                   // title: Play, Chapter, Volume, Quit.   pause: Resume, Volume, Restart, Quit to title
            int volRow = (state == GS_MENU) ? 2 : 1;
            if (state == GS_MENU && chapter >= selectable) chapter = selectable - 1;
            if (up)   menuSel = (menuSel + count - 1) % count;
            if (down) menuSel = (menuSel + 1) % count;
            if (state == GS_MENU && menuSel == 1 && (left || right)) {       // the chapter row
                int next = chapter + (right ? 1 : -1);
                if (next >= 0 && next < selectable) { chapter = next; AudioPaper(); }
            }
            if (menuSel == volRow && (left || right)) {      // the volume row
                volume += right ? 1 : -1;
                if (volume < 0) volume = 0;
                if (volume > 10) volume = 10;
                AudioSetVolume(volume / 10.0f);
                SaveSettings(volume, unlocked, fullscreen);
                AudioChime();                                // so you can hear the new level
            }
            if (accept) {
                if (state == GS_MENU) {
                    if (menuSel == 0 || menuSel == 1) startRun = true;
                    else if (menuSel == 3) quit = true;
                } else {
                    if (menuSel == 0) state = GS_PLAYING;
                    else if (menuSel == 2) startRun = true;
                    else if (menuSel == 3) { state = GS_MENU; menuSel = 0; }
                }
            }
        } else if (state == GS_CREDITS) {                    // the credits scroll up and stop; Enter goes back to the title
            creditsY = fmaxf(creditsY - CREDITS_SPEED * dt, CreditsEndY(GetScreenHeight()));
            if (accept) { state = GS_MENU; menuSel = 0; }
        } else {                                             // caught or finished
            if (IsKeyPressed(KEY_R)) startRun = true;        // R starts the chapter over
            else if (state == GS_COMPLETE && accept) {
                if (chapter + 1 < CHAPTER_COUNT) { chapter++; startRun = true; }                 // Enter: next chapter...
                else { state = GS_CREDITS; creditsY = (float)GetScreenHeight(); }                // ...or, after the finale, the credits
            }
        }

        if (startRun) {
            if (LoadChapter(chapter, &map, notes, &noteCount, &obj, &player, &stalker, &cams)) {   // reloads the map, so everything you collected is back
                runTime = 0;
                openNote = -1;
                ThrowInit(&thrown);
                HuntInit(&hunt);
                ApplyChapterSound(&CHAPTERS[chapter]);
                endReason = NULL;
                menuMsg[0] = '\0';
                TextCopy(toast, TextFormat("Chapter %d: %s", chapter + 1, CHAPTERS[chapter].name));
                toastTime = 3.5f;
                state = GS_PLAYING;
            } else {
                TextCopy(menuMsg, TextFormat("Could not load %s. Is the assets folder next to the game?", CHAPTERS[chapter].map));
                state = GS_MENU;
                menuSel = 1;
            }
        }
        if (IsKeyPressed(KEY_F11)) {                         // fullscreen on / off (borderless: the screen resolution does not change)
            ToggleBorderlessWindowed();
            fullscreen = IsWindowState(FLAG_BORDERLESS_WINDOWED_MODE);
            SaveSettings(volume, unlocked, fullscreen);
        }
        if (IsKeyPressed(KEY_F3)) debug = !debug;
        if (IsKeyPressed(KEY_F5)) tileBrowser = !tileBrowser;
        if (IsKeyPressed(KEY_F6)) TilesLoad();               // reload tiles.txt and the sheet without restarting
        AudioSetHunt(hunt.active && (state == GS_PLAYING || state == GS_READING));      // the hunt: the heartbeat never stops
        AudioUpdate(&player, &stalker, state == GS_PLAYING || state == GS_READING, dt);

        cam.target = (Vector2){ roundf(player.pos.x), roundf(player.pos.y) };   // whole pixels = no shimmer
        int jolt = (int)ceilf(SHAKE_MAX * shake * shake);                       // screen shake: a random whole-pixel jolt that fades out
        if (jolt > 0) {
            cam.target.x += (float)GetRandomValue(-jolt, jolt);
            cam.target.y += (float)GetRandomValue(-jolt, jolt);
        }

        // Mouse position: window pixels -> game pixels.
        float scale = GetViewScale();
        Vector2 origin = { (GetScreenWidth() - VIRT_W * scale) / 2, (GetScreenHeight() - VIRT_H * scale) / 2 };
        Vector2 mouse = { (GetMouseX() - origin.x) / scale, (GetMouseY() - origin.y) / scale };
        Vector2 me = GetWorldToScreen2D(player.pos, cam);          // player position on the 640x360 picture
        Vector2 aim = Vector2Subtract(mouse, me);
        float aimDeg = (aim.x == 0 && aim.y == 0) ? 0 : atan2f(aim.y, aim.x) * RAD2DEG;

        if (state == GS_PLAYING && !player.hidden && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
            UseSelected(&player, &thrown, &map, aim, toast, &toastTime);       // click = use the item in the selected slot

        // ---- Draw 1: the darkness mask (dark everywhere, light added where the lights are) ----
        float now = (float)GetTime();
        Color ambient = AMBIENT;
        if (map.powered || hunt.active) {                    // emergency lighting (power on, or the hunt): a slow red pulse
            float pulse = 0.5f + 0.5f * sinf(now * 2.2f);
            ambient = (Color){ (unsigned char)(50.0f + 18.0f * pulse), 36, 44, 255 };
        }
        ClearMask(mask, ambient);
        if (player.hidden) {
            AddLight(&lights, mask, &map, cam, lights.glow, player.pos, 20.0f, 0);       // only a faint glow inside the locker
        } else {
            AddLight(&lights, mask, &map, cam, lights.glow, player.pos, GLOW_RANGE, 0);
            if (player.flashlight) {
                float range = BEAM_RANGE;
                bool beamOn = true;
                if (player.usesBattery) {
                    float charge = player.battery / BATTERY_MAX;
                    if (charge < BATTERY_DIM_AT) range *= 0.55f + 0.45f * (charge / BATTERY_DIM_AT);   // a weak battery: a shorter beam...
                    if (charge < BATTERY_FLICKER_AT && sinf(now * 41.0f) * sinf(now * 17.0f + 1.0f) > 0.45f)
                        beamOn = false;                                                                // ...that flickers
                }
                if (beamOn) AddLight(&lights, mask, &map, cam, lights.beam, player.pos, range, aimDeg);
            }
        }
        CamsLights(&cams, &lights, mask, &map, cam, player.pos);           // each camera's cone of view is lit
        if (map.powered) {                                   // the elevator glows once it has power (only drawn when it is near)
            Vector2 lamp;
            if (ExitLightPos(&map, &lamp) && Vector2Distance(lamp, player.pos) < 380.0f)
                AddLight(&lights, mask, &map, cam, lights.glow, lamp, 56.0f, 0);
        }

        // ---- Draw 2: the world, then the mask multiplied on top ----
        BeginTextureMode(scene);
            ClearBackground(BLACK);
            BeginMode2D(cam);
                // You only see the stalker, keys and notes if there is a clear line to them (hiding: you peek out of the locker door).
                Vector2 eyes = player.hidden ? player.exitPos : player.pos;
                DrawMap(&map, cam, eyes);
                CamsDraw(&cams);
                if (HasLineOfSight(&map, eyes, stalker.pos)) DrawStalker(&stalker);
                DrawPlayer(&player, aim);                                    // (nothing while you hide)
                ThrowDraw(&thrown);
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

            DrawText("WASD move  |  Shift run (loud)  |  Mouse aim  |  F flashlight  |  E use / hide  |  M mute  |  R restart  |  F3 debug  |  Esc pause",
                     12, 10, 16, (Color){ 200, 200, 200, 170 });
            DrawText(GoalText(&obj, &map, &CHAPTERS[chapter]), 12, 34, 20, (Color){ 235, 205, 110, 235 });

            if (hunt.active) {                               // the hunt: a pulsing red edge and the clock
                float pulse = 0.5f + 0.5f * sinf(now * 6.0f);
                DrawRectangleLinesEx((Rectangle){ 0, 0, (float)sw, (float)sh }, 10.0f, Fade((Color){ 220, 30, 30, 255 }, 0.15f + 0.25f * pulse));
                int secs = (int)ceilf(hunt.left);
                const char *clock = TextFormat("RUN   %d:%02d", secs / 60, secs % 60);
                Color cc = (hunt.left < 20.0f && fmodf(now, 0.5f) < 0.25f) ? WHITE : (Color){ 235, 60, 50, 255 };
                DrawText(clock, (sw - MeasureText(clock, 40)) / 2, 14, 40, cc);
            }
            float watched = CamsSeen(&cams);                 // a red edge closes in while a camera is watching you
            if (watched > 0.0f)
                DrawRectangleLinesEx((Rectangle){ 0, 0, (float)sw, (float)sh }, 6.0f + 22.0f * watched, Fade((Color){ 220, 30, 30, 255 }, 0.25f + 0.5f * watched));
            if (cams.alarm > 0.0f && fmodf(now, 0.6f) < 0.35f)
                DrawText("ALARM", (sw - MeasureText("ALARM", 32)) / 2, 70, 32, (Color){ 235, 60, 50, 255 });

            if (player.usesBattery) {                       // flashlight battery: a bar that turns yellow, then red and blinking
                float charge = player.battery / BATTERY_MAX;
                Color bc = (charge > BATTERY_DIM_AT) ? (Color){ 110, 220, 130, 235 }
                         : (charge > BATTERY_FLICKER_AT) ? (Color){ 235, 200, 80, 235 } : (Color){ 235, 70, 60, 235 };
                if (charge <= BATTERY_FLICKER_AT && fmodf(now, 0.8f) > 0.5f) bc.a = 90;
                DrawText("BATTERY", 12, sh - 34, 16, (Color){ 200, 200, 200, 190 });
                DrawRectangle(88, sh - 33, 124, 14, (Color){ 20, 20, 24, 200 });
                DrawRectangle(90, sh - 31, (int)(120.0f * charge), 10, bc);
                if (player.battery <= 0.0f) DrawText("DEAD", 222, sh - 34, 16, (Color){ 235, 70, 60, 235 });
            }
            if (CHAPTERS[chapter].inventory) InvDraw(&player.inv, sw, sh);    // the hotbar
            int lift = CHAPTERS[chapter].inventory ? 56 : 0;                  // the hotbar takes the bottom of the screen: messages move up

            // What E would do right now.
            const char *prompt = NULL;
            if (state == GS_PLAYING) {
                Vector2 lockerAt;
                if (player.hidden)                              prompt = "Hiding...   press E to climb out";
                else if (NoteNear(&map, player.pos) >= 0)       prompt = "E   read note";
                else if (FuseBoxNear(&map, player.pos))         prompt = map.powered ? "The power is on"
                                                                       : (obj.got[COLLECT_FUSE] >= obj.total[COLLECT_FUSE]) ? "E   put in the fuses  (loud!)"
                                                                       : TextFormat("The fuse box needs %d fuses  (you have %d)", obj.total[COLLECT_FUSE], obj.got[COLLECT_FUSE]);
                else if (ExitNear(&map, player.pos))            prompt = ObjectiveDone(&obj, &map) ? TextFormat("E   %s", CHAPTERS[chapter].exitUse) : LockedText(&obj, &CHAPTERS[chapter], true);
                else if (ItemNear(&map, player.pos) != ITEM_NONE) prompt = "Inventory full.  Use something first.";
                else if (PlayerNearLocker(&player, &map, &lockerAt)) prompt = "E   hide in locker";
            }
            if (prompt)
                DrawText(prompt, (sw - MeasureText(prompt, 22)) / 2, sh - 56 - lift, 22, (Color){ 230, 230, 230, 220 });
            if (toastTime > 0)
                DrawText(toast, (sw - MeasureText(toast, 24)) / 2, sh - 100 - lift, 24, Fade(WHITE, fminf(1.0f, toastTime)));

            if (debug) {
                DrawFPS(12, 62);
                DrawText(TextFormat("tile %d, %d", (int)(player.pos.x / TILE), (int)(player.pos.y / TILE)), 12, 84, 16, GREEN);
                DrawText(TextFormat("stalker: %s   distance %d   %s", StalkerStateName(&stalker),
                                    (int)Vector2Distance(stalker.pos, player.pos),
                                    player.hidden ? (stalker.knowsHiding ? "HIDDEN (it saw you!)" : "HIDDEN (unseen)") : ""),
                         12, 106, 16, ORANGE);
                if (player.usesBattery) DrawText(TextFormat("battery %.0f / %.0f", player.battery, BATTERY_MAX), 12, 128, 16, YELLOW);
                if (cams.count > 0) DrawText(TextFormat("cameras %d   watched %.2f   alarm %.1f s", cams.count, CamsSeen(&cams), cams.alarm), 12, 150, 16, RED);
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
                    DrawWrapped(TextFormat("Write this note in %s", CHAPTERS[chapter].notes), px + 24, py + 70, pw - 48, 22, (Color){ 225, 220, 200, 255 });
                }
                DrawText("E or Esc  close          (the stalker is still out there)", px + 24, py + ph - 34, 18, (Color){ 160, 150, 130, 255 });
            }
            if (state == GS_CAUGHT) {
                DrawRectangle(0, 0, sw, sh, (Color){ 60, 0, 0, 170 });
                const char *title = endReason ? endReason : "IT FOUND YOU";
                const char *hint = "Press R to try again     (Esc: title screen)";
                DrawText(title, (sw - MeasureText(title, 48)) / 2, sh / 2 - 40, 48, (Color){ 225, 50, 50, 255 });
                DrawText(hint, (sw - MeasureText(hint, 20)) / 2, sh / 2 + 20, 20, RAYWHITE);
            }
            if (state == GS_COMPLETE) {
                DrawRectangle(0, 0, sw, sh, (Color){ 0, 0, 0, 170 });
                bool last = (chapter + 1 >= CHAPTER_COUNT);               // the finale gets an ending
                const char *title = last ? "THE END" : TextFormat("CHAPTER %d COMPLETE", chapter + 1);
                const char *timeText = TextFormat("%s   -   Time  %d:%02d", CHAPTERS[chapter].name, (int)runTime / 60, (int)runTime % 60);
                const char *hint = last ? "Enter  credits        R  play again        Esc  title screen"
                                        : "Enter  next chapter        R  play again        Esc  title screen";
                DrawText(title, (sw - MeasureText(title, 48)) / 2, sh / 2 - 60, 48, (Color){ 235, 205, 110, 255 });
                DrawText(timeText, (sw - MeasureText(timeText, 26)) / 2, sh / 2 + 6, 26, RAYWHITE);
                int hintY = sh / 2 + 48;
                if (last) {
                    const char *thanks = "You made it out of St. Marrow's.   Thank you for playing.";
                    DrawText(thanks, (sw - MeasureText(thanks, 22)) / 2, sh / 2 + 48, 22, (Color){ 225, 220, 200, 255 });
                    hintY = sh / 2 + 92;
                }
                DrawText(hint, (sw - MeasureText(hint, 20)) / 2, hintY, 20, (Color){ 190, 190, 190, 255 });
            }
            if (state == GS_CREDITS) {                                       // the credits
                DrawRectangle(0, 0, sw, sh, (Color){ 6, 6, 10, 255 });
                for (int i = 0; i < CREDITS_LINES; i++) {
                    const char *line = CREDITS[i];
                    bool head = (line[0] == '#');
                    if (head) line++;
                    float y = creditsY + i * CREDITS_LINE_H;
                    if (line[0] == '\0' || y < -40.0f || y > sh + 10.0f) continue;
                    int size = head ? 30 : 22;
                    DrawText(line, (sw - MeasureText(line, size)) / 2, (int)y, size, head ? (Color){ 235, 205, 110, 255 } : (Color){ 225, 220, 200, 255 });
                }
                if (creditsY <= CreditsEndY(sh) + 0.5f && fmodf(now, 1.2f) < 0.8f) {
                    const char *back = "Enter  back to the title screen";
                    DrawText(back, (sw - MeasureText(back, 20)) / 2, sh - 60, 20, (Color){ 190, 190, 190, 255 });
                }
            }
            if (state == GS_MENU || state == GS_PAUSED) {
                float t = (float)GetTime();
                DrawRectangle(0, 0, sw, sh, (state == GS_MENU) ? (Color){ 0, 0, 0, 255 } : (Color){ 0, 0, 0, 200 });
                if (state == GS_MENU) DrawTitleBackdrop(t);

                char bar[11];                                        // the volume as ten little bars
                for (int i = 0; i < 10; i++) bar[i] = (i < volume) ? '|' : '.';
                bar[10] = '\0';
                char volText[64];
                TextCopy(volText, TextFormat("Volume   < %s >%s", bar, AudioIsMuted() ? "  (muted)" : ""));

                if (state == GS_MENU) {
                    char chapText[64];                               // "Chapter < 2: Surgery Wing >" (arrows only if there is another to pick)
                    if (selectable > 1) TextCopy(chapText, TextFormat("Chapter   < %d: %s >", chapter + 1, CHAPTERS[chapter].name));
                    else                TextCopy(chapText, TextFormat("Chapter   %d: %s", chapter + 1, CHAPTERS[chapter].name));
                    const char *items[4] = { "Play", chapText, volText, "Quit" };
                    DrawMenu("ST. MARROW'S", TextFormat("Mode 1  -  Chapter %d: %s", chapter + 1, CHAPTERS[chapter].name), items, 4, menuSel, t);
                } else {
                    const char *items[4] = { "Resume", volText, "Restart", "Quit to title" };
                    DrawMenu("PAUSED", "", items, 4, menuSel, t);
                }
                const char *help = "W / S  choose        A / D  change        Enter  select        Esc  back        F11  fullscreen";
                DrawText(help, (sw - MeasureText(help, 16)) / 2, sh - 44, 16, (Color){ 110, 110, 120, 255 });
                if (state == GS_MENU) {
                    DrawText(TITLE_CREDIT, 16, sh - 26, 14, (Color){ 90, 90, 100, 255 });
                    if (menuMsg[0]) DrawText(menuMsg, (sw - MeasureText(menuMsg, 18)) / 2, sh - 76, 18, (Color){ 230, 90, 80, 255 });
                    if (t < 1.5f)                                    // fade in from black when the game starts
                        DrawRectangle(0, 0, sw, sh, (Color){ 0, 0, 0, (unsigned char)(255.0f * (1.0f - t / 1.5f)) });
                }
            }
            if (tileBrowser) TilesBrowser();
        EndDrawing();
    }

    TilesUnload();
    LightKitUnload(&lights);
    UnloadRenderTexture(scene);
    UnloadRenderTexture(mask);
    AudioShutdown();
    CloseWindow();
    return 0;
}