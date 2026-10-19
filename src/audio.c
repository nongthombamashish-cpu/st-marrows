#include "audio.h"
#include "raymath.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

#define SR 44100                      // sample rate of every sound

// ---- Building sounds in code ---------------------------------------------------------------

static unsigned int rng = 2463534242u;
static void  Seed(unsigned int s) { rng = s ? s : 1u; }
static float Rand(void)               // white noise in [-1, 1]
{
    rng ^= rng << 13; rng ^= rng >> 17; rng ^= rng << 5;
    return (float)(rng & 0xFFFFFF) / 8388607.0f - 1.0f;
}

static float *Buf(float seconds, unsigned int *frames)
{
    *frames = (unsigned int)(seconds * SR);
    return (float *)RL_CALLOC(*frames, sizeof(float));
}

// Turns a float buffer into a Wave: loudest point set to 'peak' (0..1), short fades so it never clicks.
static Wave Finish(float *buf, unsigned int frames, float peak)
{
    float mx = 0.0001f;
    for (unsigned int i = 0; i < frames; i++) if (fabsf(buf[i]) > mx) mx = fabsf(buf[i]);
    float gain = peak / mx;

    short *data = (short *)RL_MALLOC(frames * sizeof(short));
    for (unsigned int i = 0; i < frames; i++) {
        float t = (float)i / SR, tail = (float)(frames - 1 - i) / SR;
        float fade = fminf(1.0f, fminf(t / 0.004f, tail / 0.02f));
        data[i] = (short)(Clamp(buf[i] * gain * fade, -1.0f, 1.0f) * 32000.0f);
    }
    RL_FREE(buf);

    Wave w = { 0 };
    w.frameCount = frames;
    w.sampleRate = SR;
    w.sampleSize = 16;
    w.channels = 1;
    w.data = data;
    return w;
}

// A low "thump": a sine (plus an octave above it) that dies away quickly. t0 = when it starts.
static float Thump(float t, float t0, float freq, float amp, float decay)
{
    if (t < t0) return 0.0f;
    float d = t - t0;
    return amp * (sinf(2.0f * PI * freq * d) + 0.45f * sinf(4.0f * PI * freq * d)) * expf(-d * decay) * fminf(1.0f, d / 0.005f);
}

// A bell-like tone: a few partials that fade out.
static float Bell(float t, float t0, float freq, float amp, float decay)
{
    if (t < t0) return 0.0f;
    float d = t - t0;
    float v = sinf(2.0f * PI * freq * d) + 0.45f * sinf(2.0f * PI * freq * 1.5f * d) + 0.2f * sinf(2.0f * PI * freq * 2.0f * d);
    return amp * v * expf(-d * decay) * fminf(1.0f, d / 0.003f);
}

Wave SynthStep(float freq)            // a footstep: a dull thump plus a little scuff
{
    unsigned int n; float *b = Buf(0.16f, &n);
    Seed(7u + (unsigned int)freq);
    float lp = 0;
    for (unsigned int i = 0; i < n; i++) {
        float t = (float)i / SR;
        lp += 0.18f * (Rand() - lp);
        b[i] = 0.9f * sinf(2.0f * PI * freq * t) * expf(-t * 38.0f) + 2.2f * lp * expf(-t * 55.0f);
    }
    return Finish(b, n, 0.8f);
}

Wave SynthStalkerStep(void)           // heavier and wetter than yours
{
    unsigned int n; float *b = Buf(0.3f, &n);
    Seed(99u);
    float lp = 0;
    for (unsigned int i = 0; i < n; i++) {
        float t = (float)i / SR;
        lp += 0.06f * (Rand() - lp);
        b[i] = sinf(2.0f * PI * 38.0f * t) * expf(-t * 12.0f) + 0.5f * sinf(2.0f * PI * 76.0f * t) * expf(-t * 20.0f)
             + 4.0f * lp * expf(-t * 14.0f);
    }
    return Finish(b, n, 0.9f);
}

