"""
Shadow-state layer - Phase 3: COMBAT RESOLUTION primitives.

The dice-and-tables layer the ADVISOR never needed (the client rolled), but the
shadow does - both to CHECK the client (referee: is an observed volley legal?)
and, later, to REPLACE it (engine: roll the volley ourselves).

Two faces, deliberately separate:

  * BOUNDS (deterministic) - the min/max a weapon CAN do at a range, from the
    client's own chart. The referee uses these: observed damage that exceeds the
    chart maximum is impossible, so a rule or a read is wrong. No dice involved,
    so nothing to be lucky about.

  * ROLLS (seeded) - actually resolve a volley with a reproducible RNG. The
    engine uses these. Seedable so a shadow run is repeatable and a divergence
    is diagnosable rather than a dice artifact.

Charts come from sfb_charts (parsed from the client's data files); the DAC comes
from client_data/ship_dac.table. Nothing here is transcribed by hand.
"""
from __future__ import annotations

import os
import sfb_charts as CH

# ---------------------------------------------------------- weapon name mapping
# The combat log names weapons loosely ('phaser-1', 'disruptor'); the charts key
# them tightly ('Phaser1', 'Disr'). One normaliser, so every caller agrees.
_WEAPON_KEY = {
    "phaser-1": "Phaser1", "phaser1": "Phaser1", "ph-1": "Phaser1", "ph1": "Phaser1",
    "phaser-2": "Phaser2", "phaser2": "Phaser2", "ph-2": "Phaser2", "ph2": "Phaser2",
    "phaser-3": "Phaser3", "phaser3": "Phaser3", "ph-3": "Phaser3", "ph3": "Phaser3",
    "phaser-4": "Phaser4", "phaser4": "Phaser4", "ph-4": "Phaser4", "ph4": "Phaser4",
    "phaser": "Phaser1", "phasers": "Phaser1",
    "disruptor": "Disr", "disr": "Disr", "disruptors": "Disr",
    "photon": "Photon", "photons": "Photon",
    "fusion": "Fusion", "fusionol": "FusionOL", "fusion-ol": "FusionOL",
    "hellbore": "Hellbore", "esg": "ESG",
}

PHASER_KEYS = {"Phaser1", "Phaser2", "Phaser3", "Phaser4"}
# Heavy weapons roll a to-hit, then apply a fixed damage-per-hit from the chart.
HEAVY_KEYS = {"Disr", "Photon", "Fusion", "FusionOL", "Hellbore"}


def weapon_key(name):
    """Chart key for a log/weapon token, or None if we do not chart it."""
    if not name:
        return None
    n = str(name).strip().lower()
    if n in _WEAPON_KEY:
        return _WEAPON_KEY[n]
    # tolerate a trailing plural or stray punctuation
    n2 = n.rstrip("s").replace(" ", "")
    return _WEAPON_KEY.get(n2)


def _bracket(ranges, rng):
    """Index of the first bracket whose upper bound >= rng, or None if past max."""
    for i, bound in enumerate(ranges):
        if rng <= bound:
            return i
    return None


# --------------------------------------------------------------------- BOUNDS
def phaser_bounds(key, rng):
    """(min, max) damage a single phaser of this type can roll at range rng.

    (0, 0) when the range is off the end of the chart - the phaser cannot reach.
    """
    ch = CH.charts().get(key)
    if not ch or not ch["ranges"]:
        return (0.0, 0.0)
    i = _bracket(ch["ranges"], rng)
    if i is None:
        return (0.0, 0.0)
    vals = []
    for d in range(1, 7):
        row = ch["rows"].get(str(d))
        if row and i < len(row):
            vals.append(CH._num(row[i]))
    if not vals:
        return (0.0, 0.0)
    return (min(vals), max(vals))


def heavy_damage(key, rng):
    """(damage_if_hit, hit_number) for a heavy weapon at range rng, from DMG-STD.

    damage 0 means out of range. hit_number is the d6 span (or d20 target for
    the hellbore) the weapon hits on.
    """
    for bound, dmg, hit in CH.heavy(key):
        if rng <= bound:
            return (dmg, hit)
    return (0.0, 0)


