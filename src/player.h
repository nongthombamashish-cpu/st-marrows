// player.h - the investigator: input, movement, noise, hiding in lockers.
#ifndef PLAYER_H
#define PLAYER_H

#include "map.h"
#include "inventory.h"

typedef struct {
    Vector2 pos;          // centre of the player, in world pixels (the locker's centre while hiding)
    float   half;         // half the size of the collision box
    bool    flashlight;
    float   battery;      // flashlight charge, 0..BATTERY_MAX
    bool    usesBattery;  // false = the flashlight never runs out (chapter 1)
    Inventory inv;        // what you carry (batteries, bottles ...)
    bool    moving, running;
    bool    hidden;       // inside a locker
    Vector2 exitPos;      // where you stand again when you climb out
} Player;

void  PlayerInit(Player *p, Vector2 start);
// Loads the spare battery in an inventory slot into the flashlight. False if that slot holds no battery, or the flashlight is already full.
bool  PlayerLoadBattery(Player *p, int slot);
void  DrawPlayer(const Player *p, Vector2 aim);          // a shadow and a hop for every step; nothing while you hide (call inside BeginMode2D)
void  PlayerUpdate(Player *p, const Map *m, float dt);   // reads the keyboard
float PlayerNoise(const Player *p);                      // how far away the player can be heard right now

bool  PlayerNearLocker(const Player *p, const Map *m, Vector2 *lockerCentre);   // a locker within reach?
bool  PlayerTryHide(Player *p, const Map *m);                                    // climb in (false if none near)
void  PlayerLeaveLocker(Player *p);

#endif
