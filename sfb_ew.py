"""
Electronic warfare model (D6) — the square-law ECM/ECCM shift system, extracted
from the Captain's Tactics Manual.

Net EW shift degrades the firer's to-hit. On the phaser chart a +N shift moves N
columns right (a worse effective die), so we model it as die += net_shift.

Shift thresholds (perfect squares): 1 pt -> +1, 4 pts -> +2, 9 pts -> +3.
Natural EW sources: Wild Weasel = 6 ECM (~+2), Erratic Maneuvers = 4 ECM (~+2),
ECM drone = +1 (unpowered), Orion stealth adds free ECM, Nebula = 9 ECM.
Each ECCM point cancels one ECM point before the shift is computed.
"""

from __future__ import annotations
import math

WW_ECM = 6          # wild weasel
EM_ECM = 4          # erratic maneuvers
ECM_DRONE_SHIFT = 1
SENSOR_EW_POOL = 6  # nominal undamaged ship - see ew_pool() for the live value


def ew_status(ship):
    """(ecm, eccm) a ship is ACTUALLY running, from the client's own state.

    The client tracks this per ship as `ew_status`, a plain "ECM/ECCM" pair.
    Nothing in the engine read it, so enemy ECM was unknown, ECCM was pinned at
    zero, and every EW shift computed to +0 - the fire advice silently assumed
    an unjammed battlefield all game. D17.194 makes EW levels public, so there
    is no hidden-information reason not to read it.
    """
    raw = (ship.get("ew_status") or "").strip()
    if not raw:
        return 0.0, 0.0
    parts = raw.split("/")
    try:
        ecm = float(parts[0] or 0)
        eccm = float(parts[1]) if len(parts) > 1 and parts[1] else 0.0
    except ValueError:
        return 0.0, 0.0
    return max(0.0, ecm), max(0.0, eccm)


def sensor_rating(ship):
    """(rating, known) - the highest unchecked number on the sensor track.

    D6.11: "almost all ships have a '6' in the first box of their sensor track
    (or an assumed rating of six)", and the track is checked off as it takes
    damage. This ONE number does double duty in the rules:
      * D6.11  - lock-on succeeds on a d6 roll <= the rating
      * D6.310 - it caps total ECM + ECCM a ship may allocate
    so both are derived from it here rather than from two separate constants.
    """
    sens = (ship.get("systems") or {}).get("sensor")
    if not sens or not sens[1]:
        return SENSOR_EW_POOL, False
    return max(0, min(SENSOR_EW_POOL, int(sens[0]))), True


def guiding(ship, state, log):
    """How many seekers this ship is currently guiding (F3.21).

    ONE ledger, shared by the ship's own drone racks and its attack shuttles'
    drones - an AAS cannot guide its own (R5.F2), so both draw on the SAME
    sensor-rating channel budget. Two separate counts (rack code + flight code)
    could double-count or, worse, each ignore the other and jointly over-commit.

    Attribution is by the LOG: the client names the launcher on each seeker's
    'added' line ("D001(1).1.17 ... was launched by <ship>"), which the log
    parser records as a launch event. Seeker board records carry no launcher id,
    so the log is the only link.
    """
    mine = {e.get("unit") for e in ((log or {}).get("events") or [])
            if e.get("kind") == "launch" and e.get("ship") == ship.get("label")
            and e.get("what") in ("drone", "plasma")}
    if not mine:
        return 0
    return len([s for s in (state or {}).get("seeking", []) or []
                if s.get("label") in mine])


def free_channels(ship, state, log):
    """Control channels this ship has left (F3.21): sensor rating minus guiding."""
    total = sensor_rating(ship)[0]
    return max(0, total - guiding(ship, state, log))


def ew_pool(ship):
    """(points, known) - EW this ship may allocate.

    D6.310: "The total amount of energy put into (allocated for) ECM and ECCM
    combined cannot exceed the highest unchecked number on the sensor track
    (usually six)." A flat 6 was assumed for every hull in every state of
    damage; the cap now falls as the sensor track is shot away, which is much of
    the reason for shooting it.
    """
    rating, known = sensor_rating(ship)
    return float(rating), known


