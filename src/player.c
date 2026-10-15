#include "player.h"
#include "raymath.h"
#include <math.h>

void PlayerInit(Player *p, Vector2 start)
{
    p->pos = start;
    p->half = 5.0f;
    p->flashlight = true;
    p->moving = false;
    p->running = false;
    p->hidden = false;
    p->exitPos = start;
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

    if (IsKeyPressed(KEY_F)) p->flashlight = !p->flashlight;
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
