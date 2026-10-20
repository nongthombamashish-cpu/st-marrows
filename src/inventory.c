#include "inventory.h"

_Static_assert(INV_SLOTS >= 1 && INV_SLOTS <= 9, "INV_SLOTS must be 1..9 (one number key each)");

// ---- The item table: one row per item. This is the only place an item is described. ----
static const ItemInfo ITEMS[ITEM_COUNT] = {
    [ITEM_NONE]    = { "",        "",                                  0,   0,             T_FLOOR1,  { 0, 0, 0, 0 } },
    [ITEM_BATTERY] = { "Battery", "Click: load it into the flashlight", 'B', BATTERY_STACK, T_BATTERY, {  90, 200, 255, 255 } },
    [ITEM_BOTTLE]  = { "Bottle",  "Click: throw it where you aim",      'o', BOTTLE_STACK,  T_BOTTLE,  { 130, 190, 150, 255 } },
};

const ItemInfo *ItemGet(ItemId id)
{
    if (id <= ITEM_NONE || id >= ITEM_COUNT) return &ITEMS[ITEM_NONE];
    return &ITEMS[id];
}

ItemId ItemFromMapChar(char c)
{
    for (int i = ITEM_NONE + 1; i < ITEM_COUNT; i++)
        if (ITEMS[i].mapChar == c) return (ItemId)i;
    return ITEM_NONE;
}

void ItemDraw(ItemId id, float x, float y)
{
    const ItemInfo *it = ItemGet(id);
    if (TileDraw(it->tile, x, y, false)) return;
    DrawRectangle((int)x + 5, (int)y + 4, 6, 9, it->color);                    // plain little box with a cap
    DrawRectangle((int)x + 7, (int)y + 2, 2, 2, (Color){ 200, 210, 220, 255 });
}

// ---- The slots ----

void InvInit(Inventory *inv)
{
    for (int i = 0; i < INV_SLOTS; i++) inv->slot[i] = (Slot){ ITEM_NONE, 0 };
    inv->selected = 0;
}

bool InvAdd(Inventory *inv, ItemId id)
{
    if (id <= ITEM_NONE || id >= ITEM_COUNT) return false;
    int max = ITEMS[id].maxStack;
    for (int i = 0; i < INV_SLOTS; i++) {                                       // 1. a stack of the same item with room
        if (inv->slot[i].id == id && inv->slot[i].count < max) { inv->slot[i].count++; return true; }
    }
    for (int i = 0; i < INV_SLOTS; i++) {                                       // 2. an empty slot
        if (inv->slot[i].id == ITEM_NONE) {
            inv->slot[i] = (Slot){ id, 1 };
            if (inv->slot[inv->selected].id == ITEM_NONE) inv->selected = i;    // nothing selected yet: select the new item
            return true;
        }
    }
    return false;                                                               // full
}

void InvRemove(Inventory *inv, int slot)
{
    if (slot < 0 || slot >= INV_SLOTS || inv->slot[slot].id == ITEM_NONE) return;
    if (--inv->slot[slot].count <= 0) inv->slot[slot] = (Slot){ ITEM_NONE, 0 };
}

int InvCount(const Inventory *inv, ItemId id)
{
    int n = 0;
    for (int i = 0; i < INV_SLOTS; i++) if (inv->slot[i].id == id) n += inv->slot[i].count;
    return n;
}

int InvFind(const Inventory *inv, ItemId id)
{
    for (int i = 0; i < INV_SLOTS; i++) if (inv->slot[i].id == id) return i;
    return -1;
}

void InvSelect(Inventory *inv, int slot)
{
    if (slot >= 0 && slot < INV_SLOTS) inv->selected = slot;
}

void InvScroll(Inventory *inv, int step)
{
    inv->selected = ((inv->selected + step) % INV_SLOTS + INV_SLOTS) % INV_SLOTS;
}

// ---- The hotbar ----

void InvDraw(const Inventory *inv, int sw, int sh)
{
    const int size = 46, gap = 6;                                               // each slot is 46 px: a 16 px tile drawn 2x fits inside
    int x0 = (sw - (INV_SLOTS * size + (INV_SLOTS - 1) * gap)) / 2;
    int y = sh - size - 12;

    for (int i = 0; i < INV_SLOTS; i++) {
        int x = x0 + i * (size + gap);
        bool sel = (i == inv->selected);
        DrawRectangle(x, y, size, size, (Color){ 18, 20, 26, 210 });
        DrawRectangleLinesEx((Rectangle){ (float)x, (float)y, (float)size, (float)size }, sel ? 3.0f : 1.0f,
                             sel ? (Color){ 235, 205, 110, 255 } : (Color){ 90, 92, 104, 255 });
        DrawText(TextFormat("%d", i + 1), x + 4, y + 3, 12, (Color){ 150, 150, 160, 200 });

        const Slot *s = &inv->slot[i];
        if (s->id == ITEM_NONE) continue;
        const ItemInfo *it = ItemGet(s->id);
        Rectangle icon = { (float)(x + 7), (float)(y + 7), (float)(size - 14), (float)(size - 14) };
        if (!TileDrawEx(it->tile, icon, WHITE, false))
            DrawRectangle((int)icon.x + 8, (int)icon.y + 6, (int)icon.width - 16, (int)icon.height - 10, it->color);
        if (s->count > 1) DrawText(TextFormat("x%d", s->count), x + size - 22, y + size - 16, 14, RAYWHITE);
    }

    const Slot *cur = &inv->slot[inv->selected];                                // the selected item: its name and what a click does
    if (cur->id != ITEM_NONE) {
        const ItemInfo *it = ItemGet(cur->id);
        const char *label = TextFormat("%s   -   %s", it->name, it->hint);
        DrawText(label, (sw - MeasureText(label, 16)) / 2, y - 22, 16, (Color){ 235, 205, 110, 230 });
    }
}
