"""
Sophisticated tactical engine — applies the Captain's-Tactics-Manual doctrine
(sfb_doctrine) plus the EW model (sfb_ew) and hard damage tables (sfb_rules) to a
concrete position, producing both ADVICE (for a human) and CONTROL decisions
(for the AI).

Consumes sfb_tactics.TacticalShip (pos, facing, speed, hull, power, shields[6],
weapons[], faction). Everything here is deterministic analysis; the Claude layer
in sfb_brain turns Assessments into bridge dialogue or move commands.
"""

from __future__ import annotations
from dataclasses import dataclass, field
from typing import Optional

import sfb_hex as H
import sfb_ew as EW
import sfb_rules as R
import sfb_doctrine as DOC

SHIELD_NAMES = ["#1 fwd", "#2 fwd-R", "#3 aft-R", "#4 aft", "#5 aft-L", "#6 fwd-L"]


# ==========================================================================
# Firing analysis
# ==========================================================================

@dataclass
class FireSolution:
    weapon: object
    target_id: str
    rng: int
    target_shield: int          # 0-5, which of the target's shields faces us
    expected: float             # EW-adjusted expected damage
    overload: bool = False
    note: str = ""


def _fam(w, rng):
    return EW.weapon_family(w.type.value, rng)


def firing_solutions(ship, target, ew_shift: int = 0) -> list:
    """Every weapon that bears on `target`, with EW-adjusted expected damage."""
    rng = H.hex_distance(ship.pos, target.pos)
    shield = H.shield_hit(target.pos, target.facing, ship.pos)
    sols = []
    for w in ship.weapons:
        if w.disabled or w.fired:
            continue
        if not H.target_in_arc(ship.pos, ship.facing, w.arc, target.pos):
            continue
        base = w.damage_at(rng)
        if base <= 0:
            continue
        exp = EW.degrade_expected(base, _fam(w, rng), ew_shift)
        sols.append(FireSolution(w, target.obj_id, rng, shield, exp))
    sols.sort(key=lambda s: -s.expected)
    return sols


def alpha_strike(ship, target, ew_shift: int = 0) -> float:
    """Max single-impulse expected damage on target (all bearing weapons)."""
    return sum(s.expected for s in firing_solutions(ship, target, ew_shift))


def power_curve(ship) -> float:
    """Rough surplus-power measure (>1 good). power vs weapons+housekeeping demand."""
    demand = R.MANDATORY_POWER + sum(max(w.max_power, w.arming_cost) for w in ship.weapons)
    return ship.power / max(1, demand)


# ==========================================================================
# Shield / Mizia analysis
# ==========================================================================

def shield_report(target) -> list:
    """(facing, current, max, is_down) for each shield."""
    out = []
    for i in range(6):
        cur = target.shields[i] if i < len(target.shields) else 0
        mx = target.shields_max[i] if i < len(target.shields_max) else 0
        out.append((i, cur, mx, cur == 0 and mx > 0))
    return out


def facing_shield_is_down(ship, target) -> bool:
    s = H.shield_hit(target.pos, target.facing, ship.pos)
    cur = target.shields[s] if s < len(target.shields) else 1
    return cur == 0


def mizia_recommendation(ship, target) -> str:
    """Whether to concentrate mini-volleys (Mizia) or deliver one massive volley."""
    if facing_shield_is_down(ship, target):
        return ("MIZIA: facing shield is DOWN — pour several small volleys onto it "
                "(more weapon hits, cripples his guns). Split direct-fire + seeking + "
                "T-bomb into separate volleys on the same impulse where possible.")
    # shield still up: need to knock it down first, then a massed volley is fine
    s = H.shield_hit(target.pos, target.facing, ship.pos)
    cur = target.shields[s] if s < len(target.shields) else 0
    return (f"Facing shield {SHIELD_NAMES[s]} still up ({cur}). Knock it down first "
            f"(massed phasers on the approach), THEN concentrate on the down shield.")


# ==========================================================================
# Electronic warfare decision
# ==========================================================================

