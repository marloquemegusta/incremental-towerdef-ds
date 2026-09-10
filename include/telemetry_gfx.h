#ifndef TELEMETRY_GFX_H
#define TELEMETRY_GFX_H

#include <nds.h>
#include <stdint.h>

#define TOP_TAB_BATTERIES 0
#define TOP_TAB_SWARM     1
#define TOP_TAB_FORGE     2

void telemetry_gfx_init(void);
void telemetry_gfx_update(int keys_down, int keys_held);
void telemetry_gfx_render(void);
void telemetry_gfx_present(void);

int telemetry_gfx_get_tab(void);
int telemetry_gfx_get_selected_turret(void);
int telemetry_gfx_is_detail_mode(void);

#endif // TELEMETRY_GFX_H