def lock_on_chance(ship):
    """Probability of achieving sensor lock-on this turn (D6.11).

    Roll one die; lock-on succeeds on a result <= the sensor rating. Undamaged
    that is 6, i.e. automatic - which is why this only starts to matter once the
    sensor track has been hit.

    NOTE: electronic warfare does NOT modify this roll. ECM and ECCM change the
    to-hit shift once you HAVE a lock-on; they cannot create or break one
    (D6.3 preamble: "ECM and ECCM cannot 'break' a lock-on, but they can
    dramatically reduce the effectiveness of that lock-on").
    """
    rating, known = sensor_rating(ship)
    return min(1.0, max(0.0, rating / 6.0)), known


# D6.124 / D6.371: systems that simply cannot be used without a lock-on.
NEEDS_LOCK_ON = ("tractor beams (G7.412)", "transporters (G8.17)",
                 "stasis field generators (G16.35)", "anti-drones (E5.14)",
                 "PPDs (E11.15)", "maulers (E8.15)", "DERFACS (E3.62)",
                 "web casters (E12.13)", "displacement devices (G18.13)")


def lock_on_advice(ship, has_tractor_link=False):
    """What a degraded sensor track actually costs, in order of consequence.

    Returns (headline, [reasons]) or None while lock-on is still automatic.
    """
    rating, known = sensor_rating(ship)
    if not known or rating >= SENSOR_EW_POOL:
        return None
    p = rating / 6.0
    # State the failure rate directly. "Fails one turn in N" only reads sensibly
    # while success is high, and collapses into nonsense at low ratings.
    why = [f"D6.11: lock-on succeeds on a d6 roll of {rating} or less - "
           f"{p * 100:.0f}% success, so it FAILS {(1 - p) * 100:.0f}% of turns",
           f"D6.123: on a failure the TRUE range to every unlocked target is "
           f"DOUBLED before the scanner adjustment - and if that exceeds a "
           f"weapon's maximum, it cannot fire at all",
           f"D6.121: a failing ship may not LAUNCH seeking weapons, and "
           f"D6.122 RELEASES any already on the map that it can no longer see",
           f"D6.310: the same rating caps ECM + ECCM combined, so we are now "
           f"limited to {rating} point(s) of EW as well",
           "cannot be used at all after a failure: " + ", ".join(NEEDS_LOCK_ON[:4])]
    if has_tractor_link:
        why.append("G7.412: a tractor beam ALREADY ATTACHED keeps an automatic "
                   "lock-on both ways - that link survives a failed roll")
    return (f"SENSORS DEGRADED - lock-on now {p * 100:.0f}%, not automatic", why)


# EW is bought in whole points but only PAYS at perfect squares (D6): 1, 4, 9.
# Spending 3 buys the same +1 shift as spending 1 - two points wasted. These are
# the only allocations worth making.
EW_THRESHOLDS = (0, 1, 4, 9)


def recommend_ew(ship, enemies, budget=None):
    """(ecm, eccm, why) - how to split this ship's EW points.

    Only ever recommends a threshold value; anything between them buys nothing.
    ECCM is sized to cancel the worst enemy ECM we can see, because each ECCM
    point cancels one ECM point BEFORE the square-law is applied - far cheaper
    than trying to out-jam him.
    """
    pool, known = ew_pool(ship)
    if budget is not None:
        pool = min(pool, budget)
    worst_ecm = max([ew_status(e)[0] for e in (enemies or [])] or [0.0])
    why = []

    eccm = 0.0
    if worst_ecm > 0:
        eccm = min(pool, worst_ecm)
        why.append(f"his ECM is {worst_ecm:g}: {eccm:g} ECCM cancels it 1-for-1 "
                   f"BEFORE the square-law (D6), which is the cheapest answer - "
                   f"out-jamming him instead would leave his shift intact")
    left = pool - eccm
    ecm = max([t for t in EW_THRESHOLDS if t <= left] or [0])
    if ecm:
        nxt = [t for t in EW_THRESHOLDS if t > ecm]
        why.append(f"{ecm:g} ECM buys a +{points_to_shift(ecm)} shift against his "
                   f"fire" + (f"; the next step up costs {nxt[0]}" if nxt else ""))
    if left > ecm:
        why.append(f"WARN {left - ecm:g} point(s) left over buy NOTHING - EW pays "
                   f"only at 1/4/9 (D6). Spend them elsewhere")
    if not known:
        why.append(f"sensor track not in the save - pool assumed to be the "
                   f"undamaged {SENSOR_EW_POOL}")
    return ecm, eccm, why


