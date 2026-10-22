#include "player.h"
#include "raymath.h"
#include "tiles.h"
#include <math.h>

void PlayerInit(Player *p, Vector2 start)
{
    p->pos = start;
    p->half = 5.0f;
    p->flashlight = true;
    p->battery = BATTERY_MAX;
    p->usesBattery = false;
    InvInit(&p->inv);
    p->moving = false;
    p->running = false;
    p->hidden = false;
    p->exitPos = start;
}

bool PlayerLoadBattery(Player *p, int slot)
{
    if (!p->usesBattery || slot < 0 || slot >= INV_SLOTS || p->inv.slot[slot].id != ITEM_BATTERY) return false;
    if (p->battery >= BATTERY_MAX - 0.5f) return false;                        // already full: do not waste it
    InvRemove(&p->inv, slot);
    p->battery += BATTERY_CHARGE;
    if (p->battery > BATTERY_MAX) p->battery = BATTERY_MAX;
    return true;
}

void PlayerUpdate(Player *p, const Map *m, float dt)
{
    if (p->hidden) {                                    // inside a locker: no walking (main.c handles E to climb out)
        p->moving = p->running = false;
        return;
    }

    Vector2 dir = { 0, 0 };
    if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP))    dir.y -= 1;
    if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN))  dir.y += 1;
    if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT))  dir.x -= 1;
    if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) dir.x += 1;

    p->moving  = (dir.x != 0 || dir.y != 0);
    p->running = p->moving && IsKeyDown(KEY_LEFT_SHIFT);
    if (p->moving) dir = Vector2Normalize(dir);

    float speed = p->running ? PLAYER_RUN : PLAYER_WALK;
    MoveBox(m, &p->pos, p->half, Vector2Scale(dir, speed * dt));

    if (IsKeyPressed(KEY_F)) {
        if (p->flashlight) p->flashlight = false;
        else if (!p->usesBattery || p->battery > 0.0f) p->flashlight = true;
        else if (PlayerLoadBattery(p, InvFind(&p->inv, ITEM_BATTERY))) p->flashlight = true;   // dead: F loads a spare battery if you carry one
    }
    if (p->usesBattery && p->flashlight) {                                         // the light costs charge
        p->battery -= BATTERY_DRAIN * dt;
        if (p->battery <= 0.0f) { p->battery = 0.0f; p->flashlight = false; }
    }
}

float PlayerNoise(const Player *p)
{
    if (p->hidden || !p->moving) return 0.0f;           // standing still or hiding = silent
    return p->running ? PLAYER_NOISE_RUN : PLAYER_NOISE_WALK;
}

// Looks at the tiles around you for the nearest locker ('L') within reach.
bool PlayerNearLocker(const Player *p, const Map *m, Vector2 *lockerCentre)
{
    int cx = (int)floorf(p->pos.x / TILE), cy = (int)floorf(p->pos.y / TILE);
    float best = LOCKER_REACH;
    bool found = false;
    for (int y = cy - 2; y <= cy + 2; y++) {
        for (int x = cx - 2; x <= cx + 2; x++) {
            if (x < 0 || y < 0 || x >= m->w || y >= m->h || m->tiles[y][x] != 'L') continue;
            Vector2 c = { (x + 0.5f) * TILE, (y + 0.5f) * TILE };
            float d = Vector2Distance(p->pos, c);
            if (d < best) {
                best = d;
                found = true;
                if (lockerCentre) *lockerCentre = c;
            }
        }
    }
    return found;
}

bool PlayerTryHide(Player *p, const Map *m)
{
    Vector2 c;
    if (p->hidden || !PlayerNearLocker(p, m, &c)) return false;
    p->exitPos = p->pos;                                // remember where to climb out
    p->pos = c;
    p->hidden = true;
    p->moving = p->running = false;
    return true;
}

void PlayerLeaveLocker(Player *p)
{
    if (!p->hidden) return;
    p->pos = p->exitPos;
    p->hidden = false;
}

void DrawPlayer(const Player *p, Vector2 aim)
{
    if (p->hidden) return;
    float t = (float)GetTime();
    float bob;
    if (p->moving) bob = fabsf(sinf(t * (p->running ? 19.0f : 12.0f))) * (p->running ? 2.0f : 1.4f);   // a hop for every step
    else           bob = (sinf(t * 2.2f) + 1.0f) * 0.35f;                                              // standing still: a slow breath
    bob = roundf(bob);                                                                                // whole pixels: crisp pixel art

    DrawEllipse((int)p->pos.x, (int)p->pos.y + 6, 5.0f - bob * 0.5f, 2.0f, (Color){ 0, 0, 0, 90 });   // the shadow shrinks as the feet leave the floor
    Rectangle dst = { p->pos.x - TILE / 2.0f, p->pos.y - TILE / 2.0f - bob, (float)TILE, (float)TILE };
    if (TileDrawEx(T_PLAYER, dst, WHITE, aim.x < 0)) return;
    DrawCircleV(p->pos, p->half, (Color){ 225, 205, 160, 255 });                                      // the plain shape, if there is no picture
    DrawLineEx(p->pos, Vector2Add(p->pos, Vector2Scale(Vector2Normalize(aim), 7)), 2, (Color){ 255, 240, 200, 255 });
}
