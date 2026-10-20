// throw.h - throwing bottles. A bottle flies to where you aim, shatters there, and the noise brings the stalker.
#ifndef THROW_H
#define THROW_H

#include <stdbool.h>
#include "raylib.h"
#include "map.h"

typedef struct {
    bool    flying;
    Vector2 from, to;     // where it left your hand, and where it will land
    float   t;            // 0..1 along the flight
    float   shatter;      // seconds since it broke (0 = nothing to show)
    Vector2 at;           // where it broke
} Throw;

void    ThrowInit(Throw *th);
// Where a bottle thrown from 'from' toward 'aim' (both world positions) would land: at most THROW_RANGE away, and it stops before a wall.
Vector2 ThrowLanding(const Map *m, Vector2 from, Vector2 aim);
// Starts a throw. False if one is already in the air, or there is no room (a wall right in front of you).
bool    ThrowStart(Throw *th, const Map *m, Vector2 from, Vector2 aim);
// Moves the bottle. Returns true on the frame it shatters (and puts the spot in *landed).
bool    ThrowUpdate(Throw *th, float dt, Vector2 *landed);
void    ThrowDraw(const Throw *th);       // call this inside BeginMode2D

#endif
