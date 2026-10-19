// map.h - the tile map: loading, collision, line of sight, drawing.
#ifndef MAP_H
#define MAP_H

#include "raylib.h"
#include "config.h"

#define MAX_W 128            // biggest map we support, in tiles
#define MAX_H 128
#define MAX_WAYPOINTS 9      // patrol points '1'..'9' in the map file

typedef struct {
    char    tiles[MAX_H][MAX_W + 1];     // one text row per map row
    int     w, h;                        // size in tiles
    Vector2 playerStart;                 // 'P'
    Vector2 stalkerStart;                // 'S'
    bool    hasStalker;
    Vector2 waypoint[MAX_WAYPOINTS];     // '1'..'9' (x < 0 = not placed)
    int     waypointCount;               // highest waypoint number found
    bool    hasFuseBox;                  // the map has a 'G' (the exit needs power)
    bool    powered;                     // the fuse box has been filled: the elevator works
} Map;

bool LoadMap(Map *m, const char *path);
bool IsSolid(const Map *m, int tx, int ty);                         // tile coordinates
bool IsSolidAt(const Map *m, Vector2 p);                            // pixel position
bool HitsWall(const Map *m, Vector2 centre, float half);            // square box vs. walls
void MoveBox(const Map *m, Vector2 *pos, float half, Vector2 delta);// move and slide along walls
bool HasLineOfSight(const Map *m, Vector2 a, Vector2 b);            // nothing solid in between?
void DrawMap(const Map *m, Camera2D cam, Vector2 eyes);          // keys and notes show only if 'eyes' can see them

#endif
