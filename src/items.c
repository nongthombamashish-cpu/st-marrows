#include "items.h"
#include "raymath.h"
#include <math.h>
#include <string.h>

// ---- The collectibles table: one row per kind ----
static const CollectInfo COLLECTIBLES[COLLECT_COUNT] = {
    [COLLECT_KEY]  = { 'K', "Key",          "keys",          T_KEY,  { 230, 190,  70, 255 } },
    [COLLECT_FUSE] = { 'F', "Fuse",         "fuses",         T_FUSE, { 214,  96,  74, 255 } },
    [COLLECT_FILE] = { 'R', "Patient file", "patient files", T_FILE, { 226, 214, 170, 255 } },
    [COLLECT_CARD] = { 'A', "Access card",  "access cards",  T_CARD, { 120, 180, 230, 255 } },
    [COLLECT_LEDGER] = { 'M', "Ledger",     "ledger",        T_LEDGER, { 150,  90,  60, 255 } },
};

const CollectInfo *CollectGet(Collect c)
{
    if (c < 0 || c >= COLLECT_COUNT) c = COLLECT_KEY;
    return &COLLECTIBLES[c];
}

Collect CollectFromMapChar(char c)
{
    for (int i = 0; i < COLLECT_COUNT; i++)
        if (COLLECTIBLES[i].mapChar == c) return (Collect)i;
    return COLLECT_COUNT;
}

void CollectDraw(Collect c, float x, float y)
{
    const CollectInfo *it = CollectGet(c);
    if (TileDraw(it->tile, x, y, false)) return;
    DrawRectangle((int)x + 5, (int)y + 3, 6, 10, it->color);                   // a plain little shape
    DrawRectangleLines((int)x + 5, (int)y + 3, 6, 10, (Color){ 40, 40, 40, 255 });
}

void ObjectiveInit(Objective *o, const Map *m)
{
    for (int i = 0; i < COLLECT_COUNT; i++) o->got[i] = o->total[i] = 0;
    for (int y = 0; y < m->h; y++) {
        for (int x = 0; x < m->w; x++) {
            Collect c = CollectFromMapChar(m->tiles[y][x]);
            if (c != COLLECT_COUNT) o->total[c]++;
        }
    }
}

bool ObjectiveDone(const Objective *o, const Map *m)
{
    for (int i = 0; i < COLLECT_COUNT; i++)
        if (o->got[i] < o->total[i]) return false;
    return !m->hasFuseBox || m->powered;
}

// Looks for a tile of this kind within 'dist' pixels of pos. Returns true and its tile coordinates if there is one.
static bool FindTileNear(const Map *m, Vector2 pos, char kind, float dist, int *tx, int *ty)
{
    int cx = (int)floorf(pos.x / TILE), cy = (int)floorf(pos.y / TILE);
    for (int y = cy - 1; y <= cy + 1; y++) {
        for (int x = cx - 1; x <= cx + 1; x++) {
            if (x < 0 || y < 0 || x >= m->w || y >= m->h || m->tiles[y][x] != kind) continue;
            Vector2 c = { (x + 0.5f) * TILE, (y + 0.5f) * TILE };
            if (Vector2Distance(pos, c) < dist) { *tx = x; *ty = y; return true; }
        }
    }
    return false;
}

// Picks up the tile (it turns into floor). Returns true if there was one to pick up.
static bool TakeTile(Map *m, Vector2 pos, char kind)
{
    int x, y;
    if (!FindTileNear(m, pos, kind, KEY_PICKUP_DIST, &x, &y)) return false;
    m->tiles[y][x] = '.';
    return true;
}

bool PickUpCollectible(Objective *o, Map *m, Vector2 pos, Collect *which)
{
    for (int i = 0; i < COLLECT_COUNT; i++) {
        if (!TakeTile(m, pos, COLLECTIBLES[i].mapChar)) continue;
        o->got[i]++;
        *which = (Collect)i;
        return true;
    }
    return false;
}

ItemId PickUpItem(Inventory *inv, Map *m, Vector2 pos)
{
    int cx = (int)floorf(pos.x / TILE), cy = (int)floorf(pos.y / TILE);
    for (int y = cy - 1; y <= cy + 1; y++) {
        for (int x = cx - 1; x <= cx + 1; x++) {
            if (x < 0 || y < 0 || x >= m->w || y >= m->h) continue;
            ItemId id = ItemFromMapChar(m->tiles[y][x]);
            if (id == ITEM_NONE) continue;
            Vector2 c = { (x + 0.5f) * TILE, (y + 0.5f) * TILE };
            if (Vector2Distance(pos, c) >= KEY_PICKUP_DIST) continue;
            if (!InvAdd(inv, id)) continue;                                // no room: it stays on the floor
            m->tiles[y][x] = '.';
            return id;
        }
    }
    return ITEM_NONE;
}

ItemId ItemNear(const Map *m, Vector2 pos)
{
    int cx = (int)floorf(pos.x / TILE), cy = (int)floorf(pos.y / TILE);
    for (int y = cy - 1; y <= cy + 1; y++) {
        for (int x = cx - 1; x <= cx + 1; x++) {
            if (x < 0 || y < 0 || x >= m->w || y >= m->h) continue;
            ItemId id = ItemFromMapChar(m->tiles[y][x]);
            if (id != ITEM_NONE && Vector2Distance(pos, (Vector2){ (x + 0.5f) * TILE, (y + 0.5f) * TILE }) < KEY_PICKUP_DIST) return id;
        }
    }
    return ITEM_NONE;
}

