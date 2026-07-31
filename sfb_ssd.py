"""
SSD parser — turns the game's decoded SSD blob (gub.plugins.game.sfb.AbbrevSSD)
into structured ship data the tactics can use: per-facing shields, hull total,
and a best-effort weapon list.

Box-"kind" mapping (confirmed from captured SSDs + SSD.class category checks):
  26            -> shields (6 groups, designations 1..6 = facings #1..#6)
  28            -> photon torpedo tubes (forward)
  33, 35, 45    -> phasers (designations are phaser numbers; firing arc per box)
  category 3    -> drone rack ; category 2/16 -> plasma ; category 13 -> plasma rack

Shields are the load-bearing, fully-reliable extraction. Weapon identification is
best-effort by kind; unknown weapon boxes are ignored (the brain keeps faction
default weapons when the SSD yields none).
"""

from __future__ import annotations
from dataclasses import dataclass, field
from typing import Optional

from sfb_rules import (Weapon, make_ph1, make_photon, make_drone, make_plasma_r,
                       ARC_BROAD, ARC_FWD, ARC_ALL)

KIND_SHIELD = 26
KIND_PHOTON = 28
KIND_PHASER = {33, 35, 45}


@dataclass
class ParsedSSD:
    title: str = ""
    shields: list = field(default_factory=lambda: [0] * 6)      # current (intact)
    shields_max: list = field(default_factory=lambda: [0] * 6)
    hull: int = 0
    hull_max: int = 0
    weapons: list = field(default_factory=list)


def _boxes(ssd: dict) -> list:
    b = ssd.get("boxes")
    if isinstance(b, dict):
        return b.get("@items", []) or []
    if isinstance(b, list):
        return b
    return []


def _intact(box: dict) -> int:
    st = box.get("boxStatus") or []
    return sum(1 for s in st if s)


def _designations(box: dict) -> list:
    d = box.get("designations")
    if isinstance(d, dict):
        return d.get("@items", []) or []
    if isinstance(d, list):
        return d
    return []


def parse_ssd(ssd: dict) -> ParsedSSD:
    out = ParsedSSD(title=ssd.get("title", ""))
    if not isinstance(ssd, dict):
        return out

    hull_intact = hull_total = 0
    for box in _boxes(ssd):
        kind = box.get("kind")
        n = box.get("maxNumOfBoxes", 0) or 0
        intact = _intact(box)

        if kind == KIND_SHIELD:
            # designation "1".."6" -> facing index 0..5
            desig = _designations(box)
            try:
                fi = int(str(desig[0])) - 1 if desig else None
            except (ValueError, IndexError):
                fi = None
            if fi is not None and 0 <= fi < 6:
                out.shields_max[fi] = n
                out.shields[fi] = intact
            continue

        if kind == KIND_PHOTON:
            for lbl in (_designations(box) or [f"P{i+1}" for i in range(n)]):
                out.weapons.append(make_photon(f"Photon {lbl}"))
            continue

        if kind in KIND_PHASER:
            for lbl in (_designations(box) or [str(i + 1) for i in range(n)]):
                out.weapons.append(make_ph1(f"Ph {lbl}"))
            continue

        # everything else counts toward hull integrity (engines, hull, systems)
        hull_total += n
        hull_intact += intact

    out.hull = hull_intact or 1
    out.hull_max = hull_total or 1
    return out


def ssd_available(ssd) -> bool:
    return isinstance(ssd, dict) and bool(_boxes(ssd))