Wave SynthHeartbeat(void)             // lub-dub
{
    unsigned int n; float *b = Buf(0.75f, &n);
    for (unsigned int i = 0; i < n; i++) {
        float t = (float)i / SR;
        b[i] = Thump(t, 0.00f, 52.0f, 1.0f, 18.0f) + Thump(t, 0.25f, 46.0f, 0.7f, 18.0f);
    }
    return Finish(b, n, 0.9f);
}

Wave SynthChime(void)                 // a key: bright and short
{
    unsigned int n; float *b = Buf(1.0f, &n);
    for (unsigned int i = 0; i < n; i++) {
        float t = (float)i / SR;
        b[i] = Bell(t, 0.0f, 880.0f, 1.0f, 4.0f) + 0.3f * Bell(t, 0.0f, 2217.0f, 1.0f, 9.0f);
    }
    return Finish(b, n, 0.7f);
}

Wave SynthPaper(void)                 // a rustle: high-passed noise that swells and fades
{
    unsigned int n; float *b = Buf(0.3f, &n);
    Seed(31u);
    float lp = 0;
    for (unsigned int i = 0; i < n; i++) {
        float t = (float)i / SR;
        float noise = Rand();
        lp += 0.2f * (noise - lp);                                  // high-pass: keep the mids, drop the lows
        float env = sinf(PI * t / 0.3f);
        b[i] = (noise - lp) * env * env * (0.6f + 0.4f * Rand());
    }
    return Finish(b, n, 0.6f);
}

Wave SynthLocker(void)                // a metal creak, then a thunk
{
    unsigned int n; float *b = Buf(0.5f, &n);
    float phase = 0, lp = 0;
    for (unsigned int i = 0; i < n; i++) {
        float t = (float)i / SR;
        float f = (t < 0.32f) ? 190.0f - 70.0f * (t / 0.32f) : 120.0f;
        phase += 2.0f * PI * f / SR;
        if (phase > 2.0f * PI) phase -= 2.0f * PI;
        float saw = phase / PI - 1.0f;
        lp += 0.08f * (saw - lp);
        float creak = (t < 0.32f) ? sinf(PI * t / 0.32f) : 0.0f;
        float tremolo = 0.7f + 0.3f * sinf(2.0f * PI * 26.0f * t);
        b[i] = 0.55f * lp * creak * tremolo * 4.0f + Thump(t, 0.33f, 62.0f, 1.0f, 28.0f);
    }
    return Finish(b, n, 0.7f);
}

Wave SynthLockedDoor(void)            // a dull rattle: "no"
{
    unsigned int n; float *b = Buf(0.35f, &n);
    Seed(5u);
    float lp = 0;
    for (unsigned int i = 0; i < n; i++) {
        float t = (float)i / SR;
        lp += 0.2f * (Rand() - lp);
        b[i] = Thump(t, 0.0f, 95.0f, 1.0f, 22.0f) + Thump(t, 0.13f, 80.0f, 0.8f, 26.0f) + 3.0f * lp * expf(-t * 28.0f);
    }
    return Finish(b, n, 0.8f);
}

Wave SynthAlert(void)                 // the stalker has seen you: a rising screech
{
    unsigned int n; float *b = Buf(0.9f, &n);
    Seed(1234u);
    float phase = 0;
    for (unsigned int i = 0; i < n; i++) {
        float t = (float)i / SR;
        float f = 320.0f + 900.0f * powf(t / 0.9f, 1.4f) + 14.0f * sinf(2.0f * PI * 6.5f * t);
        phase += 2.0f * PI * f / SR;
        if (phase > 2.0f * PI) phase -= 2.0f * PI;
        float saw = phase / PI - 1.0f;
        float env = fminf(1.0f, t / 0.01f) * expf(-t * 1.8f);
        b[i] = (0.6f * sinf(phase) + 0.4f * saw) * env + 0.5f * Rand() * expf(-t * 20.0f);
    }
    return Finish(b, n, 0.8f);
}

