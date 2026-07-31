"""
Deterministic SFB tactical AI — ported from the home-grown engine
(main.cpp: base_faction_profile, compute_ai_profile, ai_eaf, ai_move, ai_fire)
into the online-driver environment.

Operates on a lightweight TacticalShip view (built by sfb_brain from decoded
online pieces + SSD). Consumes an optional high-level posture (from the Claude
bridge) and produces concrete decisions: target, heading, speed, fire list, and
an energy allocation. Pure logic — no I/O.
"""

from __future__ import annotations

from dataclasses import dataclass, field
from typing import Optional

import sfb_hex as hexlib
from sfb_rules import Weapon, EnergyAllocation, MANDATORY_POWER, turn_mode, max_accel


# --------------------------------------------------------------------------
# Views
# --------------------------------------------------------------------------

@dataclass
class TacticalShip:
    obj_id: str
    faction: str                 # upper-case, e.g. "FEDERATION"
    pos: tuple                   # (x, y)
    facing: int                  # 0-5
    speed: int
    hull: int
    hull_max: int
    power: int                   # total power available this turn
    ecm: int = 0
    eccm: int = 0
    last_speed: int = 0
    hexes_since_turn: int = 99    # allow first turn
    turn_mode_cat: int = 4        # 0=AA..6=F; Fed CA = D = 4
    weapons: list = field(default_factory=list)
    sensor_lock: bool = True
    shields: list = field(default_factory=lambda: [0] * 6)      # current, facing #1..#6
    shields_max: list = field(default_factory=lambda: [0] * 6)

    @property
    def hull_pct(self) -> int:
        return int(self.hull * 100 / max(1, self.hull_max))


@dataclass
class Profile:
    preferred_range: int
    retreat_hull_pct: int
    use_ecm: bool
    speed_close: int   # tenths-of-power fraction when closing
    speed_hold: int    # tenths-of-power fraction when holding


# base_faction_profile (main.cpp:1269) — the posture "personality" table.
BASE_PROFILE = {
    "FEDERATION": Profile(5, 25, True, 5, 3),
    "KLINGON":    Profile(2, 15, False, 7, 5),
    "ROMULAN":    Profile(7, 30, True, 4, 2),
    "GORN":       Profile(4, 30, False, 5, 3),
    "KZINTI":     Profile(3, 20, True, 7, 5),
    "THOLIAN":    Profile(6, 35, True, 3, 2),
    "ORION":      Profile(6, 25, True, 6, 3),
    "HYDRAN":     Profile(5, 25, True, 5, 3),
    "LYRAN":      Profile(2, 20, True, 7, 5),
}


def base_profile(faction: str) -> Profile:
    p = BASE_PROFILE.get(faction.upper(), Profile(5, 25, True, 5, 3))
    return Profile(p.preferred_range, p.retreat_hull_pct, p.use_ecm,
                   p.speed_close, p.speed_hold)


def compute_profile(ship: TacticalShip) -> Profile:
    """base + adjustments by weapon mix (compute_ai_profile, main.cpp:1285)."""
    p = base_profile(ship.faction)
    # plasma-primary opens the range; short-range-only closes it.
    from sfb_rules import WeaponType
    types = {w.type for w in ship.weapons}
    if types & {WeaponType.PLASMA_R, WeaponType.PLASMA_F, WeaponType.PLASMA_G}:
        p.preferred_range = max(p.preferred_range, 5)
    short_only = types and types <= {WeaponType.FUSION, WeaponType.PHASER3,
                                     WeaponType.DISRUPTOR}
    if short_only:
        p.preferred_range = min(p.preferred_range, 3)
    return p


def apply_posture(p: Profile, posture: str) -> tuple[Profile, bool]:
    """Posture mutates the profile (ai_eaf, main.cpp:1334). Returns (profile,
    force_retreat)."""
    force_retreat = False
    if posture == "AGGRESSIVE":
        p.speed_close += 2
        p.preferred_range = max(1, p.preferred_range - 1)
    elif posture == "DEFENSIVE":
        p.speed_hold = max(0, p.speed_hold - 1)
        p.retreat_hull_pct += 10
    elif posture == "EVASIVE":
        force_retreat = True
    return p, force_retreat


