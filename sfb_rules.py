"""
SFB rules engine — ported from the home-grown C++ interface (include/weapons.h,
include/ship.h) and docs/sfb_mechanics_reference.md into the online-driver
environment.

Pure rules/data: weapon damage tables, firing arcs, movement/turn-mode chart,
power/EAF model, and the Damage Allocation Chart. No I/O, no networking — the
tactical brain (sfb_brain.py) and the driver (sfb_client.py) consume this.

Geometry (hex distance / bearing / sextant) lives in sfb_hex.py; arc tests here
take a precomputed relative sextant so this module stays geometry-agnostic.
"""

from __future__ import annotations

from dataclasses import dataclass, field
from enum import Enum
from typing import Optional


# ==========================================================================
# Firing arcs  (weapons.h) — 6 sextants as a bitmask; sextant 0 = ship facing
# ==========================================================================

ARC_ALL   = 0b00111111   # 360°
ARC_FWD   = 0b00100011   # 180° forward  (sextants 5,0,1)
ARC_BROAD = 0b00110111   # 240° forward  (sextants 4,5,0,1,2)
ARC_REAR  = 0b00011100   # 180° aft      (sextants 2,3,4)


def in_arc(arc_mask: int, rel_sextant: int) -> bool:
    """rel_sextant = bearing to target relative to ship facing (0=dead ahead)."""
    return bool(arc_mask & (1 << (rel_sextant % 6)))


# ==========================================================================
# Weapons  (weapons.h)
# ==========================================================================

class WeaponType(Enum):
    PHASER1 = "Ph-1"
    PHASER2 = "Ph-2"
    PHASER3 = "Ph-3"
    PHOTON = "Photon"
    DISRUPTOR = "Disruptor"
    PLASMA_F = "Plasma-F"     # Gorn CA: 2-turn, fwd, launches at 20
    PLASMA_R = "Plasma-R"     # Romulan: 3-turn, fwd, launches at 50
    PLASMA_G = "Plasma-G"     # Gorn BC: 2-turn, fwd, launches at 20
    DRONE = "Drone"           # 1-turn, 360°, 6 (Cadet) at <=20
    HELLBORE = "Hellbore"     # Hydran: 1-turn, fwd, 12 <=5 / 8 <=10
    GATLING = "Gatling"       # Hydran: instant Ph-1 curve, high power
    FUSION = "Fusion"         # Lyran: instant, 10 <=3 / 5 <=6
    ESG = "ESG"               # Lyran: sphere, (5-radius)*energy
    FIGHTER = "Fighter"       # Hydran: seeking, 6 on hit


# Which row of the client's plasma table each torpedo type reads.
_PLASMA_KIND = {WeaponType.PLASMA_F: "F", WeaponType.PLASMA_R: "R",
                WeaponType.PLASMA_G: "G"}


# Phaser damage curves (weapons.h roll_damage) — indexed [range_row][die-1].
_PH1_TBL = [
    [10, 9, 8, 7, 6, 5],   # r 1-3
    [9, 7, 5, 4, 3, 1],    # r 4-8
    [5, 4, 3, 2, 1, 0],    # r 9-15
    [3, 2, 1, 0, 0, 0],    # r 16-24
    [0, 0, 0, 0, 0, 0],    # r 25+
]
_PH2_TBL = [
    [5, 4, 4, 3, 2, 1],    # r 1-3
    [3, 3, 2, 1, 0, 0],    # r 4-8
    [2, 1, 0, 0, 0, 0],    # r 9-15
    [0, 0, 0, 0, 0, 0],    # r 16+
]
_DISR_TBL = [
    [9, 8, 6, 5, 4, 1],    # r 1-8
    [5, 4, 3, 2, 1, 0],    # r 9-15
]

