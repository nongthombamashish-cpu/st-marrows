#include "map.h"
#include "tiles.h"
#include <math.h>
#include <string.h>

// Reads a text map. '#' wall, '.' floor, 'L' locker, 'P' player start,
// 'S' stalker start, '1'..'9' stalker patrol points (visited in order),
// 'K' key, 'n' note, 'X' locked exit door.
// Lines starting with ';' are comments. Short rows are padded with walls.
bool LoadMap(Map *m, const char *path)
{
    memset(m, 0, sizeof *m);
    char *text = LoadFileText(path);
    if (!text) return false;

    int row = 0, col = 0;
    bool comment = false;
    for (const char *c = text; *c && row < MAX_H; c++) {
        if (*c == '\r') continue;
        if (*c == '\n') {
            if (col > 0) { if (col > m->w) m->w = col; row++; }
            col = 0;
            comment = false;
            continue;
        }
        if (col == 0 && *c == ';') comment = true;
        if (comment || col >= MAX_W) continue;
        m->tiles[row][col++] = *c;
    }
    if (col > 0 && row < MAX_H) { if (col > m->w) m->w = col; row++; }   // last line had no newline
    m->h = row;
    UnloadFileText(text);
    if (m->w == 0 || m->h == 0) return false;

    m->playerStart = (Vector2){ TILE * 1.5f, TILE * 1.5f };
    for (int i = 0; i < MAX_WAYPOINTS; i++) m->waypoint[i] = (Vector2){ -1, -1 };

    for (int y = 0; y < m->h; y++) {
        for (int x = 0; x < m->w; x++) {
            char *t = &m->tiles[y][x];
            Vector2 centre = { (x + 0.5f) * TILE, (y + 0.5f) * TILE };
            if (*t == '\0') {
                *t = '#';                                            // pad short rows
            } else if (*t == 'P') {
                m->playerStart = centre;
                *t = '.';
            } else if (*t == 'S') {
                m->stalkerStart = centre;
                m->hasStalker = true;
                *t = '.';
            } else if (*t >= '1' && *t <= '9') {
                int i = *t - '1';
                m->waypoint[i] = centre;
                if (i + 1 > m->waypointCount) m->waypointCount = i + 1;
                *t = '.';
            }
        }
    }
    return true;
}

bool IsSolid(const Map *m, int tx, int ty)
{
    if (tx < 0 || ty < 0 || tx >= m->w || ty >= m->h) return true;      // outside = wall
    char t = m->tiles[ty][tx];
    return t == '#' || t == 'L' || t == 'X';
}

bool IsSolidAt(const Map *m, Vector2 p)
{
    return IsSolid(m, (int)floorf(p.x / TILE), (int)floorf(p.y / TILE));
}

// Does a square box (centre c, half-size) overlap any solid tile?
bool HitsWall(const Map *m, Vector2 c, float half)
{
    int x0 = (int)floorf((c.x - half) / TILE), x1 = (int)floorf((c.x + half) / TILE);
    int y0 = (int)floorf((c.y - half) / TILE), y1 = (int)floorf((c.y + half) / TILE);
    for (int y = y0; y <= y1; y++)
        for (int x = x0; x <= x1; x++)
            if (IsSolid(m, x, y)) return true;
    return false;
}

// Move on X and Y separately so the box slides along walls instead of sticking.
void MoveBox(const Map *m, Vector2 *pos, float half, Vector2 delta)
{
    Vector2 next = { pos->x + delta.x, pos->y };
    if (!HitsWall(m, next, half)) *pos = next;
    next = (Vector2){ pos->x, pos->y + delta.y };
    if (!HitsWall(m, next, half)) *pos = next;
}

// Walks along the line from a to b in small steps; any solid tile blocks it.
bool HasLineOfSight(const Map *m, Vector2 a, Vector2 b)
{
    float dx = b.x - a.x, dy = b.y - a.y;
    int steps = (int)(sqrtf(dx * dx + dy * dy) / 4.0f) + 1;             // check about every 4 pixels
    for (int i = 1; i < steps; i++) {
        float t = (float)i / (float)steps;
        if (IsSolidAt(m, (Vector2){ a.x + dx * t, a.y + dy * t })) return false;
    }
    return true;
}

