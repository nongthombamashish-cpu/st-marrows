// items.h - things you can find and use: collectibles (keys, fuses, files, cards), inventory items, notes, the fuse box, the exit door.
#ifndef ITEMS_H
#define ITEMS_H

#include "map.h"
#include "inventory.h"

#define MAX_NOTES 16

typedef struct {
    char title[64];
    char body[600];
} Note;

// ---- Things you must collect to open the exit: keys, fuses, patient files, access cards ----
// One row per kind in COLLECTIBLES (items.c). To add a kind: add a name here, a row there, and put its map character on a map.
typedef enum { COLLECT_KEY, COLLECT_FUSE, COLLECT_FILE, COLLECT_CARD, COLLECT_COUNT } Collect;

typedef struct {
    char        mapChar;     // the character on a map
    const char *name;        // "Key": the message when you pick one up ("Key found  (1/3)")
    const char *plural;      // "keys": the goal line ("Find the keys: 1/3")
    TileId      tile;        // its picture
    Color       color;       // the plain shape's colour, if the picture is not set up
} CollectInfo;

const CollectInfo *CollectGet(Collect c);
Collect CollectFromMapChar(char c);                      // COLLECT_COUNT if the character is not a collectible
void    CollectDraw(Collect c, float x, float y);        // draws one lying on the floor (the TILE x TILE spot at x, y)

typedef struct {
    int got[COLLECT_COUNT];      // collected so far, per kind
    int total[COLLECT_COUNT];    // how many the map has, per kind
} Objective;

void ObjectiveInit(Objective *o, const Map *m);          // counts the collectibles in the map
bool ObjectiveDone(const Objective *o, const Map *m);    // may the player use the exit now? (everything collected, power on if there is a fuse box)
// Takes a collectible you are touching (it vanishes from the map). Returns true and says which kind in *which.
bool PickUpCollectible(Objective *o, Map *m, Vector2 pos, Collect *which);
// Picks up an item lying where you stand (any item from the table in inventory.c). Returns which one,
// or ITEM_NONE if there is nothing there, or your inventory has no room (then it stays on the floor).
ItemId PickUpItem(Inventory *inv, Map *m, Vector2 pos);
ItemId ItemNear(const Map *m, Vector2 pos);              // an item within reach, even one you could not pick up (inventory full)
int  NoteNear(const Map *m, Vector2 pos);                // number of a note within reach, or -1
bool ExitNear(const Map *m, Vector2 pos);                // is the exit door within reach?
bool FuseBoxNear(const Map *m, Vector2 pos);             // is the fuse box within reach?
bool ExitLightPos(const Map *m, Vector2 *out);           // a floor spot in front of the exit doors, where their light shines from

// Reads notes from a text file: first line = title, then the text, '---' ends a note.
// Lines starting with ';' are comments. Returns how many notes were read.
int  LoadNotes(const char *path, Note *notes, int max);

#endif