# --------------------------------------------------------------------------
# Enemy ranking
# --------------------------------------------------------------------------

def rank_enemies(me: TacticalShip, enemies: list) -> list:
    """kill_score = dmg_pct*10 - dist*0.1 (ai_move, main.cpp:1472)."""
    def score(e):
        dmg_pct = 1.0 - e.hull / max(1, e.hull_max)
        d = hexlib.hex_distance(me.pos, e.pos)
        return dmg_pct * 10.0 - d * 0.1
    return sorted(enemies, key=score, reverse=True)


# --------------------------------------------------------------------------
# Movement: heading + speed
# --------------------------------------------------------------------------

def choose_heading(me: TacticalShip, enemies: list, profile: Profile,
                   retreating: bool) -> int:
    """Probe all 6 neighbours; score by weighted range-fit (ai_move)."""
    if not enemies:
        return me.facing
    ranked = rank_enemies(me, enemies)
    best_face, best_val = me.facing, -1e9
    for d in range(6):
        probe = hexlib.neighbor(me.pos, d)
        val = 0.0
        for i, e in enumerate(ranked):
            w = 3.0 if i == 0 else 0.5 if i == 1 else 0.25
            nd = hexlib.hex_distance(probe, e.pos)
            val += (w * nd) if retreating else (w * (10.0 - abs(nd - profile.preferred_range)))
        if val > best_val:
            best_val, best_face = val, d
    return best_face


def limited_turn(me: TacticalShip, desired_face: int) -> int:
    """Apply turn-mode limit: at most one 60° step, only if turn mode satisfied."""
    if desired_face == me.facing:
        return me.facing
    tm = turn_mode(me.speed, me.turn_mode_cat)
    if me.hexes_since_turn < tm:
        return me.facing  # not allowed to turn yet
    cw = (desired_face - me.facing) % 6
    ccw = (me.facing - desired_face) % 6
    return (me.facing + 1) % 6 if cw <= ccw else (me.facing + 5) % 6


