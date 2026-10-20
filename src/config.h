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
#define KEY_PICKUP_DIST    12.0f    // walk this close to a key, fuse or battery to pick it up

// Inventory: a row of slots at the bottom of the screen. Items of one kind stack in a slot.
#define INV_SLOTS              6     // number of slots (selected with the keys 1-6 or the mouse wheel; at most 9)
#define BATTERY_STACK          3     // spare batteries in one slot
#define BOTTLE_STACK           5     // bottles in one slot

// Flashlight battery (only in chapters that use it: see chapters.h)
#define BATTERY_MAX        100.0f
#define BATTERY_DRAIN        1.2f    // lost per second while the flashlight is on
#define BATTERY_CHARGE      40.0f    // what one spare battery gives back when you load it
#define BATTERY_DIM_AT       0.25f   // below this fraction of a full battery the beam gets shorter...
#define BATTERY_FLICKER_AT   0.12f   // ...and below this one it flickers

// Throwing bottles (Chapter 3)
#define THROW_RANGE          210.0f  // how far you can throw (pixels)
#define THROW_SPEED          300.0f  // how fast a bottle flies (pixels per second)
#define THROW_NOISE_RADIUS   420.0f  // the stalker hears a shattering bottle this close

// Security cameras (Chapter 4): they hang from the ceiling, sweep a cone of view, and see in the dark
#define CAM_RANGE            150.0f  // how far they see (pixels)
#define CAM_FOV               22.0f  // half-width of the cone they see (degrees)
#define CAM_SWEEP             45.0f  // how far they swing left and right of their direction (degrees)
#define CAM_SWEEP_PERIOD       6.0f  // seconds for one full swing there and back
#define CAM_DETECT_TIME        0.8f  // seconds in view before the alarm rings
#define CAM_DECAY              0.8f  // how fast that empties again once you are out of view (per second)
#define CAM_ALARM_TIME         8.0f  // after an alarm no camera can ring again for this long
#define CAM_BREAK_DIST        24.0f  // a bottle that shatters this close to a camera breaks it

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