Wave SynthSting(void)                 // caught: a dissonant crash that sinks away
{
    unsigned int n; float *b = Buf(1.5f, &n);
    Seed(777u);
    static const float notes[4] = { 196.0f, 207.65f, 277.18f, 293.66f };      // notes a semitone apart clash on purpose
    float sweep = 0;
    for (unsigned int i = 0; i < n; i++) {
        float t = (float)i / SR;
        float v = 0;
        for (int k = 0; k < 4; k++) v += 0.25f * sinf(2.0f * PI * notes[k] * t);
        sweep += 2.0f * PI * (80.0f + 620.0f * expf(-t * 2.2f)) / SR;
        v += 0.5f * sinf(sweep);
        float env = fminf(1.0f, t / 0.004f) * expf(-t * 1.6f);
        b[i] = v * env + 0.8f * Rand() * expf(-t * 10.0f);
    }
    return Finish(b, n, 0.9f);
}

Wave SynthComplete(void)              // chapter complete: a rising arpeggio
{
    unsigned int n; float *b = Buf(2.2f, &n);
    static const float notes[4] = { 523.25f, 659.25f, 783.99f, 1046.5f };
    for (unsigned int i = 0; i < n; i++) {
        float t = (float)i / SR;
        float v = 0;
        for (int k = 0; k < 4; k++) v += Bell(t, 0.2f * k, notes[k], 0.6f, (k == 3) ? 1.8f : 3.0f);
        b[i] = v;
    }
    return Finish(b, n, 0.7f);
}

// The ambient drone: two slightly detuned low notes that beat slowly, a breathing hum, and an
// uneasy extra note that fades in with 'tension' (0..1).
void AudioDroneFill(short *out, unsigned int frames, float tension)
{
    static double p1, p2, p3, p4, lfo;
    static float lp, smooth;
    static unsigned int rs = 88172645u;
    const double k = 2.0 * 3.14159265358979323846 / SR, twoPi = 2.0 * 3.14159265358979323846;

    for (unsigned int i = 0; i < frames; i++) {
        p1 += k * 55.0;  p2 += k * 55.9;  p3 += k * 82.5;  p4 += k * 77.8;  lfo += k * 0.07;
        if (p1 >= twoPi) p1 -= twoPi;
        if (p2 >= twoPi) p2 -= twoPi;
        if (p3 >= twoPi) p3 -= twoPi;
        if (p4 >= twoPi) p4 -= twoPi;
        if (lfo >= twoPi) lfo -= twoPi;

        rs ^= rs << 13; rs ^= rs >> 17; rs ^= rs << 5;
        float noise = (float)(rs & 0xFFFFFF) / 8388607.0f - 1.0f;
        lp += 0.02f * (noise - lp);
        smooth += 0.0002f * (tension - smooth);                      // no sudden jumps in loudness

        float swell = 0.65f + 0.35f * (float)sin(lfo);
        float s = 0.5f * (float)sin(p1) + 0.5f * (float)sin(p2)
                + 0.2f * (float)sin(2.0 * p1)                         // octave above: laptop speakers can hear it
                + 0.3f * swell * (float)sin(p3) + 0.12f * (float)sin(2.0 * p3)
                + smooth * 0.35f * (float)sin(p4)
                + (0.25f + 0.5f * smooth) * lp * 4.0f;
        s *= 0.22f + 0.16f * smooth;
        out[i] = (short)(Clamp(s, -1.0f, 1.0f) * 32000.0f);
    }
}

// ---- Playing sounds ------------------------------------------------------------------------

#define POOL 4                                     // copies of a sound, so quick repeats can overlap
typedef struct { Sound s[POOL]; int next; } Pool;

static void PoolMake(Pool *p, Wave w)
{
    p->s[0] = LoadSoundFromWave(w);
    for (int i = 1; i < POOL; i++) p->s[i] = LoadSoundAlias(p->s[0]);
    p->next = 0;
    UnloadWave(w);
}

