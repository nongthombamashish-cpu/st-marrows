// light.h - lights and shadows. Solid tiles block light.
#ifndef LIGHT_H
#define LIGHT_H

#include "raylib.h"
#include "map.h"

typedef struct {
    Texture2D       beam;      // cone sprite (points right, +X)
    Texture2D       glow;      // round sprite
    RenderTexture2D layer;     // scratch picture, used for one light at a time
} LightKit;

void LightKitInit(LightKit *k);
void LightKitUnload(LightKit *k);

// Fill the darkness mask with the ambient darkness. Call once per frame, before AddLight.
void ClearMask(RenderTexture2D mask, Color ambient);

// Add one light to the mask. Walls and lockers block it.
//   sprite    k->glow (round) or k->beam (cone)
//   range     how far it reaches, in pixels
//   angleDeg  which way a cone points
// Call it outside BeginTextureMode: it draws into textures by itself.
void AddLight(const LightKit *k, RenderTexture2D mask, const Map *m, Camera2D cam,
              Texture2D sprite, Vector2 pos, float range, float angleDeg);

#endif