# Fusion beam (E7.31) — [die-1][range bracket]; brackets: r0, r1, r2, r3-10.
_FUSION_TBL = [
    [13, 8, 6, 4], [11, 8, 5, 3], [10, 7, 4, 2],
    [9, 6, 3, 1], [8, 5, 3, 1], [8, 4, 2, 0],
]
# Overloaded fusion (E7.41) — [die-1][range bracket]; brackets r0, r1, r2 only.
_FUSION_OL_TBL = [
    [19, 12, 9], [16, 12, 7], [15, 10, 6],
    [13, 9, 4], [12, 7, 4], [12, 6, 3],
]


def _fusion_bracket(r: int) -> int:
    return 0 if r == 0 else 1 if r == 1 else 2 if r == 2 else 3 if r <= 10 else -1


# Direct-fire hellbore (E10.7): 2d6 <= hit# to hit; base/overload dmg by bracket.
# (max_range, hit_number_on_2d6, base_damage, overload_damage)
_HELLBORE_DF = [(1, 11, 10, 15), (4, 10, 8, 12), (8, 9, 7, 11), (15, 8, 6, 9)]
# P(2d6 <= n) for n in 2..12
_P2D6_LE = {2: 1/36, 3: 3/36, 4: 6/36, 5: 10/36, 6: 15/36, 7: 21/36,
            8: 26/36, 9: 30/36, 10: 33/36, 11: 35/36, 12: 36/36}


def _ph1_row(r: int) -> int:
    return 0 if r <= 3 else 1 if r <= 8 else 2 if r <= 15 else 3 if r <= 24 else 4


