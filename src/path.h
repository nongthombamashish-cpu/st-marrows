// path.h - finds a walkable route across the tile grid (breadth-first search).
#ifndef PATH_H
#define PATH_H

#include "map.h"

#define PATH_MAX_LEN 1024

typedef struct {
    int x[PATH_MAX_LEN];     // tile coordinates, start first, goal last
    int y[PATH_MAX_LEN];
    int len;
} TilePath;

// Shortest 4-direction route from tile (sx,sy) to (gx,gy). Returns false (len = 0) if there is none.
bool FindPath(const Map *m, int sx, int sy, int gx, int gy, TilePath *out);

#endif
