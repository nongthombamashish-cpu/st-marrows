// inventory.h - what you carry: a row of slots (the hotbar). Items of the same kind stack in one slot.
//
// To add a new item to the game:
//   1. add its name to ItemId below, and a row for it to ITEMS in inventory.c
//      (name, what a click does, the map character, stack size, picture, plain-shape colour)
//   2. put its map character on a map: it lies on the floor, and walking over it picks it up
//   3. add a 'case' for it in UseSelected() in main.c: what happens when you click
// Nothing else changes: picking up, stacking, the hotbar and drawing it on the map all use the table.
#ifndef INVENTORY_H
#define INVENTORY_H

#include <stdbool.h>
#include "raylib.h"
#include "tiles.h"
#include "config.h"

typedef enum { ITEM_NONE = 0, ITEM_BATTERY, ITEM_BOTTLE, ITEM_COUNT } ItemId;    // <- new items go here, before ITEM_COUNT

typedef struct {
    const char *name;        // shown in the hotbar
    const char *hint;        // what a click does (shown under the name)
    char        mapChar;     // the character on a map that means "this item lies here"
    int         maxStack;    // how many fit in one slot
    TileId      tile;        // its picture
    Color       color;       // the plain shape's colour, if the picture is not set up
} ItemInfo;

typedef struct { ItemId id; int count; } Slot;

typedef struct {
    Slot slot[INV_SLOTS];
    int  selected;           // the highlighted slot
} Inventory;

void InvInit(Inventory *inv);
bool InvAdd(Inventory *inv, ItemId id);               // puts one in (a stack with room first, then an empty slot). False if there is no room.
void InvRemove(Inventory *inv, int slot);             // uses up one item from a slot
int  InvCount(const Inventory *inv, ItemId id);       // how many of this item you carry in total
int  InvFind(const Inventory *inv, ItemId id);        // the first slot that holds it, or -1
void InvSelect(Inventory *inv, int slot);
void InvScroll(Inventory *inv, int step);             // +1 next slot, -1 previous slot (wraps around)
void InvDraw(const Inventory *inv, int screenW, int screenH);    // the hotbar, bottom centre (call it with the HUD)

const ItemInfo *ItemGet(ItemId id);
ItemId ItemFromMapChar(char c);                       // ITEM_NONE if the character is not an item
void   ItemDraw(ItemId id, float x, float y);         // draws it lying on the floor (the TILE x TILE spot at x, y)

#endif