@dataclass
class Weapon:
    label: str
    type: WeaponType
    arc: int
    max_power: int = 0      # phasers: max allocatable; others 0 (fixed cost)
    arming_turns: int = 0   # 0 = instant
    arming_cost: int = 0    # power per arming turn
    allocated: int = 0
    charge: int = 0
    armed: bool = False
    fired: bool = False
    disabled: bool = False   # knocked out by DAC
    ammo: int = -1           # -1 unlimited; drone rack = 4
    stored: int = 0          # ESG accumulated energy
    overloaded: bool = False # E3.5/E4.4: 2x cost, <=8 hex range cap, more damage

    def is_instant(self) -> bool:
        return self.arming_turns == 0

    def is_ready(self) -> bool:
        return self.armed or self.is_instant()

    def can_fire(self) -> bool:
        return (not self.disabled and self.is_ready() and not self.fired
                and (self.allocated > 0 or not self.is_instant())
                and self.ammo != 0)

    def phaser_damage(self, rng: int) -> int:
        d = float(self.allocated)
        if rng <= 3:
            return int(d)
        if rng <= 8:
            return int(d * 0.5)
        if rng <= 15:
            return int(d * 0.25)
        return 0

    def damage_at(self, rng: int) -> int:
        """Expected/planning damage (deterministic) — used by the AI."""
        t = self.type
        if t in (WeaponType.PHASER1, WeaponType.GATLING):
            return self.phaser_damage(rng)
        if t == WeaponType.PHASER2:
            pd = self.phaser_damage(rng)
            return pd // 2 + 1 if pd > 0 else 0
        if t == WeaponType.PHASER3:
            return self.allocated if rng <= 2 else 0
        if t == WeaponType.PHOTON:
            if self.overloaded:                    # E4.4: 16 dmg, <=8 hexes
                return 16 if 2 <= rng <= 8 else 0
            return 8 if 2 <= rng <= 30 else 0
        if t == WeaponType.DISRUPTOR:
            if self.overloaded:                    # E3.5: 2x, <=8 hexes
                return 12 if rng <= 8 else 0
            return 6 if rng <= 8 else 3 if rng <= 15 else 0
        # Plasma decays on a 14-bracket TABLE, not a straight line, and every one
        # of these linear formulas was wrong at launch strength: Plasma-R
        # launches at 50 (this said 30), Plasma-F at 20 (this capped it at 10),
        # Plasma-G at 20 (this said 10). Read the client's plasma table instead.
        if t in _PLASMA_KIND:
            try:
                import sfb_charts as CH
                row = CH.lookup(CH.PLASMA.get(_PLASMA_KIND[t]) or [], rng)
                if row is not None:
                    return int(row[1])
                return 0          # beyond the table's reach - the torpedo is spent
            except Exception:
                pass
            # Fallbacks keep the correct LAUNCH strengths even with no chart file.
            if t == WeaponType.PLASMA_R:
                return max(0, 50 - rng * 2)
            return max(0, 20 - rng)
        if t == WeaponType.DRONE:
            return 6 if rng <= 20 else 0
        if t == WeaponType.HELLBORE:
            # E10.7: 2d6<=hit#; enveloping hits the WEAKEST shield regardless of facing
            for maxr, hit, base, ol in _HELLBORE_DF:
                if rng <= maxr:
                    if self.overloaded and rng > 8:
                        return 0
                    return round(_P2D6_LE[hit] * (ol if self.overloaded else base))
            return 0
        if t == WeaponType.FUSION:
            tbl = _FUSION_OL_TBL if self.overloaded else _FUSION_TBL
            b = _fusion_bracket(rng)
            if b < 0 or (self.overloaded and b > 2):   # OL caps at range 2 (<=8 anyway)
                return 0
            b = min(b, len(tbl[0]) - 1)
            return sum(row[b] for row in tbl) // 6      # expected = mean over d6
        if t == WeaponType.ESG:
            total = self.stored + self.allocated
            return max(0, (5 - rng) * total) if rng <= 3 else 0
        if t == WeaponType.FIGHTER:
            return 6
        return 0

    # Which client chart each weapon rolls on. Phasers read a per-die cell;
    # heavy weapons read a to-hit number and a flat damage figure.
    _PHASER_CHART = {WeaponType.PHASER1: "Phaser1", WeaponType.GATLING: "Phaser3",
                     WeaponType.PHASER2: "Phaser2", WeaponType.PHASER3: "Phaser3"}
    _HEAVY_CHART = {WeaponType.PHOTON: "Photon", WeaponType.DISRUPTOR: "Disr"}

    def roll_damage(self, rng: int, die: int) -> int:
        """Dice-resolved actual damage (die 1-6). 0 = miss.

        Resolved against the CLIENT'S OWN charts (sfb_charts), not the hand-typed
        tables this used to carry. Those were wrong in ways that mattered: the
        Ph-2 table matched the real chart in no row at all, a ph-3 was cut off at
        range 2 when it reaches 15, and a disruptor at 15 when it reaches 40.
        E2.152: a gatling (ph-G) fires on the ph-3 chart.
        """
        t = self.type
        try:
            import sfb_charts as CH
        except Exception:
            CH = None

        if CH is not None and t in self._PHASER_CHART:
            key = self._PHASER_CHART[t]
            ch = CH.charts().get(key)
            if ch and ch["ranges"]:
                col = next((i for i, b in enumerate(ch["ranges"]) if rng <= b), None)
                if col is None:                      # beyond the chart's reach
                    return 0
                row = ch["rows"].get(str(max(1, min(6, die))))
                base = int(CH._num(row[col])) if row and col < len(row) else 0
                # A partially-energised phaser scales down (E2.31).
                full = 10 if key in ("Phaser1", "Phaser4") else 5
                if self.allocated >= full:
                    return base
                return (base * self.allocated + full - 1) // full

        if CH is not None and t in self._HEAVY_CHART:
            key = self._HEAVY_CHART[t]
            tbl = CH.DISR_OVL if (t == WeaponType.DISRUPTOR and self.overloaded) else \
                CH.heavy(key, ("DMG-STD",), ("STD/OLVD", "STD"))
            row = CH.lookup(tbl, rng)
            if row is None:
                return 0
            _bound, dmg, hit = row
            return int(dmg) if (hit and die <= hit) else 0

        return self.damage_at(rng)

    def eaf_cost(self) -> int:
        if self.is_instant():
            return self.allocated
        if self.armed:
            return 0
        return self.arming_cost if self.charge < self.arming_turns else 0