def choose_speed(me: TacticalShip, enemies: list, profile: Profile,
                 retreating: bool, move_power: int) -> int:
    """Posture fraction of power, accel-capped (ai_eaf, main.cpp:1380)."""
    accel_max = min(31, me.last_speed + max_accel(me.last_speed))
    move_avail = min(accel_max, move_power)
    if not enemies:
        return min(profile.speed_hold * me.power // 10, move_avail)
    threat_dist = min(hexlib.hex_distance(me.pos, e.pos) for e in enemies)
    if retreating:
        sfrac = 8
    elif threat_dist > profile.preferred_range + 1:
        sfrac = profile.speed_close
    else:
        sfrac = profile.speed_hold
    return max(0, min(me.power * sfrac // 10, move_avail))


# --------------------------------------------------------------------------
# Firing: which weapon at which target (ai_fire, main.cpp:1563)
# --------------------------------------------------------------------------

@dataclass
class FireOrder:
    weapon: Weapon
    target: TacticalShip
    range: int
    expected_damage: int


def plan_fire(me: TacticalShip, enemies: list) -> list:
    """For each ready weapon (highest-power first), pick best target by
    damage/hull, honouring arc + ECM screen."""
    from sfb_rules import WeaponType
    seeking = {WeaponType.DRONE, WeaponType.PLASMA_R, WeaponType.PLASMA_F,
               WeaponType.PLASMA_G, WeaponType.FIGHTER}
    ready = [w for w in me.weapons if w.can_fire()]
    ready.sort(key=lambda w: (w.allocated if w.is_instant() else w.max_power),
               reverse=True)
    orders = []
    for w in ready:
        is_seek = w.type in seeking
        best, best_score, best_rng, best_dmg = None, 0.0, 0, 0
        for e in enemies:
            if not is_seek and not hexlib.target_in_arc(me.pos, me.facing, w.arc, e.pos):
                continue
            rng = hexlib.hex_distance(me.pos, e.pos)
            if not me.sensor_lock:
                rng *= 2
            dmg = w.damage_at(rng)
            if w.is_instant():
                dmg = max(0, dmg - max(0, e.ecm - me.eccm))
            if dmg <= 0:
                continue
            s = dmg / max(1, e.hull)
            if s > best_score:
                best, best_score, best_rng, best_dmg = e, s, rng, dmg
        if best is not None:
            orders.append(FireOrder(w, best, best_rng, best_dmg))
    return orders


# --------------------------------------------------------------------------
# Energy allocation (ai_eaf, main.cpp:1320) — strict priority order
# --------------------------------------------------------------------------

def plan_eaf(me: TacticalShip, enemies: list, profile: Profile,
             retreating: bool, shield_damage: int = 0) -> EnergyAllocation:
    eaf = EnergyAllocation()
    power = me.power - MANDATORY_POWER

    # 2. Damage control (multiples of 2)
    if shield_damage > 0 and power >= 2:
        want = min(8 if retreating else 6, shield_damage)
        rp = min(want, (power // 2) * 2)
        eaf.repair = rp
        power -= rp

    # 3. Arm non-instant weapons
    for w in me.weapons:
        if not w.is_instant() and not w.armed and not w.disabled:
            if (not retreating or power > 12) and power >= w.arming_cost:
                w.allocated = w.arming_cost
                power -= w.arming_cost

    # 4. Speed
    accel_max = min(31, me.last_speed + max_accel(me.last_speed))
    if not enemies:
        sfrac = profile.speed_hold
        threat_dist = 99
    else:
        threat_dist = min(hexlib.hex_distance(me.pos, e.pos) for e in enemies)
        sfrac = 8 if retreating else (profile.speed_close
                if threat_dist > profile.preferred_range + 1 else profile.speed_hold)
    spd = max(0, min(min(accel_max, power), me.power * sfrac // 10))
    eaf.speed = spd
    power -= spd

    # 5. ECM
    if profile.use_ecm and not retreating and power >= 2:
        eaf.ecm = 2
        power -= 2

    # 6. General reinforcement when damaged
    if me.hull_pct < 100 and power >= 2:
        gr = min(4, (power // 2) * 2)
        eaf.general_reinforce = gr
        power -= gr

    # 7. Instant weapons (phasers) to max
    for w in me.weapons:
        if w.is_instant() and not w.disabled and w.max_power > 0:
            fill = min(w.max_power, power)
            w.allocated = fill
            power -= fill
            if power <= 0:
                break

    # 9. Directional reinforce leftover onto threatened facing
    if power > 0 and enemies:
        threat = min(enemies, key=lambda e: hexlib.hex_distance(me.pos, e.pos))
        si = hexlib.shield_hit(me.pos, me.facing, threat.pos)
        eaf.reinforce[si] += power
        power = 0

    return eaf


# --------------------------------------------------------------------------
# Top-level: turn a posture into a full plan
# --------------------------------------------------------------------------

@dataclass
class TurnPlan:
    speed: int
    heading: int            # desired facing (turn-limited applied by caller per hex)
    fire: list              # list[FireOrder]
    eaf: EnergyAllocation
    retreating: bool
    posture: str


def make_plan(me: TacticalShip, enemies: list, posture: str = "HOLD_RANGE",
              speed_override: int = -1) -> TurnPlan:
    profile = compute_profile(me)
    profile, force_retreat = apply_posture(profile, posture)
    retreating = force_retreat or me.hull_pct < profile.retreat_hull_pct

    move_power = me.power  # caller may pass a movement-only budget
    eaf = plan_eaf(me, enemies, profile, retreating,
                   shield_damage=0)
    speed = eaf.speed if speed_override < 0 else max(0, min(speed_override, 31))
    heading = choose_heading(me, enemies, profile, retreating)
    fire = plan_fire(me, enemies)
    return TurnPlan(speed, heading, fire, eaf, retreating, posture)
