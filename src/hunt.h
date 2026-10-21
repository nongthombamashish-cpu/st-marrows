// hunt.h - the hunt (Chapter 5). When you take the last thing, the building turns on you: a countdown starts, and every
// HUNT_PING seconds the stalker is told where you are. Reach the exit before the clock runs out.
#ifndef HUNT_H
#define HUNT_H

#include <stdbool.h>
#include "config.h"

typedef struct {
    bool  active;
    float left;       // seconds left on the clock
    float ping;       // seconds since the stalker was last told where you are
} Hunt;

typedef enum { HUNT_QUIET, HUNT_PING_NOW, HUNT_TIMEOUT } HuntEvent;

void      HuntInit(Hunt *h);
void      HuntStart(Hunt *h, float seconds);
// Call every frame. HUNT_PING_NOW: tell the stalker where the player is. HUNT_TIMEOUT: the time is up (the hunt ends).
HuntEvent HuntUpdate(Hunt *h, float dt);

#endif
