#include "path.h"

bool FindPath(const Map *m, int sx, int sy, int gx, int gy, TilePath *out)
{
    static int prev[MAX_H * MAX_W];     // for each tile: the tile we came from (-2 = not visited yet)
    static int queue[MAX_H * MAX_W];

    out->len = 0;
    if (IsSolid(m, sx, sy) || IsSolid(m, gx, gy)) return false;

    int total = m->w * m->h;
    for (int i = 0; i < total; i++) prev[i] = -2;

    int start = sy * m->w + sx, goal = gy * m->w + gx;
    int head = 0, tail = 0;
    prev[start] = -1;
    queue[tail++] = start;

    static const int dx[4] = { 1, -1, 0, 0 };
    static const int dy[4] = { 0, 0, 1, -1 };

    while (head < tail && prev[goal] == -2) {
        int cur = queue[head++];
        int cx = cur % m->w, cy = cur / m->w;
        for (int d = 0; d < 4; d++) {
            int nx = cx + dx[d], ny = cy + dy[d];
            if (IsSolid(m, nx, ny)) continue;
            int ni = ny * m->w + nx;
            if (prev[ni] != -2) continue;
            prev[ni] = cur;
            queue[tail++] = ni;
        }
    }
    if (prev[goal] == -2) return false;

    // Walk back from the goal to the start, then flip the order.
    int n = 0;
    for (int i = goal; i != -1; i = prev[i]) {
        if (n >= PATH_MAX_LEN) return false;                 // route too long for our buffer
        out->x[n] = i % m->w;
        out->y[n] = i / m->w;
        n++;
    }
    for (int i = 0; i < n / 2; i++) {
        int tx = out->x[i], ty = out->y[i];
        out->x[i] = out->x[n - 1 - i]; out->y[i] = out->y[n - 1 - i];
        out->x[n - 1 - i] = tx;        out->y[n - 1 - i] = ty;
    }
    out->len = n;
    return true;
}
