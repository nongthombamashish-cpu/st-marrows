// config.h - shared settings. Tune the feel of the game here.
#ifndef CONFIG_H
#define CONFIG_H

// Picture
#define VIRT_W 640           // the game is drawn at this small size...
#define VIRT_H 360           // ...then scaled up to the window (crisp pixel art)
#define TILE    16           // tile size in pixels

// Flashlight
#define BEAM_RANGE 150.0f    // reach, in pixels
#define BEAM_HALF    0.45f   // half-width of the cone, in radians
#define GLOW_RANGE  36.0f    // faint glow around the player

// Player (pixels per second / pixels)
#define PLAYER_WALK        50.0f
#define PLAYER_RUN         90.0f
#define PLAYER_NOISE_WALK  40.0f    // how far away a walking player can be heard
#define PLAYER_NOISE_RUN  120.0f    // ...and a running one
#define LOCKER_REACH       22.0f    // how close you must stand to a locker to hide in it
#define ITEM_REACH         22.0f    // how close you must be to read a note or use the exit
#define KEY_PICKUP_DIST    12.0f    // walk this close to a key to pick it up

// Stalker (pixels per second / pixels)
#define STALKER_PATROL_SPEED       30.0f
#define STALKER_INVESTIGATE_SPEED  44.0f
#define STALKER_CHASE_SPEED        68.0f   // faster than walking, slower than running
#define STALKER_SIGHT_LIT         190.0f   // sees you this far away when your flashlight is on...
#define STALKER_SIGHT_DARK         55.0f   // ...and this far when it is off
#define STALKER_FOV_HALF            1.0f   // half of its field of view, in radians (about 57 degrees)
#define STALKER_CATCH_DIST         11.0f   // closer than this = caught
#define STALKER_LOCKER_DIST        24.0f   // if it saw you hide, it opens your locker from this close

#endif