# Weapon factories (weapons.h make_*)
def make_ph1(l):       return Weapon(l, WeaponType.PHASER1, ARC_BROAD, 10)
def make_ph2(l):       return Weapon(l, WeaponType.PHASER2, ARC_BROAD, 5)
def make_ph3(l):       return Weapon(l, WeaponType.PHASER3, ARC_ALL, 2)
def make_photon(l):    return Weapon(l, WeaponType.PHOTON, ARC_FWD, 0, 2, 2)
def make_disruptor(l): return Weapon(l, WeaponType.DISRUPTOR, ARC_BROAD, 2)
def make_plasma_f(l):  return Weapon(l, WeaponType.PLASMA_F, ARC_FWD, 0, 2, 4)
def make_plasma_r(l):  return Weapon(l, WeaponType.PLASMA_R, ARC_FWD, 0, 3, 6)
def make_plasma_g(l):  return Weapon(l, WeaponType.PLASMA_G, ARC_FWD, 0, 2, 5)
def make_drone(l):     w = Weapon(l, WeaponType.DRONE, ARC_ALL, 0, 1, 2); w.ammo = 4; return w
def make_hellbore(l):  return Weapon(l, WeaponType.HELLBORE, ARC_FWD, 0, 1, 4)
def make_gatling(l):   return Weapon(l, WeaponType.GATLING, ARC_BROAD, 15)
def make_fusion(l):    return Weapon(l, WeaponType.FUSION, ARC_FWD, 0, 1, 3)
def make_esg(l):       return Weapon(l, WeaponType.ESG, ARC_ALL, 8)
def make_fighter(l):   return Weapon(l, WeaponType.FIGHTER, ARC_ALL, 0, 1, 2)


# ==========================================================================
# Movement  (ship.h, mechanics §2-3)
# ==========================================================================

# Impulse Movement Chart (C1.4) — VERIFIED 2026-07-19 against the SFU Online
# Client's own Impulse Chart tab, read directly off screen.
#
# The printed chart could not be extracted from any PDF (annex foldout, no text or
# table layer) nor found in the client jars, so it was verified against the live
# client instead. Columns 7, 8, 9, 18, 28, 31 and 32 were read at high zoom and
# match this formula exactly, impulse for impulse. IMPULSE_CHART_SAMPLE below
# preserves those readings as a regression test.
#
# The chart is an even (Bresenham-style) distribution after all: a ship at speed N
# moves on impulse i exactly when floor(N*i/32) > floor(N*(i-1)/32), which yields
# exactly N moves per 32-impulse turn.
IMPULSE_CHART_SAMPLE = {
    7:  [5, 10, 14, 19, 23, 28, 32],
    8:  [4, 8, 12, 16, 20, 24, 28, 32],
    9:  [4, 8, 11, 15, 18, 22, 25, 29, 32],
    18: [2, 4, 6, 8, 9, 11, 13, 15, 16, 18, 20, 22, 24, 25, 27, 29, 31, 32],
    28: [2, 3, 4, 5, 6, 7, 8, 10, 11, 12, 13, 14, 15, 16, 18, 19, 20, 21, 22, 23,
         24, 26, 27, 28, 29, 30, 31, 32],
    31: list(range(2, 33)),
    32: list(range(1, 33)),
}