// Notes are numbered in the order their 'n' tiles appear in the map: top to bottom, left to right.
static int NoteNumber(const Map *m, int tx, int ty)
{
    int n = 0;
    for (int y = 0; y < m->h; y++) {
        for (int x = 0; x < m->w; x++) {
            if (m->tiles[y][x] != 'n') continue;
            if (x == tx && y == ty) return n;
            n++;
        }
    }
    return -1;
}

int NoteNear(const Map *m, Vector2 pos)
{
    int cx = (int)floorf(pos.x / TILE), cy = (int)floorf(pos.y / TILE);
    float best = ITEM_REACH;
    int bx = -1, by = -1;
    for (int y = cy - 2; y <= cy + 2; y++) {
        for (int x = cx - 2; x <= cx + 2; x++) {
            if (x < 0 || y < 0 || x >= m->w || y >= m->h || m->tiles[y][x] != 'n') continue;
            float d = Vector2Distance(pos, (Vector2){ (x + 0.5f) * TILE, (y + 0.5f) * TILE });
            if (d < best) { best = d; bx = x; by = y; }
        }
    }
    return (bx < 0) ? -1 : NoteNumber(m, bx, by);
}

bool ExitNear(const Map *m, Vector2 pos)
{
    int cx = (int)floorf(pos.x / TILE), cy = (int)floorf(pos.y / TILE);
    for (int y = cy - 2; y <= cy + 2; y++) {
        for (int x = cx - 2; x <= cx + 2; x++) {
            if (x < 0 || y < 0 || x >= m->w || y >= m->h || m->tiles[y][x] != 'X') continue;
            if (Vector2Distance(pos, (Vector2){ (x + 0.5f) * TILE, (y + 0.5f) * TILE }) < ITEM_REACH) return true;
        }
    }
    return false;
}

bool FuseBoxNear(const Map *m, Vector2 pos)
{
    int cx = (int)floorf(pos.x / TILE), cy = (int)floorf(pos.y / TILE);
    for (int y = cy - 2; y <= cy + 2; y++) {
        for (int x = cx - 2; x <= cx + 2; x++) {
            if (x < 0 || y < 0 || x >= m->w || y >= m->h || m->tiles[y][x] != 'G') continue;
            if (Vector2Distance(pos, (Vector2){ (x + 0.5f) * TILE, (y + 0.5f) * TILE }) < ITEM_REACH) return true;
        }
    }
    return false;
}

// Takes the middle one of the 'X' tiles and looks for open floor next to it.
bool ExitLightPos(const Map *m, Vector2 *out)
{
    int count = 0;
    for (int y = 0; y < m->h; y++)
        for (int x = 0; x < m->w; x++)
            if (m->tiles[y][x] == 'X') count++;
    if (count == 0) return false;

    int want = count / 2, seen = 0;
    for (int y = 0; y < m->h; y++) {
        for (int x = 0; x < m->w; x++) {
            if (m->tiles[y][x] != 'X' || seen++ != want) continue;
            static const int dx[4] = { 0, 0, -1, 1 }, dy[4] = { 1, -1, 0, 0 };
            for (int d = 0; d < 4; d++) {
                if (IsSolid(m, x + dx[d], y + dy[d])) continue;
                *out = (Vector2){ (x + dx[d] + 0.5f) * TILE, (y + dy[d] + 0.5f) * TILE };
                return true;
            }
            return false;
        }
    }
    return false;
}

int LoadNotes(const char *path, Note *notes, int max)
{
    char *text = LoadFileText(path);
    if (!text) return 0;
    memset(notes, 0, sizeof(Note) * (size_t)max);

    int count = 0, lines = 0;                            // lines = lines read so far in the current note
    const char *p = text;
    while (*p && count < max) {
        const char *e = p;
        while (*e && *e != '\n') e++;
        int len = (int)(e - p);
        if (len > 0 && p[len - 1] == '\r') len--;

        if (len > 0 && p[0] == ';') {
            // comment: ignore
        } else if (len == 3 && strncmp(p, "---", 3) == 0) {
            if (lines > 0) count++;                      // '---' ends the note
            lines = 0;
        } else if (len == 0 && lines == 0) {
            // blank line before a title: skip
        } else {
            Note *n = &notes[count];
            if (lines == 0) {
                int t = len < (int)sizeof n->title - 1 ? len : (int)sizeof n->title - 1;
                memcpy(n->title, p, (size_t)t);
                n->title[t] = '\0';
            } else {
                size_t cur = strlen(n->body);
                size_t room = sizeof n->body - 1 - cur;
                if (cur > 0 && room > 0) { n->body[cur++] = '\n'; n->body[cur] = '\0'; room--; }
                size_t take = (size_t)len < room ? (size_t)len : room;
                memcpy(n->body + cur, p, take);
                n->body[cur + take] = '\0';
            }
            lines++;
        }
        p = (*e == '\n') ? e + 1 : e;
    }
    if (lines > 0 && count < max) count++;               // last note had no closing '---'
    UnloadFileText(text);
    return count;
}
