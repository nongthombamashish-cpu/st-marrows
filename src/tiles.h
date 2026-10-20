// tiles.h - draws sprites from one tileset picture, with plain colored shapes as the fallback.
#ifndef TILES_H
#define TILES_H

#include "raylib.h"

typedef enum { T_FLOOR1, T_FLOOR2, T_WALL, T_LOCKER, T_KEY, T_NOTE, T_EXIT, T_PLAYER, T_STALKER, T_BATTERY, T_FUSE, T_FUSEBOX, T_TABLE, T_BOTTLE, T_FILE, T_SHELF, T_COUNT } TileId;

bool TilesLoad(void);      // reads assets/tiles/tiles.txt and the sheet it names (call again to reload)
void TilesUnload(void);

// Draws tile 'id' with its top-left corner at (x, y), TILE pixels wide. Returns false if that tile is not
// set up, so the caller can draw its colored shape instead.
bool TileDraw(TileId id, float x, float y, bool flipX);

// Same, but you choose the size and tint (used for the big ghost on the title screen).
bool TileDrawEx(TileId id, Rectangle dst, Color tint, bool flipX);

// A screen showing every tile of the sheet with its number. Call once per frame while it is open.
void TilesBrowser(void);

#endif