void DrawMap(const Map *m, Camera2D cam, Vector2 eyes)
{
    // Only draw the tiles the camera can see.
    Vector2 tl = GetScreenToWorld2D((Vector2){ 0, 0 }, cam);
    Vector2 br = GetScreenToWorld2D((Vector2){ VIRT_W, VIRT_H }, cam);
    int x0 = (int)fmaxf(0, floorf(tl.x / TILE)), y0 = (int)fmaxf(0, floorf(tl.y / TILE));
    int x1 = (int)fminf((float)(m->w - 1), floorf(br.x / TILE));
    int y1 = (int)fminf((float)(m->h - 1), floorf(br.y / TILE));

    for (int y = y0; y <= y1; y++) {
        for (int x = x0; x <= x1; x++) {
            float px = (float)(x * TILE), py = (float)(y * TILE);
            Rectangle r = { px, py, TILE, TILE };
            Color floorCol = ((x + y) & 1) ? (Color){ 86, 92, 106, 255 } : (Color){ 80, 86, 100, 255 };
            Vector2 centre = { px + 8.0f, py + 8.0f };

            // The floor goes under everything (sprites can have see-through corners).
            if (!TileDraw(((x + y) & 1) ? T_FLOOR2 : T_FLOOR1, px, py, false)) DrawRectangleRec(r, floorCol);

            switch (m->tiles[y][x]) {
                case '#':
                    if (!TileDraw(T_WALL, px, py, false)) {
                        DrawRectangleRec(r, (Color){ 140, 146, 160, 255 });
                        DrawRectangleLinesEx(r, 1, (Color){ 100, 106, 120, 255 });
                    }
                    break;
                case 'L':
                    if (!TileDraw(T_LOCKER, px, py, false))
                        DrawRectangle(x * TILE + 2, y * TILE + 1, TILE - 4, TILE - 2, (Color){ 100, 150, 140, 255 });
                    break;
                case 'X':                                            // locked exit door
                    if (!TileDraw(T_EXIT, px, py, false)) {
                        DrawRectangleRec(r, (Color){ 120, 84, 52, 255 });
                        DrawRectangleLinesEx(r, 1, (Color){ 70, 48, 30, 255 });
                        DrawRectangle(x * TILE + 6, y * TILE + 5, 4, 6, (Color){ 230, 190, 70, 255 });
                    }
                    break;
                case 'K':                                            // key (only if you can see it)
                    if (!HasLineOfSight(m, eyes, centre)) break;
                    if (!TileDraw(T_KEY, px, py, false)) {
                        DrawCircle(x * TILE + 8, y * TILE + 5, 3, (Color){ 240, 200, 70, 255 });
                        DrawRectangle(x * TILE + 7, y * TILE + 7, 2, 7, (Color){ 240, 200, 70, 255 });
                        DrawRectangle(x * TILE + 9, y * TILE + 11, 3, 1, (Color){ 240, 200, 70, 255 });
                        DrawRectangle(x * TILE + 9, y * TILE + 13, 2, 1, (Color){ 240, 200, 70, 255 });
                    }
                    break;
                case 'n':                                            // note (only if you can see it)
                    if (!HasLineOfSight(m, eyes, centre)) break;
                    if (!TileDraw(T_NOTE, px, py, false)) {
                        DrawRectangle(x * TILE + 4, y * TILE + 3, 8, 10, (Color){ 232, 226, 200, 255 });
                        DrawRectangle(x * TILE + 5, y * TILE + 5, 6, 1, (Color){ 120, 112, 96, 255 });
                        DrawRectangle(x * TILE + 5, y * TILE + 7, 6, 1, (Color){ 120, 112, 96, 255 });
                        DrawRectangle(x * TILE + 5, y * TILE + 9, 4, 1, (Color){ 120, 112, 96, 255 });
                    }
                    break;
                default:
                    break;
            }
        }
    }
}