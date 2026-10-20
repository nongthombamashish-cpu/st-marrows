// cameras.h - security cameras. They hang from the ceiling, sweep a cone of view left and right, and see in the dark.
// If one keeps you in view for CAM_DETECT_TIME seconds, an alarm rings and the stalker goes to where you were.
// You can hide in a locker (they cannot see in), or break a camera by shattering a bottle next to it.
#ifndef CAMERAS_H
#define CAMERAS_H

#include <stdbool.h>
#include "raylib.h"
#include "map.h"
#include "light.h"

typedef struct {
    Vector2 pos;         // where it hangs (the middle of its tile)
    float   faceDeg;     // the middle of its sweep (0 = east, 90 = south)
    float   phase;       // seconds, so the cameras do not all swing together
    float   angleDeg;    // where it looks right now
    bool    broken;
    float   seen;        // 0..1: how close it is to ringing the alarm (fills while you are in view)
} Cam;

typedef struct {
    Cam   cam[MAX_CAMS];
    int   count;
    float time;          // drives the sweeps and the blinking lights
    float alarm;         // seconds of alarm left; while it runs no camera can ring again
} Cams;

void  CamsInit(Cams *c, const Map *m);
// Moves the cameras and checks who they see. Returns true on the frame an alarm rings, and puts the spot you were seen at in *where.
bool  CamsUpdate(Cams *c, const Map *m, Vector2 player, bool playerHidden, float dt, Vector2 *where);
bool  CamsBreakNear(Cams *c, Vector2 at);      // a bottle shattered at 'at': breaks a camera close to it. True if one broke.
float CamsSeen(const Cams *c);                 // 0..1: how close the closest camera is to ringing (for the red screen edge)
void  CamsDraw(const Cams *c);                 // the camera heads: call it inside BeginMode2D
// Lights each working camera's cone (so you can see where it looks). Only cameras near 'around' (the player) get a light.
void  CamsLights(const Cams *c, const LightKit *k, RenderTexture2D mask, const Map *m, Camera2D cam, Vector2 around);

#endif
