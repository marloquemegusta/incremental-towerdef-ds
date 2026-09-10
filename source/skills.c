#include "skills.h"
#include "game.h"

SkillTree g_skill_tree;

void skills_init(void) {
    memset(&g_skill_tree, 0, sizeof(g_skill_tree));
    g_skill_tree.selected_node = 0;

    // TRACK 0: CADENCIA (Rate of Fire)
    g_skill_tree.nodes[0] = (SkillNode){
        .id = 0, .track = 0, .tier = 0,
        .code = "ROF-1", .name = "ALIMENTADOR DOBLE",
        .desc = "+25% VELOCIDAD DISPARO",
        .cost = 15, .unlocked = 1, .purchased = 0,
        .x = 76, .y = 30, .w = 48, .h = 22
    };
    g_skill_tree.nodes[1] = (SkillNode){
        .id = 1, .track = 0, .tier = 1,
        .code = "ROF-2", .name = "CAMISAS REFRIGERADAS",
        .desc = "+50% CADENCIA CONTINUA",
        .cost = 30, .unlocked = 0, .purchased = 0,
        .x = 136, .y = 30, .w = 48, .h = 22
    };
    g_skill_tree.nodes[2] = (SkillNode){
        .id = 2, .track = 0, .tier = 2,
        .code = "ROF-3", .name = "SERVOS OVERCLOCK",
        .desc = "FUEGO TORMENTA (20 B/S)",
        .cost = 60, .unlocked = 0, .purchased = 0,
        .x = 196, .y = 30, .w = 48, .h = 22
    };

    // TRACK 1: CALIBRE & DANIO (Damage)
    g_skill_tree.nodes[3] = (SkillNode){
        .id = 3, .track = 1, .tier = 0,
        .code = "DMG-1", .name = "PROPELENTE DENSO",
        .desc = "+1 DANIO POR IMPACTO",
        .cost = 15, .unlocked = 1, .purchased = 0,
        .x = 76, .y = 68, .w = 48, .h = 22
    };
    g_skill_tree.nodes[4] = (SkillNode){
        .id = 4, .track = 1, .tier = 1,
        .code = "DMG-2", .name = "PUNTAS EXPLOSIVAS",
        .desc = "+2 DANIO ADICIONAL",
        .cost = 35, .unlocked = 0, .purchased = 0,
        .x = 136, .y = 68, .w = 48, .h = 22
    };
    g_skill_tree.nodes[5] = (SkillNode){
        .id = 5, .track = 1, .tier = 2,
        .code = "DMG-3", .name = "OJIVAS KRAKEN",
        .desc = "+4 DANIO MASIVO XENOS",
        .cost = 70, .unlocked = 0, .purchased = 0,
        .x = 196, .y = 68, .w = 48, .h = 22
    };

    // TRACK 2: PENETRACION AP (Armor Piercing)
    g_skill_tree.nodes[6] = (SkillNode){
        .id = 6, .track = 2, .tier = 0,
        .code = "AP-1",  .name = "NUCLEO SABOT AP",
        .desc = "+1 PERFORACION BLINDAJE",
        .cost = 20, .unlocked = 1, .purchased = 0,
        .x = 76, .y = 106, .w = 48, .h = 22
    };
    g_skill_tree.nodes[7] = (SkillNode){
        .id = 7, .track = 2, .tier = 1,
        .code = "AP-2",  .name = "FOSFORO CORROSIVO",
        .desc = "+2 AP PERFORA QUITINA",
        .cost = 40, .unlocked = 0, .purchased = 0,
        .x = 136, .y = 106, .w = 48, .h = 22
    };
    g_skill_tree.nodes[8] = (SkillNode){
        .id = 8, .track = 2, .tier = 2,
        .code = "AP-3",  .name = "PLASMA VENGANZA",
        .desc = "IGNORA 100% ARMADURA",
        .cost = 80, .unlocked = 0, .purchased = 0,
        .x = 196, .y = 106, .w = 48, .h = 22
    };
}