def max_single_damage(key, rng):
    """The most damage ONE weapon of this type can do at range rng (0 if it
    cannot reach). Phasers take the top chart roll; heavies their fixed damage."""
    if key in PHASER_KEYS:
        return phaser_bounds(key, rng)[1]
    if key in HEAVY_KEYS:
        return heavy_damage(key, rng)[0]
    # Unknown-but-charted weapon: fall back to the max over any numeric cell.
    ch = CH.charts().get(key)
    if not ch or not ch["ranges"]:
        return 0.0
    i = _bracket(ch["ranges"], rng)
    if i is None:
        return 0.0
    best = 0.0
    for row in ch["rows"].values():
        if i < len(row):
            best = max(best, CH._num(row[i]))
    return best


def volley_max(key, count, rng):
    """Upper bound on total damage from `count` weapons of one type at range."""
    return max_single_damage(key, rng) * max(0, int(count or 0))


def absolute_max(key, rng):
    """The most ONE weapon can do at range rng in ANY firing mode.

    For heavies this scans every DMG-* row (standard, overload, ...), not just
    DMG-STD - needed because the log's mode tag is unreliable: the live game
    logged 'Standard mode' for a volley whose damage (6/hit at range 8) is only
    on the DMG-OVLD row. A legality bound must not trust the tag.
    """
    if key in PHASER_KEYS:
        return phaser_bounds(key, rng)[1]
    ch = CH.charts().get(key)
    if not ch or not ch["ranges"]:
        return 0.0
    i = _bracket(ch["ranges"], rng)
    if i is None:
        return 0.0
    best = 0.0
    for label, row in ch["rows"].items():
        if not label.startswith("DMG"):
            continue
        if i < len(row):
            best = max(best, CH._num(row[i]))
    if best:
        return best
    return max_single_damage(key, rng)


def volley_absolute_max(key, count, rng):
    """Mode-agnostic upper bound for a volley - the legality-check bound."""
    return absolute_max(key, rng) * max(0, int(count or 0))


# Firing modes for heavy weapons: mode -> (damage row names, to-hit row names).
HEAVY_MODES = {
    "STD":  (("DMG-STD",), ("STD", "UIM", "DERFACS")),
    "OVLD": (("DMG-OVLD",), ("OVLD", "UIM-OVLD")),
}


def heavy_mode_damage(key, rng, mode):
    """(damage_if_hit, hit_number) for a heavy weapon at range rng in a MODE.

    Used by exact-outcome replay: with the client's actual to-hit rolls in hand,
    hits x this damage must equal the logged total for the true firing mode.
    """
    rows = HEAVY_MODES.get(mode)
    if not rows:
        return (0.0, 0)
    for bound, dmg, hit in CH.heavy(key, dmg_rows=rows[0], hit_rows=rows[1]):
        if rng <= bound:
            return (dmg, hit)
    return (0.0, 0)


def max_range(key):
    """Longest range at which this weapon still does damage, or 0."""
    ch = CH.charts().get(key)
    if not ch or not ch["ranges"]:
        return 0
    reach = 0
    for i, bound in enumerate(ch["ranges"]):
        for row in ch["rows"].values():
            if i < len(row) and CH._num(row[i]) > 0:
                reach = bound
                break
    return reach


# ------------------------------------------------------------------ SEEDED RNG
class Roller:
    """A tiny reproducible RNG (SplitMix64). Seedable so a shadow combat run is
    repeatable - a divergence is then a rules bug, never a dice artifact.

    Deliberately NOT Python's global random: the shadow must be able to replay
    a turn byte-for-byte independent of anything else touching the RNG.
    """
    _MASK = (1 << 64) - 1

    def __init__(self, seed=0):
        self._s = int(seed) & self._MASK

    def _next(self):
        self._s = (self._s + 0x9E3779B97F4A7C15) & self._MASK
        z = self._s
        z = ((z ^ (z >> 30)) * 0xBF58476D1CE4E5B9) & self._MASK
        z = ((z ^ (z >> 27)) * 0x94D049BB133111EB) & self._MASK
        return (z ^ (z >> 31)) & self._MASK

    def die(self, sides):
        return int(self._next() % sides) + 1

    def d6(self):
        return self.die(6)

    def d20(self):
        return self.die(20)


