#include "hunt.h"
#include <string.h>

void HuntInit(Hunt *h)
{
    memset(h, 0, sizeof *h);
}

void HuntStart(Hunt *h, float seconds)
{
    h->active = true;
    h->left = seconds;
    h->ping = 0.0f;
}

HuntEvent HuntUpdate(Hunt *h, float dt)
{
    if (!h->active) return HUNT_QUIET;
    h->left -= dt;
    if (h->left <= 0.0f) {
        h->left = 0.0f;
        h->active = false;
        return HUNT_TIMEOUT;
    }
    h->ping += dt;
    if (h->ping >= HUNT_PING) {
        h->ping -= HUNT_PING;
        return HUNT_PING_NOW;
    }
    return HUNT_QUIET;
}
