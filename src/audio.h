// audio.h - all the sound. Everything is synthesized in code, so the game needs no audio files yet.
#ifndef AUDIO_H
#define AUDIO_H

#include "raylib.h"
#include "player.h"
#include "stalker.h"

bool AudioInit(void);                 // false if there is no sound device (the game then runs silently)
void AudioShutdown(void);
void AudioToggleMute(void);
bool AudioIsMuted(void);

// Call once per frame: footsteps, heartbeat, the ambient drone and the chase alert.
void AudioUpdate(const Player *p, const Stalker *s, bool playing, float dt);

// One-off sounds.
void AudioChime(void);                // key picked up
void AudioPaper(void);                // note opened / closed
void AudioLocker(void);               // climbing into / out of a locker
void AudioLockedDoor(void);           // the exit is locked
void AudioSting(void);                // the stalker caught you
void AudioComplete(void);             // chapter complete

// The sounds themselves (also used by the tests). The caller frees a Wave with UnloadWave.
Wave SynthStep(float freq);
Wave SynthStalkerStep(void);
Wave SynthHeartbeat(void);
Wave SynthChime(void);
Wave SynthPaper(void);
Wave SynthLocker(void);
Wave SynthLockedDoor(void);
Wave SynthAlert(void);
Wave SynthSting(void);
Wave SynthComplete(void);
void AudioDroneFill(short *out, unsigned int frames, float tension);   // what the ambient drone plays

#endif