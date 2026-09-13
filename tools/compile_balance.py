#!/usr/bin/env python3
"""Validate canonical balance CSVs and emit towerds_balance.bin."""
import argparse, csv, struct
from pathlib import Path

MAGIC = 0x544F5744
WAVE_FIELDS = ("count", "delay", "speed", "hp")

def ints(row, fields, label):
    try: return [int(row[f]) for f in fields]
    except (KeyError, TypeError, ValueError) as exc:
        raise ValueError(f"{label}: valores enteros inválidos") from exc

def compile_balance(root, output):
    cfg = root / "config" / "balance"
    with (cfg / "waves.csv").open(newline="", encoding="utf-8-sig") as f:
        waves = list(csv.DictReader(f))
    if len(waves) != 20 or [int(r["wave"]) for r in waves] != list(range(1, 21)):
        raise ValueError("waves.csv debe contener exactamente las oleadas 1..20")
    blob = bytearray()
    for i, row in enumerate(waves, 1):
        values = []
        for enemy in ("larva", "ripper", "hormagaunt"):
            values += ints(row, [f"{enemy}_{x}" for x in WAVE_FIELDS], f"wave {i} {enemy}")
        values += ints(row, ["scrap_base"], f"wave {i} scrap_base")
        if any(v < 0 for v in values) or any(values[j] == 0 and j % 4 in (1, 2, 3) for j in range(12)):
            raise ValueError(f"wave {i}: valores negativos o configuración inválida")
        blob += struct.pack("<12iQ", *values[:12], values[12])
    with (cfg / "enemies.csv").open(newline="", encoding="utf-8-sig") as f:
        enemies = list(csv.DictReader(f))
    if len(enemies) != 6 or [int(r["enemy_id"]) for r in enemies] != list(range(6)):
        raise ValueError("enemies.csv debe contener los enemigos 0..5")
    blob += struct.pack("<6I", *(int(r["default_hp"]) for r in enemies))
    blob += struct.pack("<6I", *(int(r["scrap_value"]) for r in enemies))

    with (cfg / "upgrades.csv").open(newline="", encoding="utf-8-sig") as f:
        upgrades = list(csv.DictReader(f))
    if len(upgrades) != 7 or [int(r["upgrade_id"]) for r in upgrades] != list(range(7)):
        raise ValueError("upgrades.csv debe contener las mejoras 0..6")
    costs = []
    for row in upgrades:
        costs.extend(int(row[f"cost_{i}"] or 0) for i in range(5))
    blob += struct.pack("<35Q", *costs)
    blob += struct.pack("<5i", 2, 3, 4, 6, 8)
    blob += struct.pack("<5i", 18, 14, 10, 7, 5)
    blob += struct.pack("<5i", 65, 80, 100, 125, 150)
    blob += struct.pack("<6i", 20, 35, 50, 70, 100, 150)
    blob += struct.pack("<4i", 100, 1800, 10, 5)
    blob += struct.pack("<6i", 1, 3, 5, 7, 9, 11)
    blob += struct.pack("<6i", 45, 45, 45, 45, 45, 45)
    blob += struct.pack("<4i", 9999, 60, 20, 10)
    # Optional tiers 3..5 for each wave: zero by default, ready for editing.
    blob += struct.pack("<960i", *([0] * 960))
    blob += struct.pack("<I", MAGIC)
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_bytes(blob)
    print(f"OK: {len(waves)} oleadas, {len(blob)} bytes -> {output}")

if __name__ == "__main__":
    p = argparse.ArgumentParser()
    p.add_argument("--root", type=Path, default=Path(__file__).resolve().parents[1])
    p.add_argument("--output", type=Path, default=Path("towerds_balance.bin"))
    a = p.parse_args(); compile_balance(a.root, a.output)
