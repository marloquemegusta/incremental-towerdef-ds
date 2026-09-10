#ifndef SKILLS_H
#define SKILLS_H

#include <nds.h>

#define SKILL_TRACK_COUNT 3
#define SKILL_TIERS_PER_TRACK 3
#define SKILL_NODE_COUNT 9

typedef struct {
    int id;
    int track;        // 0=Cadencia, 1=Calibre/Daño, 2=Penetración AP
    int tier;         // 0=T1, 1=T2, 2=T3
    const char *code; // "ROF-1", etc.
    const char *name; // "ALIMENTADOR DOBLE"
    const char *desc; // "+25% CADENCIA"
    int cost;
    int unlocked;     // 1 if prerequisite purchased
    int purchased;    // 1 if acquired
    int x, y, w, h;   // Screen bounding box on 256x192
} SkillNode;

typedef struct {
    SkillNode nodes[SKILL_NODE_COUNT];
    int selected_node; // -1 if none, 0..8
    int bonus_firerate;
    int bonus_damage;
    int bonus_ap;
} SkillTree;

extern SkillTree g_skill_tree;

void skills_init(void);
int skills_try_purchase(int node_id);
int skills_handle_touch(int touch_x, int touch_y);
void skills_draw_tree(void);
const char *skills_get_active_doctrine_name(void);

#endif // SKILLS_H