def moves_this_impulse(speed: int, impulse: int) -> bool:
    """Impulse Movement Chart (C1.4): a ship at speed N moves in N of 32 impulses.

    Verified against the client's own chart — see IMPULSE_CHART_SAMPLE and
    verify_impulse_chart().
    """
    if speed <= 0 or impulse < 1 or impulse > 32:
        return False
    return (speed * impulse // 32) > (speed * (impulse - 1) // 32)


def verify_impulse_chart():
    """Regression test against the columns read off the live client chart.

    Returns (ok, failures). Kept in the module so any future change to the
    movement formula is checked against real observed data rather than intuition.
    """
    failures = []
    for speed, moves in IMPULSE_CHART_SAMPLE.items():
        calc = [i for i in range(1, 33) if moves_this_impulse(speed, i)]
        if calc != moves:
            failures.append((speed, moves, calc))
    for speed in range(1, 33):
        n = sum(moves_this_impulse(speed, i) for i in range(1, 33))
        if n != speed:
            failures.append((speed, f"expected {speed} moves", f"got {n}"))
    return (not failures), failures


# Turn Mode chart (C3.31), transcribed directly from the TURN MODE CHART table in
# the Master Rulebook (2012), page 22.
#
# The chart is laid out as SPEED BRACKETS per turn-mode VALUE, not the other way
# round. Read it as: for category D, Turn Mode 1 applies at speeds 2-4, Turn Mode
# 2 at 5-8, Turn Mode 3 at 9-12, Turn Mode 4 at 13-17, and so on.
#
# Verified against the rulebook's own worked example (C3.23): "the Federation CA
# has a Turn Mode (category) of D... A ship with a Turn Mode (category) of D would
# have a Turn Mode (in the proper sense) of three at a speed of nine." Speed 9
# falls in the 9-12 bracket -> Turn Mode 3. Correct.
#
# Each entry is the INCLUSIVE UPPER SPEED of the bracket for Turn Mode 1, 2, 3...
# in order; the final None means "this turn mode and above, at any higher speed".
# category index 0=AA, 1=A, 2=B, 3=C, 4=D, 5=E, 6=F
_TURN_MODE_BRACKETS = [
    [8, 16, 24, None],                    # AA
    [6, 12, 19, 26, None],                # A
    [5, 10, 15, 21, 28, None],            # B
    [4, 9, 14, 20, 27, None],             # C
    [4, 8, 12, 17, 24, None],             # D
    [3, 6, 10, 14, 20, 29, None],         # E
    [3, 5, 9, 13, 17, 23, 29, None],      # F
]

# Non-ship units read their own columns of the same chart.
_TURN_MODE_SEEKING = 1                    # seeking weapons: Turn Mode 1 at speeds 1-32
_TURN_MODE_SHUTTLE = [11, 23, None]       # shuttles/fighters: 1-11, 12-23, 24+

# Retained for callers that still reference it; no speed/category in the real
# chart makes a turn impossible, so nothing returns this any more.
TURN_IMPOSSIBLE = 99


def _bracket_lookup(brackets, speed: int) -> int:
    """Turn Mode = 1-based index of the first bracket whose upper bound covers speed."""
    for i, hi in enumerate(brackets):
        if hi is None or speed <= hi:
            return i + 1
    return len(brackets)


def turn_mode(speed: int, category: int) -> int:
    """Hexes that must be moved straight before a 60-degree turn is legal (C3.31).

    Sideslips count as straight-line movement for this purpose (C3.24).
    """
    if speed <= 1:
        # C3.0 governs units moving at trans-light speed (more than one hex per
        # turn); a unit at speed 0-1 is not constrained by a Turn Mode.
        return 0
    category = max(0, min(6, category))
    return _bracket_lookup(_TURN_MODE_BRACKETS[category], speed)


def turn_mode_seeking(speed: int = 0) -> int:
    """Seeking weapons (drones, plasma): Turn Mode 1 at every speed (C3.31)."""
    return _TURN_MODE_SEEKING


def turn_mode_shuttle(speed: int) -> int:
    """Shuttles and fighters: 1-11 -> TM1, 12-23 -> TM2, 24+ -> TM3 (C3.31)."""
    if speed <= 1:
        return 0
    return _bracket_lookup(_TURN_MODE_SHUTTLE, speed)


# Turn Mode categories for Basic Set ships, from the chart's own class list.
# Prefix key: F=Federation, K=Klingon, R=Romulan, G=Gorn, O=Orion, T=Tholian,
# Z=Kzinti. Useful as a cross-check when a save's turn_mode attribute is missing
# or looks wrong.
BASIC_SET_TURN_CATEGORY = {
    "A": {"K": ["F5", "E4"], "O": ["CR"], "R": ["KF5R"], "T": ["PC", "PC+"],
          "Z": ["FF", "EFF"]},
    "B": {"K": ["D6", "D7"], "R": ["KR"], "Z": ["CL"]},
    "C": {"F": ["CL", "DD", "SC"], "G": ["DD", "DDF"], "Z": ["CS", "BC", "CC"]},
    "D": {"F": ["CC", "CA", "Tug+P"], "G": ["CL", "CA", "BC"], "K": ["C8", "C9"],
          "R": ["WE"]},
    "E": {"F": ["DN", "BT", "Tug+2P"], "Z": ["CV", "CVS"]},
    "F": {"F": ["BT+P"]},
}


def max_accel(prev_speed: int) -> int:
    """C2.2: max speed *increase* per turn = max(prev_speed, 10). No decel limit."""
    return max(prev_speed, 10)


# ==========================================================================
# Damage Allocation Chart  (ship.h DACEntry, mechanics §12)
# ==========================================================================

class DACEntry(Enum):
    WARP_ENGINE = "warp"        # -3 power
    IMPULSE_ENGINE = "impulse"  # -2 power
    SHIELD = "shield"           # -5 to that shield's max
    WEAPON = "weapon"           # first live weapon disabled
    LAB = "lab"
    AUXILIARY = "aux"           # hull only
    APR = "apr"                 # -2 output
    BRIDGE = "bridge"           # crew -1, control loss


# The Cadet DAC that used to sit here has been REMOVED: it was the wrong chart
# for a full-rules game and had zero callers anywhere in the tree, so it could
# only ever mislead someone into wiring it up.
#
# The real chart is the client's own client_data/ship_dac.table. Its STRUCTURE is
# understood - 11 rows (a 2d6 roll of 2..12), 13 columns walked left to right as
# damage is scored, "Excess Damage" always last, and a per-cell flag of 1 meaning
# BOLD, i.e. D4.31's "a given BOLD result can only be scored ONE time in each
# volley".
#
# What is NOT established is the BOX-KIND MAPPING, and it must not be guessed.
# D4.31's worked example says a roll of 12 scores on "auxiliary control,
# emergency bridge, and scanners", but the table's roll-12 row begins 25, 26, 3,
# which both boxtypes.names and abbrev_boxtypes.names decode as Cargo, Shield,
# Scanner. Only the third matches. Until that is resolved the chart cannot be
# used for damage estimation without inventing numbers.
#
# To settle it empirically: apply a known volley to a throwaway ship in the
# client and read which boxes it checks off - the combat log records both
# ("Allocation of damage for: X" / "Damage: n/n/n"). That gives the mapping from
# observation rather than from a reading of an ambiguous file.


# ==========================================================================
# Power / EAF model  (ship.h EnergyAllocation, mechanics §2)
# ==========================================================================

# A2.0 life support(2) + A5.0 fire control(2) + D3.32 shield maint(2)
MANDATORY_POWER = 6

SHIELD_NAMES = ["Fwd", "FwdR", "AftR", "Aft", "AftL", "FwdL"]  # #1..#6


@dataclass
class EnergyAllocation:
    warp_power: int = 0
    impulse_power: int = 0
    apr_output: int = 0        # APR cannot fund movement
    battery_tap: int = 0
    cloak: int = 0
    speed: int = 0
    reinforce: list = field(default_factory=lambda: [0] * 6)
    general_reinforce: int = 0
    ecm: int = 0
    eccm: int = 0
    repair: int = 0
    tractors: int = 0
    transporters: int = 0

    def movement_power(self) -> int:
        """C2.12: only warp + impulse + battery may fund movement (not APR)."""
        return self.warp_power + self.impulse_power + self.battery_tap


# ==========================================================================
# Key constants  (mechanics §18)
# ==========================================================================

IMPULSES_PER_TURN = 32
MAX_SPEED = 31
MAX_WARP_MP = 30

# Energy cost chart (Master Rulebook: D3.32 shields, B-series life support, H/G).
# Keyed by ship SIZE CLASS (1=base .. 5=PF). Per turn.
# D3.32 shield operation cost by size class, MEASURED against the client's own
# energy allocation form:
#     SC2  LDR DN "Lion"    -> 4
#     SC3  Lyran CA "Tiger" -> 2
#
# These values were once changed to {2: 5, 3: 4} on the strength of a rules audit
# that read the D3.32 chart out of a pdftotext extract. That extract is column-
# garbled, and the FULL/TOTAL columns are displaced UP BY ONE ROW: the line that
# appears as "3 (Cruisers) =1 +3 =4" is in fact SC2's total, which is why the DN
# measures 4. Reading it literally produced a table that was wrong in two rows
# and over-booked every cruiser by 2 points a turn.
#
# Do not "correct" these against the text extract again. If they ever need
# revisiting, measure them on the client's EAF - a hull of the size class in
# question, and read the shield allocation line.
#
# SC4/SC5 = 1 is consistent with D3.321: "when not using fractional accounting
# (B3.2), the cost of operation for size class 4 or 5 is 1 for minimum and +0 for
# full", and with the printed fractional figures 0.5 + 0.5 = 1.
SHIELD_COST = {1: 7, 2: 4, 3: 2, 4: 1, 5: 1}

# D3.33 minimum shields only - a deliberate economy to be offered, not the default.
SHIELD_MIN_COST = {1: 2, 2: 1, 3: 1, 4: 0.5, 5: 0.5}
LIFE_SUPPORT_COST = {1: 3, 2: 1.5, 3: 1, 4: 0.5, 5: 0.0}
FIRE_CONTROL_COST = 1          # active; LPFC = 0.5 (D6.71)
PHASER_ENERGIZE = 1            # E2.31: 1 pt energizes all phasers this turn
TRACTOR_COST = 1              # per beam (G7.15); x2 at r2, x3 at r3
TRANSPORTER_COST = 0.2       # 1 pt runs up to 5 transporters (G8.13)

# Overload (E3.5/E4.4/E7.41/E10.6): 8-hex TRUE-range cap; point-blank feedback.
OVERLOAD_RANGE_CAP = 8
OVERLOAD_ARMING = {          # total energy to arm as overload
    "Disruptor": 4, "Photon": 8, "Fusion": 4, "Hellbore": 6,
}
OVERLOAD_FEEDBACK = {        # damage to firer's own shield if fired at true range 0
    "Disruptor": 2, "Photon": 4, "Hellbore": 3,   # hellbore enveloping = 1/shield
}

# High Energy Turn (C6.0): costs 5 hexes of WARP movement; breakdown roll (C6.5).
HET_COST_HEXES = 5

# Erratic Maneuvers (C10.0): 6 hexes of movement; yields 4 ECM (+2 shift), TM +1.
EM_COST_HEXES = 6
EM_ECM = 4
DRONE_SPEED = 32
DRONE_WARHEAD_CADET = 6
DRONE_WARHEAD_STD = 12
DRONE_DESTROY_THRESHOLD = 4
PHOTON_MIN_RANGE = 2
PHOTON_MAX_RANGE = 30
PHOTON_DAMAGE = 8
PHOTON_DAMAGE_OVERLOAD = 16
