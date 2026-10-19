// items.h - things you can find and use: keys, fuses, batteries, notes, the fuse box, the exit door.
#ifndef ITEMS_H
#define ITEMS_H

#include "map.h"

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
} Objective;

void ObjectiveInit(Objective *o, const Map *m);          // counts the keys and fuses in the map
bool ObjectiveDone(const Objective *o, const Map *m);    // may the player use the exit now? (all keys, all fuses, power on if there is a fuse box)
bool PickUpKey(Objective *o, Map *m, Vector2 pos);       // takes a key you are touching (it vanishes from the map)
bool PickUpFuse(Objective *o, Map *m, Vector2 pos);      // same for a fuse
// Takes a battery you are touching and charges *battery. Skipped (returns false) while the battery is still nearly full,
// so the spare stays on the map for later.
bool PickUpBattery(float *battery, Map *m, Vector2 pos);
bool BatteryNear(const Map *m, Vector2 pos);             // standing on a battery (even one you left behind)?
int  NoteNear(const Map *m, Vector2 pos);                // number of a note within reach, or -1
bool ExitNear(const Map *m, Vector2 pos);                // is the exit door within reach?
bool FuseBoxNear(const Map *m, Vector2 pos);             // is the fuse box within reach?
bool ExitLightPos(const Map *m, Vector2 *out);           // a floor spot in front of the exit doors, where their light shines from

// Reads notes from a text file: first line = title, then the text, '---' ends a note.
// Lines starting with ';' are comments. Returns how many notes were read.
int  LoadNotes(const char *path, Note *notes, int max);

#endif
