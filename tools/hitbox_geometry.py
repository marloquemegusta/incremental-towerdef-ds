#!/usr/bin/env python3
"""Hitbox geometry audit for towerds.

Parses the enemy sprite definitions in source/enemy_data.c and, for every
variant/frame, computes how well the two stylus-hitbox schemes cover the
sprite that is actually drawn:

  OLD: circle r=24 px centred on the enemy's LOGICAL point (its feet). This is
       what shipped before the tap-hitbox fix.
  NEW: union of (sprite box inflated by TAP_HIT_MARGIN) and (circle
       TAP_HIT_RADIUS centred on the sprite's own centre).

Coordinates are relative to the logical point (0,0). Flying units are drawn
lifted by flight_altitude, which is applied here exactly as the renderer does.

Usage:
  python3 tools/hitbox_geometry.py source/enemy_data.c
"""
import re
import sys

TAP_HIT_MARGIN = 6
TAP_HIT_RADIUS = 24
FRAME_RE = re.compile(
    r"\.w\s*=\s*(-?\d+),\s*\.h\s*=\s*(-?\d+),\s*"
    r"\.offset_x\s*=\s*(-?\d+),\s*\.offset_y\s*=\s*(-?\d+),\s*"
    r"\.pixels\s*=\s*(\w+)"
)


def parse(path):
    variants = []
    lines = open(path).read().splitlines()
    cur = None
    for line in lines:
        if ".render_mode" in line:
            cur = {"is_flying": False, "altitude": 0, "frames": []}
            variants.append(cur)
        elif cur is not None and ".is_flying" in line:
            cur["is_flying"] = line.split("=")[1].strip().rstrip(",") == "1"
        elif cur is not None and ".flight_altitude" in line:
            cur["altitude"] = int(line.split("=")[1].strip().rstrip(","))
        m = FRAME_RE.search(line)
        if m and cur is not None:
            w, h, ox, oy, name = m.groups()
            cur["frames"].append({
                "w": int(w), "h": int(h), "ox": int(ox), "oy": int(oy),
                "name": name, "attack": "_att_" in name,
            })
    return variants


def uncovered(w, h, ox, oy, alt):
    """Count sprite pixels outside the OLD circle (r=24 at the logical point)."""
    y0 = oy - alt
    worst2 = 0
    n_out = 0
    for dy in range(y0, y0 + h):
        for dx in range(ox, ox + w):
            d2 = dx * dx + dy * dy
            if d2 > worst2:
                worst2 = d2
            if d2 > TAP_HIT_RADIUS * TAP_HIT_RADIUS:
                n_out += 1
    return n_out, worst2 ** 0.5


def main():
    path = sys.argv[1] if len(sys.argv) > 1 else "source/enemy_data.c"
    variants = parse(path)
    print(f"parsed {len(variants)} variants")
    print("variant,fly,alt,frames,max_pixels_outside_old,worst_dist_px,example")
    total_bad = 0
    for vi, v in enumerate(variants):
        worst_n, worst_name = 0, "-"
        max_d, max_d_name = 0.0, "-"
        for f in v["frames"]:
            n, d = uncovered(f["w"], f["h"], f["ox"], f["oy"], v["altitude"])
            if n > worst_n:
                worst_n, worst_name = n, f["name"]
            if d > max_d:
                max_d, max_d_name = d, f["name"]
        if worst_n > 0:
            total_bad += 1
        example = worst_name if worst_n > 0 else max_d_name
        print(f"{vi},{int(v['is_flying'])},{v['altitude']},{len(v['frames'])},"
              f"{worst_n},{max_d:.1f},{example}")
    print(f"variants_with_sprite_outside_old_circle={total_bad}/{len(variants)}")


if __name__ == "__main__":
    main()