int skills_try_purchase(int node_id) {
    if (node_id < 0 || node_id >= SKILL_NODE_COUNT) return 0;
    SkillNode *node = &g_skill_tree.nodes[node_id];

    if (node->purchased) return 0;
    if (!node->unlocked) return 0;
    if (g_game.scrap < node->cost) return 0;

    g_game.scrap -= node->cost;
    node->purchased = 1;

    // Unlock next node in same track
    if (node->tier < 2) {
        g_skill_tree.nodes[node_id + 1].unlocked = 1;
    }

    // Recompute total active bonuses
    g_skill_tree.bonus_firerate =
        (g_skill_tree.nodes[0].purchased ? 1 : 0) +
        (g_skill_tree.nodes[1].purchased ? 2 : 0) +
        (g_skill_tree.nodes[2].purchased ? 2 : 0);

    g_skill_tree.bonus_damage =
        (g_skill_tree.nodes[3].purchased ? 1 : 0) +
        (g_skill_tree.nodes[4].purchased ? 2 : 0) +
        (g_skill_tree.nodes[5].purchased ? 4 : 0);

    g_skill_tree.bonus_ap =
        (g_skill_tree.nodes[6].purchased ? 1 : 0) +
        (g_skill_tree.nodes[7].purchased ? 2 : 0) +
        (g_skill_tree.nodes[8].purchased ? 5 : 0);

    // Apply directly to Heavy Bolter battery
    int new_interval = 8 - g_skill_tree.bonus_firerate;
    if (new_interval < 3) new_interval = 3;
    g_turret.fire_interval = new_interval;

    return 1;
}

int skills_handle_touch(int touch_x, int touch_y) {
    // 1. Check tap on skill tree nodes
    for (int i = 0; i < SKILL_NODE_COUNT; i++) {
        SkillNode *n = &g_skill_tree.nodes[i];
        if (touch_x >= n->x && touch_x <= (n->x + n->w) &&
            touch_y >= n->y && touch_y <= (n->y + n->h)) {
            g_skill_tree.selected_node = i;
            return 0;
        }
    }

    // 2. Check tap on [SANTIFICAR] action button: (x: 160..248, y: 144..164)
    if (touch_x >= 160 && touch_x <= 248 && touch_y >= 144 && touch_y <= 164) {
        if (g_skill_tree.selected_node >= 0) {
            skills_try_purchase(g_skill_tree.selected_node);
        }
        return 0;
    }

    // 3. Check tap on [>> ENGAGE NEXT WAVE <<]: (x: 10..246, y: 168..188)
    if (touch_x >= 10 && touch_x <= 246 && touch_y >= 168 && touch_y <= 188) {
        return 1; // Signal to transition to wave mode
    }

    return 0;
}

