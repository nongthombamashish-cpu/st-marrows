#include "tiles.h"
#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CONFIG_FILE "assets/tiles/tiles.txt"

static Texture2D sheet;                                 // the tileset picture
static bool loaded = false;
static int tileSize = 16, spacing = 0, margin = 0;      // how the picture is laid out
static int cols = 0, rows = 0;                          // how many tiles it holds
static int slot[T_COUNT];                               // which sheet tile draws each thing (-1 = not set)
static int page = 0;                                    // current page of the browser
static const char *names[T_COUNT] = { "floor1", "floor2", "wall", "locker", "key", "note", "exit", "player", "stalker", "battery", "fuse", "fusebox", "table", "bottle", "file", "shelf", "card", "ledger" };

// Removes spaces at both ends of a string, in place.
static void Trim(char *s)
{
    char *start = s;
    while (*start == ' ' || *start == '\t') start++;
    size_t len = strlen(start);
    while (len > 0 && (start[len - 1] == ' ' || start[len - 1] == '\t' || start[len - 1] == '\r')) len--;
    memmove(s, start, len);
    s[len] = '\0';
}

void TilesUnload(void)
{
    if (loaded) UnloadTexture(sheet);
    loaded = false;
}

bool TilesLoad(void)
{
    TilesUnload();
    for (int i = 0; i < T_COUNT; i++) slot[i] = -1;
    slot[T_BOTTLE] = 113; slot[T_FILE] = 66; slot[T_SHELF] = 63; slot[T_CARD] = 101; slot[T_LEDGER] = 66;      // Chapter 3 art by default, so an older tiles.txt still works (set -1 there for plain shapes)
    tileSize = 16; spacing = 0; margin = 0;
    char path[256] = "assets/tiles/tileset.png";

    char *text = LoadFileText(CONFIG_FILE);
    if (!text) return false;                            // no tiles.txt: the game uses its plain shapes

    const char *p = text;
    while (*p) {
        char line[256];
        int n = 0;
        while (*p && *p != '\n') {
            if (n < 255) line[n++] = *p;
            p++;
        }
        line[n] = '\0';
        if (*p == '\n') p++;

        char *eq = strchr(line, '=');
        if (line[0] == ';' || eq == NULL) continue;     // a comment, or not a setting
        *eq = '\0';
        char *key = line, *val = eq + 1;
        Trim(key);
        Trim(val);
        if (strcmp(key, "sheet") == 0) {
            snprintf(path, sizeof path, "%s", val);
        } else if (strcmp(key, "size") == 0) {
            tileSize = atoi(val);
        } else if (strcmp(key, "spacing") == 0) {
            spacing = atoi(val);
        } else if (strcmp(key, "margin") == 0) {
            margin = atoi(val);
        } else {
            for (int i = 0; i < T_COUNT; i++) {
                if (strcmp(key, names[i]) == 0) slot[i] = atoi(val);
            }
        }
    }
    UnloadFileText(text);
    if (tileSize < 1) tileSize = 16;
    if (spacing < 0) spacing = 0;
    if (margin < 0) margin = 0;

    sheet = LoadTexture(path);
    if (sheet.id == 0) {
        TraceLog(LOG_WARNING, "TILES: could not load %s", path);
        return false;
    }
    SetTextureFilter(sheet, TEXTURE_FILTER_POINT);
    cols = (sheet.width - 2 * margin + spacing) / (tileSize + spacing);
    rows = (sheet.height - 2 * margin + spacing) / (tileSize + spacing);
    if (cols < 1 || rows < 1) {
        UnloadTexture(sheet);
        return false;
    }
    loaded = true;
    return true;
}

bool TileDrawEx(TileId id, Rectangle dst, Color tint, bool flipX)
{
    if (!loaded || slot[id] < 0 || slot[id] >= cols * rows) return false;
    int i = slot[id];
    Rectangle src = { (float)(margin + (i % cols) * (tileSize + spacing)),
                      (float)(margin + (i / cols) * (tileSize + spacing)),
                      (float)tileSize, (float)tileSize };
    if (flipX) src.width = -src.width;                  // a negative width mirrors the sprite
    DrawTexturePro(sheet, src, dst, (Vector2){ 0, 0 }, 0.0f, tint);
    return true;
}

bool TileDraw(TileId id, float x, float y, bool flipX)
{
    return TileDrawEx(id, (Rectangle){ x, y, (float)TILE, (float)TILE }, WHITE, flipX);
}

void TilesBrowser(void)
{
    int sw = GetScreenWidth(), sh = GetScreenHeight();
    DrawRectangle(0, 0, sw, sh, (Color){ 12, 12, 18, 250 });
    if (!loaded) {
        DrawText("No tileset loaded.", 40, 40, 24, (Color){ 235, 205, 110, 255 });
        DrawText("Put your sheet at assets/tiles/tileset.png, check assets/tiles/tiles.txt, then press F6.", 40, 80, 20, RAYWHITE);
        DrawText("F5: close", 40, 120, 20, GRAY);
        return;
    }

    int cell = 3 * tileSize, w = cell + 8, h = cell + 22;      // each tile drawn 3x bigger, its number below
    int perRow = (sw - 8) / w;  if (perRow < 1) perRow = 1;
    int perCol = (sh - 80) / h; if (perCol < 1) perCol = 1;
    int perPage = perRow * perCol;
    int total = cols * rows;
    int pages = (total + perPage - 1) / perPage;

    if (IsKeyPressed(KEY_RIGHT_BRACKET) || IsKeyPressed(KEY_PAGE_DOWN)) page++;
    if (IsKeyPressed(KEY_LEFT_BRACKET) || IsKeyPressed(KEY_PAGE_UP)) page--;
    if (page > pages - 1) page = pages - 1;
    if (page < 0) page = 0;

    DrawText(TextFormat("Tileset: %d x %d tiles (%d in total)     page %d / %d", cols, rows, total, page + 1, pages),
             20, 14, 20, (Color){ 235, 205, 110, 255 });
    DrawText("[ and ] (or PageUp / PageDown): change page      F6: reload tiles.txt      F5: close", 20, 42, 16, GRAY);

    int x0 = (sw - perRow * w) / 2;
    if (x0 < 4) x0 = 4;
    for (int i = 0; i < perPage; i++) {
        int idx = page * perPage + i;
        if (idx >= total) break;
        int x = x0 + (i % perRow) * w, y = 72 + (i / perRow) * h;
        Rectangle src = { (float)(margin + (idx % cols) * (tileSize + spacing)),
                          (float)(margin + (idx / cols) * (tileSize + spacing)),
                          (float)tileSize, (float)tileSize };
        DrawRectangle(x, y, cell, cell, (Color){ 60, 60, 72, 255 });     // grey behind, so see-through tiles show up
        DrawTexturePro(sheet, src, (Rectangle){ (float)x, (float)y, (float)cell, (float)cell }, (Vector2){ 0, 0 }, 0.0f, WHITE);
        DrawText(TextFormat("%d", idx), x, y + cell + 3, 14, RAYWHITE);
    }
}