def recommend_ew(ship, enemy, threatened: bool) -> dict:
    """Split the 6-pt sensor pool into ECM/ECCM given the matchup. Doctrine:
    1 reserve ECM can save ~10 shield reinforcement."""
    enemy_ecm = getattr(enemy, "ecm", 0) or 0
    # match enemy ECM with ECCM (to clear our shots), keep rest as ECM (to blunt his)
    eccm = min(EW.SENSOR_EW_POOL, enemy_ecm) if enemy_ecm else 0
    ecm = EW.SENSOR_EW_POOL - eccm if threatened else max(0, (EW.SENSOR_EW_POOL - eccm) // 2)
    return {"ecm": ecm, "eccm": eccm,
            "note": f"ECCM {eccm} to clear his +{EW.points_to_shift(enemy_ecm)} shift; "
                    f"ECM {ecm} to shift his fire. {DOC.RESERVE_EFFICIENCY}"}


# ==========================================================================
# Overload / HET decisions
# ==========================================================================

def should_overload(ship, target) -> tuple:
    rng = H.hex_distance(ship.pos, target.pos)
    if rng > 8:
        return (False, "range >8: overloads can't reach (8-hex cap).")
    if power_curve(ship) < 1.0 and ship.faction.upper() == "FEDERATION":
        return (False, "Fed power curve too tight for full overload + speed same turn (6/tube).")
    if rng <= 3:
        return (True, "range <=3: overload for the alpha strike (mind point-blank feedback).")
    return (True, f"range {rng}: overload viable inside 8 hexes for extra punch.")


def breakdown_risk(breakdown_rating_low: int, first_het: bool = False,
                   nimble: bool = False, em_active: bool = False) -> float:
    """C6.5: break down if 1d6 (+mods) >= rating_low. First HET -2 (two for nimble),
    EM +1. Returns P(breakdown)."""
    mod = 0
    if first_het:
        mod -= 2
    if em_active:
        mod += 1
    faces = [min(6, max(1, d + mod)) for d in range(1, 7)]
    hits = sum(1 for f in faces if f >= breakdown_rating_low)
    return hits / 6.0


def should_het(ship, need_arc: bool, hull_pct: int) -> tuple:
    # HET costs ~5 hexes of WARP power; not on impulse 1; breakdown risk.
    if need_arc:
        return (True, "HET to suddenly bring weapons to bear the enemy didn't expect (warp power).")
    if hull_pct < 30:
        return (True, "HET to save the ship / present a fresh shield — accept breakdown risk.")
    return (False, "hold the HET in reserve (5 warp pts) for the decisive moment.")


# ==========================================================================
# Seeking-weapon defense
# ==========================================================================

def seeking_defense(ship, incoming: int, range_to_seekers: int = 3) -> str:
    if incoming <= 0:
        return ""
    tips = []
    if range_to_seekers <= 2:
        tips.append("phaser-3 point defense (effective <=2 hexes)")
    tips.append("ADD/anti-drones at 3 hexes (ignore ECM, can't be phaser-killed)")
    tips.append("Wild Weasel voids all currently-tracking seekers (needs a shuttle bay free)")
    tips.append("tractor a seeker (an ECM shift can make tractors fail)")
    return f"{incoming} seeker(s) inbound — defense: " + "; ".join(tips) + \
           ". Beware the Gorn Anchor denying your WW."


# ==========================================================================
# Energy plan (doctrine-ordered)
# ==========================================================================

def energy_plan(ship, threatened: bool, want_speed: int) -> dict:
    """Doctrine order: housekeeping -> weapons/movement -> reserve -> EW."""
    p = ship.power
    plan, notes = {}, []
    hk = R.MANDATORY_POWER
    plan["housekeeping"] = hk
    p -= hk
    notes.append(f"housekeeping {hk} (LS+shields+FC) pre-plotted")
    spd = min(want_speed, p)
    plan["movement"] = spd
    p -= spd
    weap = min(p, sum(w.max_power or w.arming_cost for w in ship.weapons if w.can_fire() or not w.is_instant()))
    plan["weapons"] = weap
    p -= weap
    # leftover -> batteries/reserve (NOT reflexive shield reinforcement)
    plan["reserve"] = max(0, p)
    notes.append("leftover -> batteries/reserve (refill from warp); "
                 "avoid reflexive shield reinforcement — prefer EW/HET/tractor")
    return {"alloc": plan, "notes": notes}


# ==========================================================================
# Top-level assessment (the advisor / control brief)
# ==========================================================================

@dataclass
class Assessment:
    summary: str
    primary_target: Optional[str]
    recommendations: list = field(default_factory=list)   # ordered strings
    fire_plan: list = field(default_factory=list)         # FireSolution list
    intel: dict = field(default_factory=dict)


def assess(ship, enemies, impulse: int = 1, turn: int = 1) -> Assessment:
    """Full sophisticated read of the position for `ship`."""
    live = [e for e in enemies if getattr(e, "hull", 1) > 0 and e.pos]
    if not live:
        return Assessment("No contacts.", None, ["Charge weapons; hold reserve; scan."], [])

    # primary target: best expected alpha strike now (kill-progress weighted)
    def score(e):
        a = alpha_strike(ship, e)
        return a * (1.0 + (1.0 - e.hull_pct / 100.0))    # favor the wounded
    tgt = max(live, key=score)
    rng = H.hex_distance(ship.pos, tgt.pos)
    sols = firing_solutions(ship, tgt)
    a_now = sum(s.expected for s in sols)
    a_them = alpha_strike(tgt, ship)

    recs = []
    # timing
    if impulse >= DOC.IMPULSE_OF_DECISION:
        recs.append(f"Impulse {impulse}: past the Impulse of Decision (#25) — fire NOW "
                    "or weapons won't recycle by Impulse #1.")
    # Mizia / shield
    recs.append(mizia_recommendation(ship, tgt))
    # overload
    ov, ovn = should_overload(ship, tgt)
    recs.append(("OVERLOAD: " if ov else "hold overload: ") + ovn)
    # EW
    ewr = recommend_ew(ship, tgt, threatened=a_them > ship.hull * 0.3)
    recs.append(f"EW: set ECM {ewr['ecm']} / ECCM {ewr['eccm']} — {ewr['note']}")
    # power curve
    pc = power_curve(ship)
    if pc < 1.0:
        recs.append(f"Power curve tight ({pc:.2f}): arm two standard weapons over one "
                    "overload; don't over-speed.")
    # HET
    het, hetn = should_het(ship, need_arc=(not sols), hull_pct=ship.hull_pct)
    if het:
        recs.append("HET: " + hetn)
    # firepower doctrine
    if facing_shield_is_down(ship, tgt):
        recs.append("Down shield facing you — a medium-range shot here beats a "
                    "short-range shot on a strong shield. Withhold one phaser to keep him guessing.")

    summ = (f"T{turn} I{impulse}: primary {tgt.obj_id} ({tgt.faction}) at range {rng}, "
            f"hull {tgt.hull_pct}%. My alpha ~{a_now:.0f}, his ~{a_them:.0f}. "
            f"Power curve {pc:.2f}. Doctrine: {DOC.RACE_DOCTRINE.get(ship.faction.upper(),'fight well')}")
    return Assessment(summ, tgt.obj_id, recs, sols,
                      {"range": rng, "my_alpha": a_now, "their_alpha": a_them,
                       "power_curve": pc, "shields": shield_report(tgt)})
