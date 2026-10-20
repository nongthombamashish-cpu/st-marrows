#include "cameras.h"
#include "raymath.h"
#include <math.h>
#include <string.h>

// A triangle wave: from -1 up to +1 and back down at a steady speed. One full cycle per 1.0 of u.
static float Triangle(float u)
{
    float f = u - floorf(u);
    return (f < 0.5f) ? (4.0f * f - 1.0f) : (3.0f - 4.0f * f);
}

// The smallest difference between two angles in degrees, -180..180.
static float AngleDiff(float a, float b)
{
    float d = fmodf(a - b + 180.0f, 360.0f);
    if (d < 0.0f) d += 360.0f;
    return d - 180.0f;
}

void CamsInit(Cams *c, const Map *m)
{
    memset(c, 0, sizeof *c);
    c->count = m->camCount;
    for (int i = 0; i < c->count; i++) {
        c->cam[i].pos = m->camPos[i];
        c->cam[i].faceDeg = m->camFace[i];
        c->cam[i].angleDeg = m->camFace[i];
        c->cam[i].phase = fmodf(i * 1.7f, CAM_SWEEP_PERIOD);     // spread the cameras out
    }
}

// Is 'p' inside this camera's cone, close enough, and not behind a wall?
static bool InView(const Cam *k, const Map *m, Vector2 p)
{
    Vector2 d = Vector2Subtract(p, k->pos);
    float dist = Vector2Length(d);
    if (dist > CAM_RANGE) return false;
    if (dist < 1.0f) return true;                                 // right under it
    float angle = atan2f(d.y, d.x) * RAD2DEG;
    if (fabsf(AngleDiff(angle, k->angleDeg)) > CAM_FOV) return false;
    return HasLineOfSight(m, k->pos, p);
}

bool CamsUpdate(Cams *c, const Map *m, Vector2 player, bool playerHidden, float dt, Vector2 *where)
{
    c->time += dt;
    if (c->alarm > 0.0f) c->alarm = fmaxf(0.0f, c->alarm - dt);

    bool rang = false;
    for (int i = 0; i < c->count; i++) {
        Cam *k = &c->cam[i];
        if (k->broken) continue;
        k->angleDeg = k->faceDeg + CAM_SWEEP * Triangle((c->time + k->phase) / CAM_SWEEP_PERIOD);

        bool inView = !playerHidden && InView(k, m, player);
        if (inView && c->alarm <= 0.0f) k->seen += dt / CAM_DETECT_TIME;     // being watched fills the bar...
        else                            k->seen -= dt * CAM_DECAY;           // ...and it empties when you are out of view
        k->seen = Clamp(k->seen, 0.0f, 1.0f);

        if (k->seen >= 1.0f && !rang) { rang = true; *where = player; }
    }
    if (rang) {
        c->alarm = CAM_ALARM_TIME;
        for (int i = 0; i < c->count; i++) c->cam[i].seen = 0.0f;
    }
    return rang;
}

bool CamsBreakNear(Cams *c, Vector2 at)
{
    bool broke = false;
    for (int i = 0; i < c->count; i++) {
        Cam *k = &c->cam[i];
        if (k->broken || Vector2Distance(k->pos, at) >= CAM_BREAK_DIST) continue;
        k->broken = true;
        k->seen = 0.0f;
        broke = true;
    }
    return broke;
}

float CamsSeen(const Cams *c)
{
    float most = 0.0f;
    for (int i = 0; i < c->count; i++) if (c->cam[i].seen > most) most = c->cam[i].seen;
    return most;
}

void CamsDraw(const Cams *c)
{
    for (int i = 0; i < c->count; i++) {
        const Cam *k = &c->cam[i];
        DrawCircleV(k->pos, 3.5f, (Color){ 40, 42, 50, 255 });                       // the ceiling mount
        if (k->broken) {
            DrawCircleV(k->pos, 2.0f, (Color){ 96, 96, 104, 255 });                  // a dead grey stump
            continue;
        }
        Vector2 dir = { cosf(k->angleDeg * DEG2RAD), sinf(k->angleDeg * DEG2RAD) };
        Vector2 lens = Vector2Add(k->pos, Vector2Scale(dir, 7.0f));
        DrawLineEx(k->pos, lens, 3.0f, (Color){ 74, 78, 90, 255 });                  // the camera body, pointing where it looks
        bool lit = (c->alarm > 0.0f) ? fmodf(c->time, 0.4f) < 0.2f : fmodf(c->time, 1.0f) < 0.5f;   // red light: blinks, fast in an alarm
        if (lit) DrawCircleV(lens, 1.6f, (Color){ 255, 50, 40, 255 });
    }
}

void CamsLights(const Cams *c, const LightKit *k, RenderTexture2D mask, const Map *m, Camera2D cam, Vector2 around)
{
    for (int i = 0; i < c->count; i++) {
        const Cam *s = &c->cam[i];
        if (s->broken || Vector2Distance(s->pos, around) > 380.0f) continue;         // only the ones on screen
        AddLight(k, mask, m, cam, k->beam, s->pos, CAM_RANGE, s->angleDeg);          // its cone of view is lit: you see where it looks
        AddLight(k, mask, m, cam, k->glow, s->pos, 12.0f, 0);                        // and a faint glow so the camera itself shows
    }
}