def phaser_roll(key, rng, roller):
    """Damage from ONE phaser at range rng on a fresh d6 (engine mode)."""
    ch = CH.charts().get(key)
    if not ch or not ch["ranges"]:
        return 0.0
    i = _bracket(ch["ranges"], rng)
    if i is None:
        return 0.0
    row = ch["rows"].get(str(roller.d6()))
    return CH._num(row[i]) if row and i < len(row) else 0.0


def heavy_roll(key, rng, roller):
    """(hit, damage) for ONE heavy weapon at range rng (engine mode)."""
    dmg, hit = heavy_damage(key, rng)
    if dmg <= 0 or hit <= 0:
        return (False, 0.0)
    if key in getattr(CH, "D20_HIT_WEAPONS", ()):    # hellbore: d20 target
        return (roller.d20() <= hit, dmg)
    return (roller.d6() <= hit, dmg)


def resolve_volley(key, count, rng, roller):
    """Total damage from `count` weapons of one type at range (engine mode).

    Phasers roll per shot; heavies roll to-hit per shot then apply fixed damage.
    Returns total damage delivered to the target's facing shield (the caller
    applies shield/DAC).
    """
    count = max(0, int(count or 0))
    total = 0.0
    if key in PHASER_KEYS:
        for _ in range(count):
            total += phaser_roll(key, rng, roller)
    elif key in HEAVY_KEYS:
        for _ in range(count):
            hit, dmg = heavy_roll(key, rng, roller)
            if hit:
                total += dmg
    else:
        for _ in range(count):
            total += max_single_damage(key, rng)     # deterministic fallback
    return total


# ------------------------------------------------------------------------- DAC
# Damage Allocation Chart, client_data/ship_dac.table. Each data row is a 2d6
# outcome; the 13 columns are the hit-location tracks the client walks. Cells are
# (boxtype-index, flag) pairs; boxtype-index maps into boxtypes.names. This is
# ENGINE-facing (rolling our own internals): the combat log reports damage TOTALS
# and by-shield, never which internal boxes were struck, so there is nothing on
# the client side to reconcile box-type against - it is here for standalone play,
# not for the referee.
_DAC_ROWS = None
_BOXTYPES = None


def _load_dac():
    global _DAC_ROWS
    if _DAC_ROWS is not None:
        return _DAC_ROWS
    path = os.path.join(os.path.dirname(os.path.abspath(__file__)),
                        "client_data", "ship_dac.table")
    rows = []
    try:
        with open(path, encoding="utf-8", errors="replace") as f:
            for line in f:
                cells = [c for c in line.strip().split(",") if c != ""]
                if len(cells) < 4:
                    continue                # skip the format marker / short lines
                nums = [int(c) for c in cells if _is_int(c)]
                pairs = [(nums[i], nums[i + 1]) for i in range(0, len(nums) - 1, 2)]
                if pairs:
                    rows.append(pairs)
    except OSError:
        rows = []
    _DAC_ROWS = rows
    return rows


def _is_int(s):
    try:
        int(s)
        return True
    except ValueError:
        return False


def _load_boxtypes():
    global _BOXTYPES
    if _BOXTYPES is not None:
        return _BOXTYPES
    path = os.path.join(os.path.dirname(os.path.abspath(__file__)),
                        "client_data", "boxtypes.names")
    names = []
    try:
        with open(path, encoding="utf-8", errors="replace") as f:
            names = [ln.strip() for ln in f if ln.strip()]
    except OSError:
        names = []
    _BOXTYPES = names
    return names


def dac_hit(column, roller):
    """(boxtype_index, boxtype_name) for a 2d6 hit on the given DAC column.

    column selects the hit-location track (0-based). Engine-facing; see the note
    on _DAC_ROWS. Returns (None, None) if the table could not be loaded.
    """
    rows = _load_dac()
    if not rows:
        return (None, None)
    roll = roller.d6() + roller.d6()          # 2..12
    idx = min(len(rows) - 1, max(0, roll - 2))
    row = rows[idx]
    col = column % len(row) if row else 0
    boxtype = row[col][0] if row else None
    names = _load_boxtypes()
    name = names[boxtype] if boxtype is not None and 0 <= boxtype < len(names) else None
    return (boxtype, name)
