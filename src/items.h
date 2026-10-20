// items.h - things you can find and use: keys, fuses, patient files, inventory items, notes, the fuse box, the exit door.
#ifndef ITEMS_H
#define ITEMS_H

#include "map.h"
#include "inventory.h"

#define MAX_NOTES 16

typedef struct {
    char title[64];
    char body[600];
} Note;

typedef struct {
    int keys;            // keys collected so far
    int keysTotal;       // keys in this map ('K' tiles)
    int fuses;           // fuses collected so far
    int fusesTotal;      // fuses in this map ('F' tiles)
    int files;           // patient files collected so far
    int filesTotal;      // patient files in this map ('R' tiles)
} Objective;

void ObjectiveInit(Objective *o, const Map *m);          // counts the keys and fuses in the map
bool ObjectiveDone(const Objective *o, const Map *m);    // may the player use the exit now? (all keys, all fuses, power on if there is a fuse box)
bool PickUpKey(Objective *o, Map *m, Vector2 pos);       // takes a key you are touching (it vanishes from the map)
bool PickUpFuse(Objective *o, Map *m, Vector2 pos);      // same for a fuse
bool PickUpFile(Objective *o, Map *m, Vector2 pos);      // same for a patient file
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
