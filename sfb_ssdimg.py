"""
Graphical SSDs: find the scanned SSD image for a ship and pair it with the
live damage picture.

assets/ssd/ holds two kinds of scan:
  * class-named files ('kzinti_cw.png', 'gorn_dreadnought_dn.png') - the LAST
    underscore token is the type code, the FIRST is the race. Auto-indexed.
  * raw book pages ('c1_p041.png', 'r2_p017.png') - unindexed until a human
    identifies them; record those in assets/ssd/index.json as
    {"Kzinti CV": "c1_p041.png", ...} and they take precedence.

Box-level damage overlay needs per-scan box coordinates we do not have, so the
damage layer is a banner (destroyed systems + shield state), not box crosses -
honest about what the data supports. Coordinates can be added per scan later.
"""
from __future__ import annotations

import json
import os

SSD_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), "assets", "ssd")

# race aliases used in scan filenames
_RACE_PREFIX = {
    "federation": "federation", "fed": "federation",
    "kzinti": "kzinti", "lyran": "lyran", "klingon": "klingon",
    "gorn": "gorn", "romulan": "romulan", "hydran": "hydran",
    "tholian": "tholian", "orion": "orion", "isc": "isc", "wyn": "wyn",
}

_INDEX = None


def _build_index():
    """{(race_lower, type_lower): path}. Manual index.json wins over filename
    auto-parse; page scans only ever enter via the manual index."""
    idx = {}
    try:
        names = os.listdir(SSD_DIR)
    except OSError:
        return idx
    for fn in names:
        if not fn.lower().endswith(".png"):
            continue
        stem = fn[:-4].lower()
        parts = stem.split("_")
        if len(parts) < 2 or (parts[0] in ("c1", "r1", "r2", "r3")
                              and parts[-1].startswith("p")):
            continue                        # raw page scan - manual index only
        race = None
        for alias, canon in _RACE_PREFIX.items():
            if parts[0] == alias:
                race = canon
                break
        if race is None:
            continue
        typ = parts[-1]
        if typ in ("tables", "pods", "fighters", "independent"):
            continue                        # aids, not a hull SSD
        idx[(race, typ)] = os.path.join(SSD_DIR, fn)
    # manual overrides / page assignments
    try:
        with open(os.path.join(SSD_DIR, "index.json"), encoding="utf-8") as f:
            manual = json.load(f)
        for key, fn in manual.items():
            bits = key.lower().split()
            if len(bits) >= 2:
                idx[(bits[0], bits[-1])] = os.path.join(SSD_DIR, fn)
    except OSError:
        pass
    return idx


def ssd_image_path(ship):
    """Path to the scanned SSD for this ship, or None (fall back to text SSD)."""
    global _INDEX
    if _INDEX is None:
        _INDEX = _build_index()
    race = str(ship.get("race") or "").lower()
    race = _RACE_PREFIX.get(race, race)
    typ = str(ship.get("type") or "").lower().replace("-", "_")
    hit = _INDEX.get((race, typ))
    if hit:
        return hit
    # a CVL scan is better than nothing for a CV, etc: same race, prefix match,
    # preferring the closest type code (CV -> CVL beats CV -> CVE only by
    # alphabetical luck; both are one letter off, so shortest-then-alpha).
    cands = [(len(t), t, p) for (r, t), p in _INDEX.items()
             if r == race and typ and (t.startswith(typ) or typ.startswith(t))]
    if cands:
        return min(cands)[2]
    return None


def refresh_index():
    """Drop the cache (call after editing index.json)."""
    global _INDEX
    _INDEX = None


def damage_banner(ship):
    """Compact damage lines to draw beside the scan: shields + destroyed boxes."""
    out = []
    sh = ship.get("shields") or []
    shm = ship.get("shields_max") or []
    if sh and shm:
        cells = []
        for i, (v, m) in enumerate(zip(sh, shm)):
            mark = "DOWN" if (m and v == 0) else f"{v}/{m}"
            cells.append(f"#{i + 1} {mark}")
        out.append("shields: " + "  ".join(cells))
    p = ship.get("power") or {}
    losses = []
    for label, cur, mx in (("warp", p.get("warp"), p.get("warp_max")),
                           ("impulse", p.get("impulse"), p.get("impulse_max")),
                           ("APR", p.get("apr"), p.get("apr_max")),
                           ("AWR", p.get("awr"), p.get("awr_max"))):
        if mx and cur is not None and cur < mx:
            losses.append(f"{label} {cur}/{mx}")
    hull = ship.get("hull") or []
    if len(hull) >= 2 and hull[0] < hull[1]:
        losses.append(f"hull {hull[0]}/{hull[1]}")
    if losses:
        out.append("DAMAGE: " + ", ".join(losses))
    return out
