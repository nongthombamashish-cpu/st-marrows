// chapters.h - the list of chapters in Mode 1. To add a chapter: add a row here and drop its map and notes in assets/.
#ifndef CHAPTERS_H
#define CHAPTERS_H

#include <stdbool.h>

typedef struct {
    const char *name;           // shown on the title screen and the "complete" screen
    const char *map;            // the level file
    const char *notes;          // the text of its notes
    const char *exitUse;        // what E does at the exit ("E   <this>")
    float       stalkerSpeed;   // 1.0 = the speed from config.h
    bool        battery;        // does the flashlight run out in this chapter?
} Chapter;

#define CHAPTER_COUNT 2

static const Chapter CHAPTERS[CHAPTER_COUNT] = {
    { "Ward A",       "assets/maps/mode1_ch1.txt", "assets/notes/mode1_ch1.txt", "unlock the exit",    1.00f, false },
    { "Surgery Wing", "assets/maps/mode1_ch2.txt", "assets/notes/mode1_ch2.txt", "call the elevator",  1.12f, true  },
};

#endif
