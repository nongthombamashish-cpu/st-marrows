#include "light.h"
#include "raymath.h"
#include <math.h>

// Builds a soft light sprite in code (no image file needed).
// halfAngle = PI gives a round glow; smaller values give a cone pointing right (+X).
static Texture2D MakeLight(int size, float halfAngle)
{
    Image img = GenImageColor(size, size, BLACK);
    Color *px = (Color *)img.data;
    float c = size * 0.5f;

    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            float dx = x + 0.5f - c, dy = y + 0.5f - c;
            float dist  = sqrtf(dx * dx + dy * dy) / c;      // 0 at the centre .. 1 at the edge
            float angle = fabsf(atan2f(dy, dx));             // 0 = straight ahead
            float v = Clamp(1.0f - dist, 0.0f, 1.0f);
            v *= v;                                          // soft falloff
            if (halfAngle < PI)                              // fade out toward the cone edges
                v *= Clamp((halfAngle - angle) / (halfAngle * 0.4f), 0.0f, 1.0f);
            unsigned char b = (unsigned char)(255.0f * v);
            px[y * size + x] = (Color){ b, b, b, 255 };
        }
    }
    Texture2D tex = LoadTextureFromImage(img);
    UnloadImage(img);
    SetTextureFilter(tex, TEXTURE_FILTER_BILINEAR);
    return tex;
}

void LightKitInit(LightKit *k)
{
    k->beam  = MakeLight(256, BEAM_HALF);
    k->glow  = MakeLight(128, PI);
    k->layer = LoadRenderTexture(VIRT_W, VIRT_H);
}

void LightKitUnload(LightKit *k)
{
    UnloadTexture(k->beam);
    UnloadTexture(k->glow);
    UnloadRenderTexture(k->layer);
}

void ClearMask(RenderTexture2D mask, Color ambient)
{
    BeginTextureMode(mask);
        ClearBackground(ambient);
    EndTextureMode();
}

// raylib only draws triangles whose points run counter-clockwise on screen, so flip the order if needed.
static void DrawTriangleAnyWay(Vector2 a, Vector2 b, Vector2 c, Color col)
{
    float cross = (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
    if (cross > 0) DrawTriangle(a, c, b, col);
    else           DrawTriangle(a, b, c, col);
}

// The shadow behind one wall edge (a-b): the edge pushed away from the light by 'reach' pixels.
static void CastEdge(Vector2 light, Vector2 a, Vector2 b, float reach)
{
    Vector2 a2 = Vector2Add(a, Vector2Scale(Vector2Normalize(Vector2Subtract(a, light)), reach));
    Vector2 b2 = Vector2Add(b, Vector2Scale(Vector2Normalize(Vector2Subtract(b, light)), reach));
    DrawTriangleAnyWay(a, b, b2, BLACK);
    DrawTriangleAnyWay(a, b2, a2, BLACK);
}

void AddLight(const LightKit *k, RenderTexture2D mask, const Map *m, Camera2D cam,
              Texture2D sprite, Vector2 pos, float range, float angleDeg)
{
    float reach = range * 2.0f + 32.0f;

    // 1) Draw this light alone on a black picture: the sprite, then black shadows on top of it.
    BeginTextureMode(k->layer);
        ClearBackground(BLACK);
        BeginMode2D(cam);
            BeginBlendMode(BLEND_ADDITIVE);
                DrawTexturePro(sprite, (Rectangle){ 0, 0, (float)sprite.width, (float)sprite.height },
                               (Rectangle){ pos.x, pos.y, range * 2, range * 2 },
                               (Vector2){ range, range }, angleDeg, WHITE);
            EndBlendMode();

            int x0 = (int)floorf((pos.x - range) / TILE), x1 = (int)floorf((pos.x + range) / TILE);
            int y0 = (int)floorf((pos.y - range) / TILE), y1 = (int)floorf((pos.y + range) / TILE);
            if (x0 < 0) x0 = 0;
            if (y0 < 0) y0 = 0;
            if (x1 > m->w - 1) x1 = m->w - 1;
            if (y1 > m->h - 1) y1 = m->h - 1;

            for (int y = y0; y <= y1; y++) {
                for (int x = x0; x <= x1; x++) {
                    if (!IsSolid(m, x, y)) continue;
                    float left = (float)(x * TILE), right = left + TILE;
                    float top  = (float)(y * TILE), bottom = top + TILE;
                    // Only edges that touch open floor AND face away from the light cast a shadow.
                    // (The wall's own face stays lit; everything behind it goes dark.)
                    if (!IsSolid(m, x, y - 1) && pos.y >= top)    CastEdge(pos, (Vector2){ left, top },    (Vector2){ right, top },    reach);
                    if (!IsSolid(m, x, y + 1) && pos.y <= bottom) CastEdge(pos, (Vector2){ left, bottom }, (Vector2){ right, bottom }, reach);
                    if (!IsSolid(m, x - 1, y) && pos.x >= left)   CastEdge(pos, (Vector2){ left, top },    (Vector2){ left, bottom },  reach);
                    if (!IsSolid(m, x + 1, y) && pos.x <= right)  CastEdge(pos, (Vector2){ right, top },   (Vector2){ right, bottom }, reach);
                }
            }
        EndMode2D();
    EndTextureMode();

    // 2) Add that picture to the darkness mask.
    BeginTextureMode(mask);
        BeginBlendMode(BLEND_ADDITIVE);
            DrawTextureRec(k->layer.texture, (Rectangle){ 0, 0, VIRT_W, -VIRT_H }, (Vector2){ 0, 0 }, WHITE);
        EndBlendMode();
    EndTextureMode();
}