static void PoolPlay(Pool *p, float volume, float pan, float pitch)
{
    Sound s = p->s[p->next];
    p->next = (p->next + 1) % POOL;
    SetSoundVolume(s, Clamp(volume, 0.0f, 1.0f));
    SetSoundPan(s, Clamp(pan, -1.0f, 1.0f));         // -1 left .. +1 right
    SetSoundPitch(s, pitch);
    PlaySound(s);
}

static void PoolFree(Pool *p)
{
    for (int i = 1; i < POOL; i++) UnloadSoundAlias(p->s[i]);
    UnloadSound(p->s[0]);
}

static struct {
    bool ready, muted;
    float volume;                                   // 0..1, set from the menu
    Pool step[3], stalkerStep, heartbeat, chime, paper, locker, lockedDoor, alert, sting, complete;
    AudioStream drone;
} A;

static volatile float g_tension = 0.0f;           // read by the audio thread

static void DroneCallback(void *buffer, unsigned int frames)
{
    AudioDroneFill((short *)buffer, frames, g_tension);
}

bool AudioInit(void)
{
    memset(&A, 0, sizeof A);
    A.volume = 0.85f;
    InitAudioDevice();
    A.ready = IsAudioDeviceReady();
    if (!A.ready) return false;
    SetMasterVolume(A.volume);

    PoolMake(&A.step[0], SynthStep(58.0f));
    PoolMake(&A.step[1], SynthStep(66.0f));
    PoolMake(&A.step[2], SynthStep(74.0f));
    PoolMake(&A.stalkerStep, SynthStalkerStep());
    PoolMake(&A.heartbeat, SynthHeartbeat());
    PoolMake(&A.chime, SynthChime());
    PoolMake(&A.paper, SynthPaper());
    PoolMake(&A.locker, SynthLocker());
    PoolMake(&A.lockedDoor, SynthLockedDoor());
    PoolMake(&A.alert, SynthAlert());
    PoolMake(&A.sting, SynthSting());
    PoolMake(&A.complete, SynthComplete());

    A.drone = LoadAudioStream(SR, 16, 1);
    SetAudioStreamCallback(A.drone, DroneCallback);
    SetAudioStreamVolume(A.drone, 0.9f);
    PlayAudioStream(A.drone);
    return true;
}

void AudioShutdown(void)
{
    if (!A.ready) return;
    StopAudioStream(A.drone);
    UnloadAudioStream(A.drone);
    for (int i = 0; i < 3; i++) PoolFree(&A.step[i]);
    PoolFree(&A.stalkerStep); PoolFree(&A.heartbeat); PoolFree(&A.chime); PoolFree(&A.paper);
    PoolFree(&A.locker); PoolFree(&A.lockedDoor); PoolFree(&A.alert); PoolFree(&A.sting); PoolFree(&A.complete);
    CloseAudioDevice();
    A.ready = false;
}

void AudioToggleMute(void)
{
    A.muted = !A.muted;
    if (A.ready) SetMasterVolume(A.muted ? 0.0f : A.volume);
}

bool AudioIsMuted(void) { return A.muted; }

void AudioSetVolume(float v)
{
    A.volume = Clamp(v, 0.0f, 1.0f);
    if (A.ready && !A.muted) SetMasterVolume(A.volume);
}

float AudioGetVolume(void) { return A.volume; }

