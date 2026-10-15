// player.h - the investigator: input, movement, noise, hiding in lockers.
#ifndef PLAYER_H
#define PLAYER_H

#include "map.h"

typedef struct {
    Vector2 pos;          // centre of the player, in world pixels (the locker's centre while hiding)
    float   half;         // half the size of the collision box
    bool    flashlight;
    bool    moving, running;
    bool    hidden;       // inside a locker
    Vector2 exitPos;      // where you stand again when you climb out
} Player;

void  PlayerInit(Player *p, Vector2 start);
void  PlayerUpdate(Player *p, const Map *m, float dt);   // reads the keyboard
float PlayerNoise(const Player *p);                      // how far away the player can be heard right now

bool  PlayerNearLocker(const Player *p, const Map *m, Vector2 *lockerCentre);   // a locker within reach?
bool  PlayerTryHide(Player *p, const Map *m);                                    // climb in (false if none near)
void  PlayerLeaveLocker(Player *p);

#endif