def points_to_shift(ecm_points: float) -> int:
    """Square-law: shift = floor(sqrt(points)). 1->1, 4->2, 9->3, 16->4."""
    if ecm_points <= 0:
        return 0
    return int(math.isqrt(int(ecm_points)))


def net_shift(target_ecm: float, firer_eccm: float,
              extra_shift: int = 0) -> int:
    """Net defensive shift the FIRER suffers. ECCM cancels ECM 1-for-1 first;
    extra_shift is added directly (ECM drones, etc.)."""
    effective_ecm = max(0.0, target_ecm - max(0.0, firer_eccm))
    return points_to_shift(effective_ecm) + max(0, extra_shift)


def apply_shift_roll(weapon, rng: int, die: int, shift: int) -> int:
    """Damage after an EW shift: worsen the die by `shift` columns (cap 6)."""
    return weapon.roll_damage(rng, min(6, die + max(0, shift)))


# Expected-damage degradation by shift (from the manual's REDUCED-EFFECTS table,
# approximated per weapon family; used by the planning/alpha-strike code).
_DEGRADE = {          # shift 1, 2, 3  (fraction of damage REMOVED)
    "phaser_short": (0.10, 0.28, 0.50),   # r<=3
    "phaser_med":   (0.30, 0.54, 0.77),   # r4-8
    "seeking":      (0.08, 0.17, 0.29),
    "photon":       (0.20, 0.40, 0.60),
    "disruptor":    (0.22, 0.45, 0.66),
    "plasma":       (0.25, 0.50, 0.67),
    "hellbore":     (0.09, 0.21, 0.42),
}


def degrade_expected(base_damage: float, family: str, shift: int) -> float:
    if shift <= 0 or base_damage <= 0:
        return base_damage
    tbl = _DEGRADE.get(family, _DEGRADE["disruptor"])
    frac = tbl[min(shift, 3) - 1]
    return base_damage * (1.0 - frac)


def weapon_family(wtype_name: str, rng: int) -> str:
    n = wtype_name.lower()
    if "phaser" in n or "gatling" in n:
        return "phaser_short" if rng <= 3 else "phaser_med"
    if "photon" in n:
        return "photon"
    if "disruptor" in n:
        return "disruptor"
    if "plasma" in n:
        return "plasma"
    if "hellbore" in n:
        return "hellbore"
    if "drone" in n or "fighter" in n:
        return "seeking"
    return "disruptor"


def seeker_warhead_fraction(shift: int, prox_roll: int = None) -> float:
    """D6.36: EW shift is added to a 1d6 Proximity-of-Detonation roll.
    1-6 -> 100%, 7-8 -> 50%, 9-10 -> 25%, 11+ -> 0%. With no roll given,
    return the EXPECTED fraction over a fair d6 (planning value)."""
    def frac(total):
        return 1.0 if total <= 6 else 0.5 if total <= 8 else 0.25 if total <= 10 else 0.0
    if prox_roll is not None:
        return frac(prox_roll + max(0, shift))
    return sum(frac(d + max(0, shift)) for d in range(1, 7)) / 6.0


# Hard EW facts the planner keys off (from the manual):
#  - a photon cannot hit beyond range 8 against a +2 shift
#  - a disruptor still has ~33% at r8 under +2
#  - a ph-1 at r2 needs a +4 shift before any clean-miss chance exists
PHOTON_DEAD_BEYOND_R8_AT_SHIFT = 2