void AudioChime(void)      { if (A.ready) PoolPlay(&A.chime, 0.8f, 0.0f, 1.0f); }
void AudioPaper(void)      { if (A.ready) PoolPlay(&A.paper, 0.6f, 0.0f, 0.95f + 0.1f * (float)GetRandomValue(0, 10) / 10.0f); }
void AudioLocker(void)     { if (A.ready) PoolPlay(&A.locker, 0.7f, 0.0f, 0.92f + 0.16f * (float)GetRandomValue(0, 10) / 10.0f); }
void AudioLockedDoor(void) { if (A.ready) PoolPlay(&A.lockedDoor, 0.8f, 0.0f, 1.0f); }
void AudioSting(void)      { if (A.ready) PoolPlay(&A.sting, 1.0f, 0.0f, 1.0f); }
void AudioComplete(void)   { if (A.ready) PoolPlay(&A.complete, 0.9f, 0.0f, 1.0f); }
void AudioBattery(void)    { if (A.ready) PoolPlay(&A.chime, 0.7f, 0.0f, 0.62f); }          // the key chime, lower
void AudioFlashlightDead(void) { if (A.ready) PoolPlay(&A.lockedDoor, 0.55f, 0.0f, 1.7f); } // the "no" rattle, higher
void AudioPowerOn(void)    { if (A.ready) PoolPlay(&A.complete, 0.9f, 0.0f, 0.55f); }       // the chapter arpeggio, deep

// Footsteps, heartbeat, drone and the chase alert, driven by what the stalker is doing.
void AudioUpdate(const Player *p, const Stalker *s, bool playing, float dt)
{
    static float stepDist, stalkerDist, heartTimer;
    static Vector2 lastStalker;
    static StalkerState prevState = ST_PATROL;
    if (!A.ready) return;

    // How worried should you be? 0 = stalker far away, 1 = right on top of you.
    float dist = 1.0e9f, closeness = 0.0f;
    if (s->active) {
        dist = Vector2Distance(s->pos, p->pos);
        closeness = Clamp(1.0f - dist / 260.0f, 0.0f, 1.0f);
        if (s->state == ST_CHASE) closeness = fmaxf(closeness, 0.85f);
    }
    g_tension = playing ? closeness : 0.0f;

    if (!playing) {
        lastStalker = s->pos;
        prevState = s->state;
        return;
    }

    // The stalker has just spotted you.
    if (s->active && s->state == ST_CHASE && prevState != ST_CHASE) PoolPlay(&A.alert, 0.85f, 0.0f, 1.0f);
    prevState = s->state;

    // Your footsteps: one per stride, louder when you run.
    float stride = p->running ? 22.0f : 17.0f;
    if (p->moving && !p->hidden) {
        stepDist += (p->running ? PLAYER_RUN : PLAYER_WALK) * dt;
        if (stepDist >= stride) {
            stepDist -= stride;
            PoolPlay(&A.step[GetRandomValue(0, 2)], p->running ? 0.7f : 0.38f, 0.0f, 0.92f + 0.16f * (float)GetRandomValue(0, 10) / 10.0f);
        }
    } else {
        stepDist = stride * 0.7f;                         // so your first step comes quickly
    }

    // Its footsteps: quieter with distance, panned left/right. You can hear it through walls.
    if (s->active) {
        float moved = Vector2Distance(s->pos, lastStalker);
        lastStalker = s->pos;
        if (moved < 8.0f) {                               // (ignore the jump when the game restarts)
            stalkerDist += moved;
            float sStride = (s->state == ST_CHASE) ? 22.0f : 16.0f;
            if (stalkerDist >= sStride) {
                stalkerDist -= sStride;
                float v = Clamp(1.0f - dist / 300.0f, 0.0f, 1.0f);
                v = v * v * 0.95f;
                if (v > 0.02f)
                    PoolPlay(&A.stalkerStep, v, Clamp((s->pos.x - p->pos.x) / 160.0f, -1.0f, 1.0f), 0.95f + 0.1f * (float)GetRandomValue(0, 10) / 10.0f);
            }
        }
    }

    // Your heartbeat speeds up and gets louder as it gets close.
    heartTimer -= dt;
    if (closeness > 0.12f) {
        if (heartTimer <= 0.0f) {
            PoolPlay(&A.heartbeat, 0.35f + 0.6f * closeness, 0.0f, 1.0f);
            heartTimer = 1.05f - 0.65f * closeness;       // 1.05 s apart when far .. 0.4 s when close
        }
    } else {
        heartTimer = fminf(heartTimer, 0.4f);             // so it starts soon after it gets near
    }
}