// stalker.h - the thing that hunts you: a small state machine.
//
//   PATROL ---hears you---> INVESTIGATE ---sees you---> CHASE ---loses you---> SEARCH ---gives up---> PATROL
//
#ifndef STALKER_H
#define STALKER_H

#include "map.h"
#include "path.h"
#include "player.h"

typedef enum { ST_PATROL, ST_INVESTIGATE, ST_CHASE, ST_SEARCH } StalkerState;

typedef struct {
    bool         active;        // false if the map has no 'S'
    Vector2      pos;           // centre, world pixels
    float        facing;        // direction it is looking, radians
    StalkerState state;
    TilePath     path;          // the route it is walking
    int          pathIdx;       // next tile of 'path' to walk to
    int          wp;            // patrol point it is heading to (-1 = none yet)
    Vector2      target;        // last place it saw or heard you
    float        timer;         // waiting / losing-track timer
    float        repath;        // seconds until it recalculates its route while chasing
    bool         sawPlayer;     // could it see you last frame?
    bool         knowsHiding;   // it watched you climb into a locker
    bool         prevHidden;    // were you hidden last frame?
} Stalker;

void        StalkerInit(Stalker *s, const Map *m);
// Returns true if the stalker caught the player this frame.
bool        StalkerUpdate(Stalker *s, const Map *m, const Player *p, float dt);
const char *StalkerStateName(const Stalker *s);
void        DrawStalker(const Stalker *s);

#endif