void skills_draw_tree(void) {
    // 1. Screen Background
    renderer_fill_rect(0, 0, SCREEN_W, SCREEN_H, COLOR_DECK_FLOOR);

    // 2. Top Header Title & Tithe Pill: (y: 0..20)
    renderer_fill_rect(0, 0, SCREEN_W, 20, COLOR_IRON_PANEL);
    renderer_draw_line(0, 20, SCREEN_W - 1, 20, COLOR_IRON_BORDER);
    renderer_draw_text(8, 7, "++ OMNISSIAH ARSENAL FORGE ++", COLOR_BRASS);

    char tithe_buf[20];
    sprintf(tithe_buf, "TITHE: %d SC", g_game.scrap);
    renderer_fill_rect(170, 3, 80, 14, COLOR_HAZARD_BLACK);
    renderer_draw_rect(170, 3, 80, 14, COLOR_HAZARD_YELLOW);
    renderer_draw_text(176, 7, tithe_buf, COLOR_HAZARD_YELLOW);

    // 3. Draw Conduit Branches connecting Nodes
    // Root center: (34, 79)
    int rx = 34, ry = 79;
    
    // Conduits from Root to Tier 1 nodes
    int t0_col = g_skill_tree.nodes[0].purchased ? COLOR_BRASS :
                 (g_skill_tree.nodes[0].unlocked ? COLOR_HAZARD_YELLOW : COLOR_IRON_BORDER);
    int t1_col = g_skill_tree.nodes[3].purchased ? COLOR_BRASS :
                 (g_skill_tree.nodes[3].unlocked ? COLOR_HAZARD_YELLOW : COLOR_IRON_BORDER);
    int t2_col = g_skill_tree.nodes[6].purchased ? COLOR_BRASS :
                 (g_skill_tree.nodes[6].unlocked ? COLOR_HAZARD_YELLOW : COLOR_IRON_BORDER);

    // Root -> Track 0 (Cadencia: y=41)
    renderer_draw_line(rx + 16, ry, 66, 41, t0_col);
    renderer_draw_line(66, 41, 76, 41, t0_col);

    // Root -> Track 1 (Calibre: y=79)
    renderer_draw_line(rx + 16, ry, 76, ry, t1_col);

    // Root -> Track 2 (Penetracion: y=117)
    renderer_draw_line(rx + 16, ry, 66, 117, t2_col);
    renderer_draw_line(66, 117, 76, 117, t2_col);

    // Conduits between T1 -> T2 and T2 -> T3 along tracks
    for (int trk = 0; trk < SKILL_TRACK_COUNT; trk++) {
        int idx = trk * SKILL_TIERS_PER_TRACK;
        int cy = g_skill_tree.nodes[idx].y + (g_skill_tree.nodes[idx].h / 2);

        // T1 -> T2
        int c1_col = g_skill_tree.nodes[idx + 1].purchased ? COLOR_BRASS :
                     (g_skill_tree.nodes[idx + 1].unlocked ? COLOR_HAZARD_YELLOW : COLOR_IRON_BORDER);
        renderer_draw_line(124, cy, 136, cy, c1_col);

        // T2 -> T3
        int c2_col = g_skill_tree.nodes[idx + 2].purchased ? COLOR_BRASS :
                     (g_skill_tree.nodes[idx + 2].unlocked ? COLOR_HAZARD_YELLOW : COLOR_IRON_BORDER);
        renderer_draw_line(184, cy, 196, cy, c2_col);
    }

    // 4. Draw Root Node (Twin Bolter Core): (x: 10, y: 68, w: 42, h: 22)
    renderer_fill_rect(10, 68, 42, 22, COLOR_HAZARD_BLACK);
    renderer_draw_rect(10, 68, 42, 22, COLOR_BRASS);
    renderer_draw_text(14, 72, "BOLTER", COLOR_WHITE);
    renderer_draw_text(17, 80, "CORE", COLOR_BRASS);

    // 5. Draw the 9 Skill Nodes
    for (int i = 0; i < SKILL_NODE_COUNT; i++) {
        SkillNode *n = &g_skill_tree.nodes[i];
        int is_sel = (g_skill_tree.selected_node == i);

        uint16_t border_col;
        uint16_t text_col;
        uint16_t led_col;

        if (n->purchased) {
            border_col = COLOR_BRASS;
            text_col = COLOR_WHITE;
            led_col = COLOR_LED_GREEN;
        } else if (n->unlocked) {
            border_col = (g_game.scrap >= n->cost) ? COLOR_HAZARD_YELLOW : COLOR_AMBER;
            text_col = COLOR_HAZARD_YELLOW;
            led_col = (g_game.scrap >= n->cost) ? COLOR_LED_GREEN : COLOR_DARK_GRAY;
        } else {
            border_col = COLOR_IRON_BORDER;
            text_col = COLOR_DARK_GRAY;
            led_col = COLOR_BLACK;
        }

        renderer_fill_rect(n->x, n->y, n->w, n->h, COLOR_HAZARD_BLACK);
        renderer_draw_rect(n->x, n->y, n->w, n->h, border_col);

        if (is_sel) {
            // Glowing double border for selected node
            renderer_draw_rect(n->x - 1, n->y - 1, n->w + 2, n->h + 2, COLOR_WHITE);
        }

        // Status LED indicator
        renderer_fill_rect(n->x + 2, n->y + 2, 3, 3, led_col);

        // Code label
        renderer_draw_text(n->x + 8, n->y + 4, n->code, text_col);

        // Subtext / Cost
        char sub[12];
        if (n->purchased) {
            sprintf(sub, "[OK]");
            renderer_draw_text(n->x + 12, n->y + 13, sub, COLOR_LED_GREEN);
        } else {
            sprintf(sub, "%dSC", n->cost);
            renderer_draw_text(n->x + 8, n->y + 13, sub, (n->unlocked && g_game.scrap >= n->cost) ? COLOR_WHITE : text_col);
        }
    }

    // 6. Track Identifiers on canvas
    renderer_draw_text(80, 22, "> CADENCIA (ROF)", COLOR_IRON_LIGHT);
    renderer_draw_text(80, 60, "> CALIBRE & DANIO", COLOR_IRON_LIGHT);
    renderer_draw_text(80, 98, "> PERFORACION (AP)", COLOR_IRON_LIGHT);

    // 7. Bottom Inspection Panel & Actions: (y: 136..191)
    renderer_fill_rect(0, 134, SCREEN_W, 58, COLOR_IRON_PANEL);
    renderer_draw_line(0, 134, SCREEN_W - 1, 134, COLOR_IRON_BORDER);

    if (g_skill_tree.selected_node >= 0) {
        SkillNode *sn = &g_skill_tree.nodes[g_skill_tree.selected_node];

        char name_buf[32];
        sprintf(name_buf, "[%s] %s", sn->code, sn->name);
        renderer_draw_text(8, 138, name_buf, COLOR_WHITE);
        renderer_draw_text(8, 146, sn->desc, COLOR_HAZARD_YELLOW);

        // Action [SANTIFICAR] button: (x: 164..250, y: 137..154)
        if (sn->purchased) {
            renderer_fill_rect(164, 137, 86, 17, COLOR_HAZARD_BLACK);
            renderer_draw_rect(164, 137, 86, 17, COLOR_BRASS);
            renderer_fill_rect(167, 140, 4, 11, COLOR_LED_GREEN);
            renderer_draw_text(176, 142, "CONSAGRADO", COLOR_LED_GREEN);
        } else if (sn->unlocked) {
            int can_afford = (g_game.scrap >= sn->cost);
            uint16_t btn_col = can_afford ? COLOR_HAZARD_YELLOW : COLOR_IRON_LIGHT;
            renderer_fill_rect(164, 137, 86, 17, COLOR_HAZARD_BLACK);
            renderer_draw_rect(164, 137, 86, 17, btn_col);
            renderer_fill_rect(167, 140, 4, 11, can_afford ? COLOR_LED_GREEN : COLOR_LED_RED);

            char buy_lbl[20];
            sprintf(buy_lbl, "SANCTIFY %d", sn->cost);
            renderer_draw_text(174, 142, buy_lbl, btn_col);
        } else {
            renderer_fill_rect(164, 137, 86, 17, COLOR_HAZARD_BLACK);
            renderer_draw_rect(164, 137, 86, 17, COLOR_IRON_BORDER);
            renderer_draw_text(176, 142, "BLOQUEADO", COLOR_DARK_GRAY);
        }
    }

    // Bottom Action Bar: [>> ENGAGE NEXT WAVE <<] (x: 10..246, y: 159..184)
    renderer_fill_rect(10, 158, 236, 26, COLOR_HAZARD_BLACK);
    renderer_draw_rect(10, 158, 236, 26, COLOR_HAZARD_YELLOW);
    renderer_fill_rect(14, 162, 228, 18, COLOR_IRON_PANEL);
    renderer_draw_text(38, 168, ">> ENGAGE NEXT XENOS WAVE <<", COLOR_HAZARD_YELLOW);
}

const char *skills_get_active_doctrine_name(void) {
    if (g_skill_tree.nodes[2].purchased) return "STORM GATLING FURY";
    if (g_skill_tree.nodes[5].purchased) return "KRAKEN HEAVY SHELLS";
    if (g_skill_tree.nodes[8].purchased) return "PLASMA ARMOR MELT";
    if (g_skill_tree.nodes[1].purchased) return "HIGH-CYCLE SUPPRESS";
    if (g_skill_tree.nodes[4].purchased) return "MICRO-EXPLOSIVE TIPS";
    if (g_skill_tree.nodes[7].purchased) return "CORROSIVE PHOSPHORUS";
    if (g_skill_tree.nodes[0].purchased) return "DUAL-BELT SPEED";
    if (g_skill_tree.nodes[3].purchased) return "HIGH-DENSITY SHOTS";
    if (g_skill_tree.nodes[6].purchased) return "SABOT AP CARTRIDGE";
    return "STANDARD BOLTER RITE";
}
