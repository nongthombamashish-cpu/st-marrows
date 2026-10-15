// items.h - things you can find and use: keys, notes, the exit door.
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
} Objective;

void ObjectiveInit(Objective *o, const Map *m);          // counts the keys in the map
bool PickUpKey(Objective *o, Map *m, Vector2 pos);       // takes a key you are touching (it vanishes from the map)
int  NoteNear(const Map *m, Vector2 pos);                // number of a note within reach, or -1
bool ExitNear(const Map *m, Vector2 pos);                // is the exit door within reach?

// Reads notes from a text file: first line = title, then the text, '---' ends a note.
// Lines starting with ';' are comments. Returns how many notes were read.
int  LoadNotes(const char *path, Note *notes, int max);

#endif
