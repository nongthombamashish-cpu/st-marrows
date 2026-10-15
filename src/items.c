#include "items.h"
#include "raymath.h"
#include <math.h>
#include <string.h>

void ObjectiveInit(Objective *o, const Map *m)
{
    o->keys = 0;
    o->keysTotal = 0;
    for (int y = 0; y < m->h; y++)
        for (int x = 0; x < m->w; x++)
            if (m->tiles[y][x] == 'K') o->keysTotal++;
}

bool PickUpKey(Objective *o, Map *m, Vector2 pos)
{
    int cx = (int)floorf(pos.x / TILE), cy = (int)floorf(pos.y / TILE);
    for (int y = cy - 1; y <= cy + 1; y++) {
        for (int x = cx - 1; x <= cx + 1; x++) {
            if (x < 0 || y < 0 || x >= m->w || y >= m->h || m->tiles[y][x] != 'K') continue;
            Vector2 c = { (x + 0.5f) * TILE, (y + 0.5f) * TILE };
            if (Vector2Distance(pos, c) < KEY_PICKUP_DIST) {
                m->tiles[y][x] = '.';
                o->keys++;
                return true;
            }
        }
    }
    return false;
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
