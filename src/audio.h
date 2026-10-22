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
void AudioSetVolume(float v);         // master volume, 0..1
float AudioGetVolume(void);

// Call once per frame: footsteps, heartbeat, the ambient drone and the chase alert.
void AudioUpdate(const Player *p, const Stalker *s, bool playing, float dt);

// What this chapter sounds like: the drone's pitch (1 = normal), and the floor under your feet (footstep pitch and loudness).
void AudioSetScene(float dronePitch, float stepPitch, float stepVolume);
void AudioSetHunt(bool on);           // the final hunt: a racing heartbeat the whole time

// One-off sounds.
void AudioChime(void);                // key picked up
void AudioPaper(void);                // note opened / closed
void AudioLocker(void);               // climbing into / out of a locker
void AudioLockedDoor(void);           // the exit is locked
void AudioSting(void);                // the stalker caught you
void AudioComplete(void);             // chapter complete
void AudioBattery(void);              // battery picked up
void AudioFlashlightDead(void);       // the flashlight has no battery left
void AudioPowerOn(void);              // the fuse box catches: power restored
void AudioThrow(void);                // a bottle leaves your hand
void AudioBottle(void);               // a bottle picked up (a little clink)
void AudioAlarm(void);                // a security camera saw you
void AudioCameraBreak(void);          // a security camera smashed
void AudioGlass(float volume, float pan);   // a bottle shatters somewhere (volume 0..1, pan -1 left .. +1 right)

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
Wave SynthDrip(void);
Wave SynthCreak(void);
Wave SynthThump(void);
Wave SynthAlarm(void);
Wave SynthGlass(void);
Wave SynthThrow(void);
void AudioDroneFill(short *out, unsigned int frames, float tension);   // what the ambient drone plays
void AudioDroneFillEx(short *out, unsigned int frames, float tension, float pitch);   // the same, at another pitch (1 = normal)

#endif