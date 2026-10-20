// chapters.h - the list of chapters in Mode 1. To add a chapter: add a row here and drop its map and notes in assets/.
#ifndef CHAPTERS_H
#define CHAPTERS_H

#include <stdbool.h>

typedef struct {
    const char *name;           // shown on the title screen and the "complete" screen
    const char *map;            // the level file
    const char *notes;          // the text of its notes
    const char *exitUse;        // what E does at the exit ("E   <this>")
    const char *allDone;        // the goal line once everything is collected (when there is no fuse box to fill)
    const char *locked;         // what the exit says while it is shut ("The exit is locked")
    float       stalkerSpeed;   // 1.0 = the speed from config.h
    bool        battery;        // does the flashlight run out in this chapter?
    bool        inventory;      // show the inventory (hotbar) in this chapter?
} Chapter;

#define CHAPTER_COUNT 4

static const Chapter CHAPTERS[CHAPTER_COUNT] = {
    { "Ward A",           "assets/maps/mode1_ch1.txt", "assets/notes/mode1_ch1.txt", "unlock the exit",    "All keys found. Reach the exit at the east end of the corridor.", "The exit is locked", 1.00f, false, false },
    { "Surgery Wing",     "assets/maps/mode1_ch2.txt", "assets/notes/mode1_ch2.txt", "call the elevator",  "",                                                              "The elevator has no power", 1.12f, true,  true  },
    { "Records Basement", "assets/maps/mode1_ch3.txt", "assets/notes/mode1_ch3.txt", "open the vault",     "All files found. Open the archive vault on the east side.",       "The vault is sealed", 1.20f, true,  true  },
    { "Security Wing",    "assets/maps/mode1_ch4.txt", "assets/notes/mode1_ch4.txt", "open the control room", "All access cards found. Get to the control room door at the top.", "The control room door is locked", 1.22f, true,  true  },
};

#endif
