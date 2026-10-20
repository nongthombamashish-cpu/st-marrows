#include "throw.h"
#include "raymath.h"
#include "tiles.h"
#include <math.h>
#include <string.h>

void ThrowInit(Throw *th)
{
    memset(th, 0, sizeof *th);
}

Vector2 ThrowLanding(const Map *m, Vector2 from, Vector2 aim)
{
    Vector2 d = Vector2Subtract(aim, from);
    float len = Vector2Length(d);
    if (len < 1.0f) return from;
    if (len > THROW_RANGE) len = THROW_RANGE;
    Vector2 dir = Vector2Normalize(d);

    Vector2 last = from;                                          // walk along the line; stop before the first wall
    for (float s = 4.0f; s <= len; s += 4.0f) {
        Vector2 p = Vector2Add(from, Vector2Scale(dir, s));
        if (IsSolid(m, (int)floorf(p.x / TILE), (int)floorf(p.y / TILE))) break;
        last = p;
    }
    return last;
}

bool ThrowStart(Throw *th, const Map *m, Vector2 from, Vector2 aim)
{
    if (th->flying) return false;
    Vector2 to = ThrowLanding(m, from, aim);
    if (Vector2Distance(from, to) < 20.0f) return false;          // a wall right in front of you
    th->flying = true;
    th->from = from;
    th->to = to;
    th->t = 0.0f;
    return true;
}

bool ThrowUpdate(Throw *th, float dt, Vector2 *landed)
{
    if (th->shatter > 0.0f) {                                      // the little burst of glass fades
        th->shatter += dt;
        if (th->shatter > 0.5f) th->shatter = 0.0f;
    }
    if (!th->flying) return false;

    th->t += THROW_SPEED * dt / Vector2Distance(th->from, th->to);
    if (th->t < 1.0f) return false;

    th->flying = false;
    th->shatter = 0.0001f;
    th->at = th->to;
    *landed = th->to;
    return true;
}

void ThrowDraw(const Throw *th)
{
    if (th->flying) {
        Vector2 p = Vector2Lerp(th->from, th->to, th->t);
        float lift = sinf(PI * th->t) * fminf(14.0f, Vector2Distance(th->from, th->to) * 0.12f);   // a little arc
        p.y -= lift;
        if (!TileDraw(T_BOTTLE, p.x - TILE / 2.0f, p.y - TILE / 2.0f, false))
            DrawRectangle((int)p.x - 2, (int)p.y - 3, 4, 7, (Color){ 130, 190, 150, 255 });
    }
    if (th->shatter > 0.0f) {                                      // six shards fly outward and fade
        float k = th->shatter / 0.5f;
        unsigned char a = (unsigned char)(255.0f * (1.0f - k));
        for (int i = 0; i < 6; i++) {
            float ang = 0.5f + i * 1.0472f, r = 4.0f + 14.0f * k;
            DrawRectangle((int)(th->at.x + cosf(ang) * r) - 1, (int)(th->at.y + sinf(ang) * r) - 1, 2, 2, (Color){ 200, 230, 220, a });
        }
    }
}
