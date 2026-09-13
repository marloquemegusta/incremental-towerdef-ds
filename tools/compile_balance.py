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
    blob += struct.pack("<I", MAGIC)
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_bytes(blob)
    print(f"OK: {len(waves)} oleadas, {len(blob)} bytes -> {output}")

if __name__ == "__main__":
    p = argparse.ArgumentParser()
    p.add_argument("--root", type=Path, default=Path(__file__).resolve().parents[1])
    p.add_argument("--output", type=Path, default=Path("towerds_balance.bin"))
    a = p.parse_args(); compile_balance(a.root, a.output)
