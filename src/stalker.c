#include "stalker.h"
#include "raymath.h"
#include "tiles.h"
#include <math.h>
#include <string.h>

static float AngleDiff(float a, float b)            // shortest signed angle from b to a
{
    float d = fmodf(a - b + PI, 2.0f * PI);
    if (d < 0) d += 2.0f * PI;
    return d - PI;
}

// Plan a route from where the stalker stands to 'dest'.
static void GoTo(Stalker *s, const Map *m, Vector2 dest)
{
    FindPath(m, (int)(s->pos.x / TILE), (int)(s->pos.y / TILE),
                (int)(dest.x / TILE),   (int)(dest.y / TILE), &s->path);
    s->pathIdx = (s->path.len > 1) ? 1 : s->path.len;       // tile 0 is the one it already stands on
}

// Walk along the planned route. Returns true once the end is reached (or there is no route).
static bool FollowPath(Stalker *s, float speed, float dt)
{
    if (s->pathIdx >= s->path.len) return true;
    Vector2 goal = { (s->path.x[s->pathIdx] + 0.5f) * TILE, (s->path.y[s->pathIdx] + 0.5f) * TILE };
    Vector2 d = Vector2Subtract(goal, s->pos);
    float dist = Vector2Length(d);
    float step = speed * dt;
    if (dist <= step) {
        s->pos = goal;
        s->pathIdx++;
        return s->pathIdx >= s->path.len;
    }
    s->pos = Vector2Add(s->pos, Vector2Scale(d, step / dist));
    s->facing = atan2f(d.y, d.x);
    return false;
}

static void NextWaypoint(Stalker *s, const Map *m)
{
    for (int i = 0; i < m->waypointCount; i++) {
        s->wp = (s->wp + 1) % m->waypointCount;
        if (m->waypoint[s->wp].x >= 0) { GoTo(s, m, m->waypoint[s->wp]); return; }
    }
}

static void ResumePatrol(Stalker *s, const Map *m)
{
    s->state = ST_PATROL;
    s->timer = 0;
    if (s->wp >= 0 && m->waypoint[s->wp].x >= 0) GoTo(s, m, m->waypoint[s->wp]);
    else NextWaypoint(s, m);
}

void StalkerInit(Stalker *s, const Map *m)
{
    memset(s, 0, sizeof *s);
    s->active = m->hasStalker;
    s->pos = m->stalkerStart;
    s->facing = PI;
    s->state = ST_PATROL;
    s->wp = -1;
}

bool StalkerUpdate(Stalker *s, const Map *m, const Player *p, float dt)
{
    if (!s->active) return false;

    // Did it watch you climb into a locker? Then it knows where you are.
    if (p->hidden && !s->prevHidden) s->knowsHiding = s->sawPlayer;
    if (!p->hidden) s->knowsHiding = false;
    s->prevHidden = p->hidden;

    Vector2 player = p->pos;                                     // the locker's centre while you hide
    Vector2 toPlayer = Vector2Subtract(player, s->pos);
    float dist = Vector2Length(toPlayer);
    bool los = !p->hidden && HasLineOfSight(m, s->pos, player);   // nobody can see into a closed locker

    // What does it notice?
    bool inView = fabsf(AngleDiff(atan2f(toPlayer.y, toPlayer.x), s->facing)) < STALKER_FOV_HALF;
    float sightRange = p->flashlight ? STALKER_SIGHT_LIT : STALKER_SIGHT_DARK;
    bool sees = los && ((inView && dist < sightRange) || dist < 20.0f);
    if (s->state == ST_CHASE) sees = los && dist < STALKER_SIGHT_LIT + 40.0f;   // once chasing, it watches every direction
    bool hears = !p->hidden && dist < PlayerNoise(p);                           // sound goes through walls
    s->sawPlayer = sees;

    // React.
    if (p->hidden && s->knowsHiding) {                 // it saw you climb in: it comes to open the locker
        if (s->state != ST_CHASE) { s->state = ST_CHASE; s->repath = 0; }
        s->target = p->exitPos;
        s->timer = 0;
        if (Vector2Distance(s->pos, player) < STALKER_LOCKER_DIST) return true;
    } else if (sees) {
        if (s->state != ST_CHASE) { s->state = ST_CHASE; s->repath = 0; }
        s->target = player;
        s->timer = 0;
    } else if (hears && s->state != ST_CHASE) {
        if (s->state != ST_INVESTIGATE || Vector2Distance(s->target, player) > 24.0f) {
            s->state = ST_INVESTIGATE;
            s->target = player;
            s->timer = 0;
            GoTo(s, m, s->target);
        }
    }

    // Act.
    switch (s->state) {
    case ST_PATROL:
        if (!FollowPath(s, STALKER_PATROL_SPEED, dt)) { s->timer = 0; break; }
        s->timer += dt;                               // at a patrol point: look around, then move on
        s->facing += 0.9f * dt;
        if (s->timer > 1.5f) { s->timer = 0; NextWaypoint(s, m); }
        break;

    case ST_INVESTIGATE:
        if (!FollowPath(s, STALKER_INVESTIGATE_SPEED, dt)) { s->timer = 0; break; }
        s->timer += dt;                               // reached the noise: look around, then give up
        s->facing += 1.2f * dt;
        if (s->timer > 2.0f) ResumePatrol(s, m);
        break;

    case ST_CHASE: {
        bool arrived;
        if (!sees) s->timer += dt;                    // lost sight: keep running to where it last saw you
        if (sees && dist < 48.0f && dist > 0.001f) {  // close and visible: run straight at the player
            Vector2 dir = Vector2Scale(toPlayer, 1.0f / dist);
            MoveBox(m, &s->pos, 5.0f, Vector2Scale(dir, STALKER_CHASE_SPEED * dt));
            s->facing = atan2f(dir.y, dir.x);
            s->repath = 0;
            arrived = false;
        } else {
            s->repath -= dt;
            if (s->repath <= 0) { GoTo(s, m, s->target); s->repath = 0.25f; }
            arrived = FollowPath(s, STALKER_CHASE_SPEED, dt);
        }
        if (!sees && (arrived || s->timer > 4.0f)) { s->state = ST_SEARCH; s->timer = 0; }
        break;
    }

    case ST_SEARCH:
        s->timer += dt;                               // stand there and look around
        s->facing += 1.5f * dt;
        if (s->timer > 4.0f) ResumePatrol(s, m);
        break;
    }

    return los && dist < STALKER_CATCH_DIST;
}

const char *StalkerStateName(const Stalker *s)
{
    switch (s->state) {
        case ST_PATROL:      return "PATROL";
        case ST_INVESTIGATE: return "INVESTIGATE";
        case ST_CHASE:       return "CHASE";
        case ST_SEARCH:      return "SEARCH";
    }
    return "?";
}

void DrawStalker(const Stalker *s)
{
    if (!s->active) return;
    if (TileDraw(T_STALKER, s->pos.x - TILE / 2.0f, s->pos.y - TILE / 2.0f, cosf(s->facing) < 0)) return;
    Color body = (s->state == ST_CHASE) ? (Color){ 235, 50, 50, 255 } : (Color){ 170, 70, 70, 255 };
    DrawCircleV(s->pos, 6, body);
    Vector2 nose = { s->pos.x + cosf(s->facing) * 9, s->pos.y + sinf(s->facing) * 9 };
    DrawLineEx(s->pos, nose, 2, (Color){ 255, 210, 210, 255 });
}
