"""
Shadow-state layer - Phase 1: KINEMATICS.

The bridge keeps its own mutable copy of each ship's position, facing and speed,
applies the moves it advises impulse by impulse, and reconciles that copy against
the client's actual save. Where they agree the movement rules are right; where
they diverge, a rule (or the even-q geometry, or the impulse chart) is wrong.
This is the "shadow referee" - a continuous regression test against real play.

Phase 1 tracks ONLY kinematics (x, y, facing, speed). Energy, combat and damage
are later phases (see SHADOW_STATE.md). It reuses the validated geometry in
sfb_hex and the C1.4 impulse chart in sfb_rules - it adds no new movement rules,
it just makes them MUTATE stored state instead of only advising.

A move is one of:
    STRAIGHT | SLIP_LEFT | SLIP_RIGHT | TURN_LEFT | TURN_RIGHT | HET | HOLD
Facings are 0-5 clockwise (sfb_hex convention); +1 = turn right, -1 = turn left.
"""
from __future__ import annotations

import sfb_hex as H
from sfb_rules import moves_this_impulse

MOVES = ("STRAIGHT", "SLIP_LEFT", "SLIP_RIGHT",
         "TURN_LEFT", "TURN_RIGHT", "HET", "HOLD")

# Fields the shadow owns in Phase 1. Kept explicit so reconcile() compares
# exactly these and nothing it does not yet model.
KINEMATIC_FIELDS = ("x", "y", "facing", "speed")


def snapshot(ship):
    """A shadow record from an observed ship dict (the dump_state schema).

    Kinematic fields drive Phase 1; shields/hull are carried so the Phase-5
    direct-fire handler has something to damage (they are NOT part of the
    kinematic reconcile - combat reconciles by legality, not exact state). A
    ship missing position is skipped by the caller, not defaulted - a made-up
    (0,0) would silently reconcile-fail against every real board.
    """
    rec = {
        "label": ship.get("label"),
        "x": ship.get("x"),
        "y": ship.get("y"),
        "facing": int(ship.get("facing", 0) or 0),
        "speed": int(ship.get("speed", 0) or 0),
    }
    if ship.get("shields") is not None:
        rec["shields"] = list(ship["shields"])
        rec["shields_max"] = list(ship.get("shields_max") or ship["shields"])
    if ship.get("hull") is not None:
        rec["hull"] = list(ship["hull"])
    # Box inventory for DAC internal allocation (6D4). Absent -> no internals
    # can be resolved for this ship, only shields tracked. boxes_max keeps the
    # ORIGINAL counts so D22/crippling percentages are exact, not save-proxied.
    inv = _box_inventory(ship)
    if inv:
        rec["boxes"] = inv
        rec["boxes_max"] = dict(inv)
        rec["pending_internals"] = 0.0     # marked in 6D2, resolved in 6D4
        rec["destroyed"] = []
        rec["crippled"] = False
    try:
        rec["move_cost"] = float(ship.get("move_cost") or 1.0)
    except (TypeError, ValueError):
        rec["move_cost"] = 1.0
    # Phaser capacitor as a LIVE engine resource: firing drains it (6D2.04),
    # end-of-turn refills it (6E3 / H6.1). Seeded from the client EAF charge.
    try:
        import sfb_command as CMD
        cap_max = CMD.phaser_capacitor(ship)
        if cap_max and cap_max > 0:
            cur, _u, _n = CMD.phaser_capacitor_state(ship)
            rec["capacitor"] = float(cur if cur else cap_max)
            rec["capacitor_max"] = float(cap_max)
    except Exception:
        pass
    # Drone racks as launch state (6B06.05). ADD boxes are anti-drone, not
    # launchers, so build_racks drops them.
    racks = _build_racks(ship)
    if racks:
        rec["racks"] = racks
    # ESG charge per generator (for anti-seeker field release, 6A3.01).
    if (ship.get("weapons") or {}).get("esg", [0, 0])[0]:
        try:
            import sfb_actions as ACT
            rec["esg_charges"] = ACT.esg_charges_from_eaf(ship) or []
        except Exception:
            rec["esg_charges"] = []
        rec["esg_active"] = None
    return rec


# DAC box-type base name (from boxtypes.names) -> the inventory key we track.
# Several DAC boxes collapse onto one pool (three warp engines -> Warp; forward
# and rear hull -> Hull), matching how the save reports them as totals.
_DAC_TO_INV = {
    "Forward Hull": "Hull", "Rear Hull": "Hull", "Center Hull": "Hull", "Hull": "Hull",
    "Left Warp Engine": "Warp", "Right Warp Engine": "Warp",
    "Center Warp Engine": "Warp",
    "Impulse": "Impulse", "Emergency Impulse": "Impulse",
    "APR": "APR", "AWR": "AWR", "Battery": "Battery",
    "Disruptor": "Disruptor", "Photon": "Photon", "Plasma": "Plasma",
    "Hellbore": "Hellbore", "Fusion": "Fusion",
    "Drone": "Drone", "Drone-B": "Drone", "ADD": "ADD", "ESG": "ESG",
    "Phaser-1": "Phaser-1", "Phaser-2": "Phaser-2", "Phaser-3": "Phaser-3",
    "Phaser-4": "Phaser-4",
    "Shuttle": "Shuttle", "Transporter": "Transporter", "Tractor": "Tractor",
    "Sensor": "Sensor", "Scanner": "Scanner", "Lab": "Lab", "Probe": "Probe",
}


# Fire/log weapon token -> inventory box key. Mirrors _box_inventory's _wmap
# from the other direction, so a declared volley can be capped by surviving boxes.
_WEAPON_BOX_KEY = {
    "disruptor": "Disruptor", "disr": "Disruptor", "photon": "Photon",
    "phaser-1": "Phaser-1", "phaser1": "Phaser-1",
    "phaser-2": "Phaser-2", "phaser2": "Phaser-2",
    "phaser-3": "Phaser-3", "phaser3": "Phaser-3",
    "phaser-4": "Phaser-4", "phaser4": "Phaser-4", "phaser": "Phaser",
    "drone": "Drone", "esg": "ESG", "hellbore": "Hellbore",
    "fusion": "Fusion", "plasma": "Plasma", "add": "ADD",
}


def surviving_weapon_boxes(rec, weapon_token):
    """How many mounts of a weapon a ship still has, or None when it cannot be
    capped (no box model, or a weapon type not tracked in the inventory).

    None means "do not cap" - the caller fires as declared rather than zeroing a
    weapon the shadow simply does not model. A generic 'phaser' sums the mounts.
    """
    inv = rec.get("boxes")
    if not inv:
        return None
    key = _WEAPON_BOX_KEY.get(str(weapon_token or "").strip().lower())
    if key and key in inv:
        return inv[key]
    if key == "Phaser" or "phaser" in str(weapon_token or "").lower():
        n = sum(inv.get(k, 0) for k in ("Phaser-1", "Phaser-2", "Phaser-3", "Phaser-4"))
        return n if any(k in inv for k in ("Phaser-1", "Phaser-2", "Phaser-3",
                                           "Phaser-4")) else None
    return None


def _box_inventory(ship):
    """{inventory_key: box_count} of the DESTROYABLE systems the save exposes.

    Uses the *_max counts (total physical boxes); the shadow decrements them as
    DAC allocation destroys boxes. Control spaces the save does not enumerate
    (bridge, sensors on some hulls) are not here - a DAC hit on one is recorded
    as an untracked box rather than pretending to deplete a count we lack.
    """
    p = ship.get("power") or {}
    w = ship.get("weapons") or {}
    sysd = ship.get("systems") or {}
    hull = ship.get("hull") or [0, 0]
    inv = {}

    def setc(name, c):
        try:
            c = int(c or 0)
        except (TypeError, ValueError):
            return
        if c > 0:
            inv[name] = inv.get(name, 0) + c

    setc("Hull", hull[1] if len(hull) > 1 else hull[0])
    setc("Warp", p.get("warp_max"))
    setc("Impulse", p.get("impulse_max"))
    setc("APR", p.get("apr_max"))
    setc("AWR", p.get("awr_max"))
    setc("Battery", p.get("battery"))
    _wmap = {"disruptor": "Disruptor", "photon": "Photon", "drone": "Drone",
             "esg": "ESG", "hellbore": "Hellbore", "fusion": "Fusion",
             "plasma": "Plasma", "add": "ADD",
             "phaser-1": "Phaser-1", "phaser-2": "Phaser-2",
             "phaser-3": "Phaser-3", "phaser-4": "Phaser-4"}
    for k, v in w.items():
        name = _wmap.get(str(k).lower())
        if name and isinstance(v, (list, tuple)) and v:
            setc(name, v[1] if len(v) > 1 else v[0])
    _smap = {"shuttle": "Shuttle", "transporter": "Transporter",
             "tractor": "Tractor", "lab": "Lab", "probe": "Probe",
             "sensor": "Sensor", "scanner": "Scanner"}
    for k, v in sysd.items():
        name = _smap.get(str(k).lower())
        if name and isinstance(v, (list, tuple)) and v:
            setc(name, v[1] if len(v) > 1 else v[0])
    return inv


def build(state):
    """Seed a shadow world from an observed dump_state. Returns {label: record}.

    Ships without a position are omitted (see snapshot's note); their absence is
    reported by reconcile() as "not tracked", never as a phantom at the origin.
    """
    world = {}
    for s in (state.get("ships") or []):
        if s.get("x") is None or s.get("y") is None:
            continue
        rec = snapshot(s)
        if rec["label"]:
            world[rec["label"]] = rec
    return world


def apply_move(rec, move):
    """Mutate one shadow record by one impulse-move. Returns the record.

    A TURN changes facing then advances one hex on the NEW facing (SFB resolves
    the turn as you enter the hex). A SLIP shifts one hex 60 deg off the bow
    WITHOUT changing facing. HET reverses facing (180) and advances. HOLD and an
    unknown move do nothing - the shadow must never invent motion it cannot name.

    Phase 1 does not enforce turn-mode / slip-count legality here; that is the
    advisor's job (sfb_move) and the reconciler's check. apply_move is the raw
    kinematic transform, kept deliberately dumb so it cannot mask a rule error.
    """
    f = rec["facing"]
    if move == "STRAIGHT":
        rec["x"], rec["y"] = H.forward_hex((rec["x"], rec["y"]), f)
    elif move == "TURN_LEFT":
        f = (f - 1) % 6
        rec["facing"] = f
        rec["x"], rec["y"] = H.forward_hex((rec["x"], rec["y"]), f)
    elif move == "TURN_RIGHT":
        f = (f + 1) % 6
        rec["facing"] = f
        rec["x"], rec["y"] = H.forward_hex((rec["x"], rec["y"]), f)
    elif move == "SLIP_LEFT":
        rec["x"], rec["y"] = H.forward_hex((rec["x"], rec["y"]), (f - 1) % 6)
    elif move == "SLIP_RIGHT":
        rec["x"], rec["y"] = H.forward_hex((rec["x"], rec["y"]), (f + 1) % 6)
    elif move == "HET":
        f = (f + 3) % 6
        rec["facing"] = f
        rec["x"], rec["y"] = H.forward_hex((rec["x"], rec["y"]), f)
    # HOLD / unknown: no change.
    return rec


def advance_impulse(world, impulse, moves=None):
    """Advance every tracked ship by one impulse.

    `moves` maps label -> move keyword for the ships doing something other than
    STRAIGHT this impulse (a turn, slip or HET). A ship only moves at all on
    impulses its speed activates on (C1.4, moves_this_impulse); on a non-moving
    impulse it stays put even if a move was supplied - you cannot turn on an
    impulse you do not move. Returns the world.
    """
    moves = moves or {}
    for label, rec in world.items():
        if not moves_this_impulse(rec["speed"], impulse):
            continue
        apply_move(rec, moves.get(label, "STRAIGHT"))
    return world


def reconcile(world, state, tol_speed=0):
    """Diff the shadow world against a fresh observed dump_state.

    Returns a list of divergence dicts, one per (ship, field) that disagrees:
        {label, field, shadow, observed}
    plus a synthetic {field: "tracked"} note for ships present in one side only.
    Position and facing must match EXACTLY - they are governed by rules the
    shadow fully models, so any mismatch is a real bug to chase, not noise.
    Speed may carry a tolerance (a mid-turn deceleration the shadow has not been
    told about is expected slack, not a movement error).

    An empty list means the shadow tracked the client perfectly this refresh.
    """
    out = []
    observed = {}
    for s in (state.get("ships") or []):
        if s.get("label"):
            observed[s["label"]] = s

    for label, rec in world.items():
        obs = observed.get(label)
        if obs is None:
            out.append({"label": label, "field": "presence",
                        "shadow": "tracked", "observed": "absent"})
            continue
        if obs.get("x") is None or obs.get("y") is None:
            out.append({"label": label, "field": "presence",
                        "shadow": "tracked", "observed": "no-position"})
            continue
        if rec["x"] != obs.get("x") or rec["y"] != obs.get("y"):
            out.append({"label": label, "field": "position",
                        "shadow": (rec["x"], rec["y"]),
                        "observed": (obs.get("x"), obs.get("y"))})
        if rec["facing"] != int(obs.get("facing", 0) or 0):
            out.append({"label": label, "field": "facing",
                        "shadow": rec["facing"],
                        "observed": int(obs.get("facing", 0) or 0)})
        osp = int(obs.get("speed", 0) or 0)
        if abs(rec["speed"] - osp) > tol_speed:
            out.append({"label": label, "field": "speed",
                        "shadow": rec["speed"], "observed": osp})

    for label in observed:
        if label not in world and observed[label].get("x") is not None:
            out.append({"label": label, "field": "presence",
                        "shadow": "absent", "observed": "on-board"})
    return out


def reconcile_summary(divergences):
    """One-line human summary for the Referee tab. Green when clean."""
    if not divergences:
        return "shadow tracks the client (kinematics)"
    bits = []
    for d in divergences:
        bits.append(f"{d['label']} {d['field']}: shadow {d['shadow']} "
                    f"vs client {d['observed']}")
    return f"{len(divergences)} divergence(s): " + "; ".join(bits)


# ============================================================ Phase 2: ENERGY
#
# Unlike position, the client stores NO live energy charge (boxStatus is damage
# state, not power) - the only energy fact it records is the EAF allocation
# itself. So the shadow cannot diff a stored number the way kinematics does; it
# HOLDS the derived energy state (available/used balance, ESG charge per
# generator, battery discharge, phaser capacitor) and RECONCILES the EAF's
# self-consistency and legality - the checks that are actually decidable:
#   - energy balance : used <= available          (cannot spend what you lack)
#   - ESG cap        : each generator <= 5         (G23.22)
#   - capacitor      : 0 <= current <= capacity    (H6.21/H6.22)
# These reuse the validated derivations in sfb_actions / sfb_command; the shadow
# just stores their output and flags any that violate a rule.

ESG_MAX_STORE = 5      # G23.22 (mirrored here so Phase 2 is self-contained)


def energy_snapshot(ship):
    """Derived energy state for one ship from its EAF, or a has_eaf=False stub.

    Reuses the ground-truth derivations (esg_charges_from_eaf,
    battery_discharge_from_eaf, phaser_capacitor_state) rather than re-deriving -
    the shadow's job in Phase 2 is to STORE and CHECK, not to re-implement.
    """
    import sfb_actions as ACT
    import sfb_command as CMD
    from sfb_shuttles import _EAF_NON_SPEND

    turns = [t for t in (ship.get("eaf") or []) if t]
    rec = {"label": ship.get("label"), "has_eaf": bool(turns)}
    if not turns:
        return rec
    row = turns[-1]

    def num(k):
        try:
            return float(row.get(k, 0) or 0)
        except (TypeError, ValueError):
            return 0.0

    available = num("Warp Power") + num("Impulse") + num("APR") + num("AWR")
    used = sum(num(k) for k in row if k not in _EAF_NON_SPEND)
    rec["available"] = round(available, 2)
    rec["used"] = round(used, 2)
    rec["balance"] = round(available - used, 2)
    rec["esg"] = ACT.esg_charges_from_eaf(ship)          # per-generator or None
    # Raw ESG allocation in the LATEST turn per generator (pre-clamp), for the
    # single-turn over-cap legality check.
    import re as _re
    rec["esg_raw"] = {m.group(1): num(k) for k in row
                      if (m := _re.match(r"ESG \(([A-Z])\)$", k)) and num(k)}
    rec["battery_used"] = ACT.battery_discharge_from_eaf(ship)
    try:
        cur, _used, _note = CMD.phaser_capacitor_state(ship)
    except Exception:
        cur = None
    rec["cap_current"] = cur
    rec["cap_max"] = round(CMD.phaser_capacitor(ship), 2)
    return rec


def energy_checks(rec):
    """Legality violations in one energy snapshot. Empty list = consistent.

    Only checks the EAF can actually be wrong about - a stored charge the client
    does not keep is not invented here. Note the ESG cap (G23.22) is NOT checked:
    esg_charges_from_eaf already clamps stored charge to five, so a violation can
    never reach this layer - the check would be dead. The raw per-turn ESG
    allocation is checked instead: a single turn cannot pour more than the
    5-pt cap into one generator.
    """
    out = []
    if not rec.get("has_eaf"):
        return out
    if rec.get("balance") is not None and rec["balance"] < -0.01:
        out.append({"label": rec["label"], "field": "balance",
                    "detail": f"allocated {rec['used']:g} but only "
                              f"{rec['available']:g} available "
                              f"(over by {-rec['balance']:g}) - EAF over-spends"})
    for lab, g in (rec.get("esg_raw") or {}).items():
        if g > ESG_MAX_STORE + 0.01:
            out.append({"label": rec["label"], "field": f"ESG {lab}",
                        "detail": f"{g:g} pts allocated in one turn, over the "
                                  f"{ESG_MAX_STORE}-pt cap (G23.22)"})
    cur, cap = rec.get("cap_current"), rec.get("cap_max")
    if cur is not None and cap:
        if cur < -0.01:
            out.append({"label": rec["label"], "field": "phaser capacitor",
                        "detail": f"negative charge {cur:g}"})
        elif cur > cap + 0.01:
            out.append({"label": rec["label"], "field": "phaser capacitor",
                        "detail": f"charge {cur:g} exceeds capacity {cap:g} (H6.21)"})
    return out


def energy_world(state):
    """Per-ship energy snapshots + their checks for the whole board.

    Returns (snapshots, violations) where snapshots is {label: rec} and
    violations is a flat list of every legality problem across all ships.
    """
    snaps, viol = {}, []
    for s in (state.get("ships") or []):
        if not s.get("label"):
            continue
        rec = energy_snapshot(s)
        snaps[s["label"]] = rec
        viol.extend(energy_checks(rec))
    return snaps, viol


# ============================================================ Phase 3: COMBAT
#
# The client rolls; the shadow CHECKS. The combat log records each volley as a
# 'fire' event (attacker fires N of a weapon at a target) and the resulting
# 'damage' event (target took `total`, split by_shield). Two things are then
# decidable WITHOUT re-rolling the dice - exactly the legality-not-outcome
# reconciliation the design calls for:
#
#   1. MAGNITUDE - observed total <= N x the weapon's chart maximum at that
#      range. More damage than the weapon can physically do => a rule or a read
#      is wrong. Range past the weapon's reach with damage scored => same.
#   2. SHIELD FACING - the shield that took the hit must be the one the geometry
#      puts toward the attacker. This directly checks bearing/shield_hit against
#      live combat. (Approximate: it uses CURRENT board positions, so a volley
#      fired several impulses before a big move can read as a facing mismatch;
#      flagged as APPROX, not a hard violation.)
#
# Box-type (DAC) is NOT reconciled: the log never reports which internal boxes
# were struck, so there is nothing to check against - see sfb_resolve's DAC note.


def _pos_of(state, label):
    for s in (state.get("ships") or []):
        if s.get("label") == label and s.get("x") is not None:
            return (s["x"], s["y"], int(s.get("facing", 0) or 0))
    return None


def combat_reconcile(state, log, turn=None):
    """Check this turn's logged volleys for magnitude and shield-facing legality.

    Returns (checks, violations): `checks` is a per-volley summary list; each
    violation also appears in `violations` with severity 'hard' (magnitude/range,
    physically impossible) or 'approx' (facing, position-timing caveat).
    """
    import sfb_resolve as RES
    events = (log or {}).get("events") or []
    turn = turn if turn is not None else (log or {}).get("turn")
    fires = [e for e in events if e.get("kind") == "fire"
             and (turn is None or e.get("t") == turn)]
    damages = [e for e in events if e.get("kind") == "damage"
               and (turn is None or e.get("t") == turn)]

    checks, viol = [], []

    for fe in fires:
        atk, tgt = fe.get("ship"), fe.get("target")
        n = int(fe.get("n") or 0)
        key = RES.weapon_key(fe.get("weapon"))
        i = fe.get("i")
        # Pair with the nearest same-impulse damage event on this target.
        dmg_ev = None
        for de in damages:
            if de.get("ship") == tgt and de.get("i") == i:
                dmg_ev = de
                break
        summary = {"t": fe.get("t"), "i": i, "attacker": atk, "target": tgt,
                   "weapon": fe.get("weapon"), "n": n}

        # --- range + magnitude (needs the geometry to know the range)
        apos, tpos = _pos_of(state, atk), _pos_of(state, tgt)
        if apos and tpos:
            rng = _hex_range((apos[0], apos[1]), (tpos[0], tpos[1]))
            summary["range"] = rng
            if key:
                cap = RES.volley_max(key, n, rng)
                reach = RES.max_range(key)
                summary["max_possible"] = round(cap, 2)
                if dmg_ev is not None:
                    total = float(dmg_ev.get("total") or 0)
                    summary["observed"] = total
                    if reach and rng > reach and total > 0:
                        viol.append({"severity": "hard", "kind": "range",
                                     "detail": f"{atk} {fe.get('weapon')} scored "
                                     f"{total:g} at range {rng}, past its {reach}-hex "
                                     f"reach"})
                    elif total > cap + 0.01:
                        viol.append({"severity": "hard", "kind": "magnitude",
                                     "detail": f"{atk}->{tgt}: {total:g} damage from "
                                     f"{n}x{fe.get('weapon')} at range {rng} exceeds "
                                     f"the {cap:g} chart maximum"})

        # --- shield facing (approx, current positions)
        if apos and tpos and dmg_ev is not None:
            hit_shields = [ix for ix, v in enumerate(dmg_ev.get("by_shield") or [])
                           if v]
            if len(hit_shields) == 1:
                predicted = H.shield_hit((tpos[0], tpos[1]), tpos[2],
                                         (apos[0], apos[1]))
                summary["hit_shield"] = hit_shields[0] + 1
                summary["predicted_shield"] = predicted + 1
                if hit_shields[0] != predicted:
                    viol.append({"severity": "approx", "kind": "facing",
                                 "detail": f"{tgt} took the hit on #{hit_shields[0] + 1} "
                                 f"but {atk}'s bearing predicts #{predicted + 1} "
                                 f"(APPROX - uses current positions)"})
        checks.append(summary)
    return checks, viol


def _hex_range(a, b):
    return H.hex_distance(a, b)


def apply_damage(world, target_label, attacker_xy, amount):
    """MUTATOR (6D2): shield absorbs; the leak is RECORDED as pending internal
    damage, not yet allocated. Per the rules, 6D2 marks shield damage and records
    internals to be resolved in 6D4 (E1.11) - so this does NOT touch boxes; that
    is allocate_internals's job. Returns {shield, absorbed, leak} or None.
    """
    rec = world.get(target_label)
    if rec is None or "shields" not in rec:
        return None
    idx = H.shield_hit((rec["x"], rec["y"]), rec["facing"], attacker_xy)
    sh = rec["shields"][idx]
    absorbed = min(sh, amount)
    rec["shields"][idx] = round(sh - absorbed, 2)
    leak = round(amount - absorbed, 2)
    if leak > 0 and "pending_internals" in rec:
        rec["pending_internals"] = round(rec["pending_internals"] + leak, 2)
    return {"shield": idx + 1, "absorbed": absorbed, "leak": leak}


def allocate_internals(world, target_label, roller):
    """MUTATOR (6D4): allocate a ship's pending internal damage to REAL boxes via
    the client's DAC, destroying one box per point.

    Rules modelled: one 2d6 roll picks the chart ROW; successive points walk the
    columns left-to-right; a box type already destroyed is SKIPPED and damage
    continues to the next column (D4.3); a fresh 2d6 row is rolled when a row is
    exhausted ('next pass'). A DAC box the save does not enumerate (a control
    space) is counted as an untracked hit - a real box was struck, we just do not
    track its depletion. Returns a per-point list; clears pending_internals.

    NOT yet modelled (flagged): DAC COLUMN SEMANTICS (which of the 13 tracks a
    given hit uses - here successive columns, an assumption), and the D22
    energy-balance / crippling knock-ons of losing warp/impulse boxes.
    """
    import sfb_resolve as RES
    rec = world.get(target_label)
    if not rec or not rec.get("pending_internals"):
        return []
    points = int(round(rec["pending_internals"]))
    rec["pending_internals"] = 0.0
    inv = rec.setdefault("boxes", {})
    rows = RES._load_dac()
    names = RES._load_boxtypes()
    if not rows or points <= 0:
        return []

    def base(bt):
        nm = names[bt] if 0 <= bt < len(names) else ""
        return nm.rsplit("=", 1)[0]

    def inv_key(bname):
        key = _DAC_TO_INV.get(bname)
        if key and key in inv:
            return key
        if bname == "Phaser":                       # generic -> any phaser mount
            for k in ("Phaser-1", "Phaser-2", "Phaser-3", "Phaser-4"):
                if inv.get(k, 0) > 0:
                    return k
        return None

    hits = []
    roll = roller.d6() + roller.d6()
    ri = min(len(rows) - 1, max(0, roll - 2))
    col = 0
    for _ in range(points):
        placed = False
        for _guard in range(len(rows[ri]) * 3 + 6):
            if col >= len(rows[ri]):                # row exhausted -> next pass
                roll = roller.d6() + roller.d6()
                ri = min(len(rows) - 1, max(0, roll - 2))
                col = 0
            bt = rows[ri][col][0]
            bname = base(bt)
            col += 1
            if bname == "Excess Damage":
                hits.append({"box": "Excess Damage", "result": "excess"})
                placed = True
                break
            key = inv_key(bname)
            if key is None:
                hits.append({"box": bname, "result": "untracked"})
                placed = True
                break
            if inv.get(key, 0) > 0:                 # live box: destroy it
                inv[key] -= 1
                rec["destroyed"].append(key)
                hits.append({"box": key, "result": "destroyed", "left": inv[key]})
                placed = True
                break
            # else: box type already gone - skip, continue across columns (D4.3)
        if not placed:
            hits.append({"box": "Excess Damage", "result": "excess"})
    return hits


def _build_racks(ship):
    """Drone-rack launch state from a ship's `drone_racks`. Drops ADD boxes
    (anti-drone, not launchers). Each rack carries ammo, rate-of-fire and the
    per-turn firing state that E1.50/FD3.0 govern.
    """
    out = []
    try:
        import sfb_command as CMD
    except Exception:
        return out
    for r in (ship.get("drone_racks") or []):
        kind, label, spt, gap = CMD.rack_kind(r.get("box_type"))
        if kind != "drone":
            continue                       # ADD = anti-drone defence, not a launcher
        rounds = list(r.get("ammo") or [])
        out.append({
            "designator": r.get("designator"), "box_type": r.get("box_type"),
            "label": label, "ammo": len(rounds), "rounds": rounds,
            "shots_per_turn": spt, "gap": gap,
            "fired_this_turn": 0, "last_impulse": None,
        })
    return out


def can_launch_rack(rack, impulse):
    """(ok, reason) - may this rack launch a drone on this impulse?

    E1.50 / FD3.0: a rack has ammunition, a per-turn shot limit (1 for type-A/B,
    2 for type-C...), and a minimum impulse gap between launches (8, or 12 for
    type-C). All three must pass.
    """
    if rack["ammo"] <= 0:
        return False, "rack empty"
    if rack["fired_this_turn"] >= rack["shots_per_turn"]:
        return False, f"rate spent ({rack['shots_per_turn']}/turn used)"
    last = rack["last_impulse"]
    if last is not None and (impulse - last) < rack["gap"]:
        return False, f"within {rack['gap']}-impulse gap (last {last})"
    return True, ""


def launch_drone(world, seekers, ship_label, rack, target, impulse):
    """MUTATOR (6B06.05 / FD1.2): launch one drone from `rack`, adding a seeker at
    the ship's hex homing on `target`. Decrements ammo and books the rate-of-fire.
    Returns (seeker | None, note).
    """
    rec = world.get(ship_label)
    if rec is None:
        return None, f"{ship_label} not on board"
    ok, why = can_launch_rack(rack, impulse)
    if not ok:
        return None, f"{rack.get('label')} cannot launch: {why}"
    round_str = rack["rounds"].pop(0) if rack["rounds"] else ""
    rack["ammo"] -= 1
    rack["fired_this_turn"] += 1
    rack["last_impulse"] = impulse
    sk = {
        "label": f"{ship_label} drone i{impulse}/{rack.get('designator') or rack['label']}",
        "kind": "drone", "x": rec["x"], "y": rec["y"],
        "speed": 8,                        # std drone; speed modules (S/M/F) TODO
        "target": target, "loadout": round_str,
        "damage_taken": 0, "alive": True,
    }
    seekers.append(sk)
    return sk, f"launched {sk['label']} ({round_str or 'std drone'}) at {target}"


def build_seekers(state):
    """Seeker records (drones/plasma) from a dump_state's `seeking` list.

    Each carries position, speed, target label and loadout. `alive` lets the
    engine retire a seeker on impact without dropping it from the list. Seekers
    live in the run context, not the ship-world, so ship reconciliation is
    unaffected.
    """
    out = []
    for s in (state.get("seeking") or []):
        if s.get("x") is None or not s.get("label"):
            continue
        out.append({
            "label": s.get("label"), "kind": s.get("kind"),
            "x": s.get("x"), "y": s.get("y"),
            "speed": int(s.get("speed") or s.get("max_speed") or 8),
            "target": s.get("target"), "loadout": s.get("loadout"),
            "damage_taken": int(s.get("damage_taken") or 0),
            "alive": True,
        })
    return out


def find_seeker(seekers, label):
    """The live seeker with this label, or None."""
    for sk in (seekers or []):
        if sk.get("alive") and sk.get("label") == label:
            return sk
    return None


def seeker_kill_value(sk):
    """Damage needed to destroy a seeker (drone kill value; 4 std, 6 heavy).

    Plasma is NOT killed by phasers - they only reduce the warhead (FP1.611) -
    so a large value is returned to signal 'not killable this way'.
    """
    if "plasma" in (sk.get("kind") or "").lower():
        return 9999
    try:
        import sfb_command as CMD
        return CMD.drone_profile(sk).get("kill", 4)
    except Exception:
        return 4


def damage_seeker(seekers, label, amount):
    """MUTATOR: apply defensive-fire damage to a seeker (F2 / FD1.51 - phasers are
    unpenalised vs drones). Accumulates toward the kill value; retires the seeker
    when reached. Returns ('killed'|'damaged'|'immune'|'absent', taken, kill).
    """
    sk = find_seeker(seekers, label)
    if sk is None:
        return ("absent", 0, 0)
    kill = seeker_kill_value(sk)
    if kill >= 9999:                       # plasma - phasers do not kill it here
        return ("immune", sk.get("damage_taken", 0), kill)
    sk["damage_taken"] = sk.get("damage_taken", 0) + amount
    if sk["damage_taken"] >= kill:
        sk["alive"] = False
        return ("killed", sk["damage_taken"], kill)
    return ("damaged", sk["damage_taken"], kill)


def seeker_warhead(sk):
    """Warhead damage a seeker delivers on impact.

    Drones use sfb_command.drone_profile (12 standard, 24 heavy IV/V). Plasma
    warhead DECAYS with distance flown (sfb_seekers) - not fielded in the current
    battle, so a coarse launch value is used and flagged. Default 12 (the Kzinti
    Type-I staple) when the kind is unclear.
    """
    kind = (sk.get("kind") or "").lower()
    if "plasma" in kind:
        try:
            import sfb_seekers as SK
            return SK.plasma_strength(sk, 0) or 12
        except Exception:
            return 12
    try:
        import sfb_command as CMD
        return CMD.drone_profile(sk).get("warhead", 12)
    except Exception:
        return 12


def advance_seekers(world, seekers, impulse):
    """MUTATOR (6A2.04 / C1.31): move each live seeker one hex toward its target
    on the impulses its speed activates (C1.4), like any other playing piece.

    Records `prev` (the pre-move hex) so a later impact knows the approach bearing
    for shield facing. Returns [(label, (x, y))] for those that moved. Movement
    belongs in the movement segment, so both the ESG (6A3.01) and the impact
    (6A3.03) steps see seekers at their post-move positions.
    """
    moved = []
    for sk in seekers:
        if not sk.get("alive"):
            continue
        sk["prev"] = (sk["x"], sk["y"])
        tgt = world.get(sk.get("target"))
        if not tgt or tgt.get("x") is None:
            continue
        pre = (sk["x"], sk["y"])
        tpos = (tgt["x"], tgt["y"])
        if H.hex_distance(pre, tpos) > 0 and moves_this_impulse(sk["speed"], impulse):
            f = H.absolute_bearing(pre, tpos)
            sk["x"], sk["y"] = H.forward_hex(pre, f)
            moved.append((sk["label"], (sk["x"], sk["y"])))
    return moved


def resolve_seeker_impacts(world, seekers, impulse, roller):
    """MUTATOR (6A3.03 / F2.3): detonate any live seeker now sharing its target's
    hex (seekers were moved in 6A2 by advance_seekers) - warhead onto the facing
    shield, internals allocated immediately (6A3 allocates step by step).

    Returns a per-impact report list. A seeker whose target is gone is left alive
    (this slice does not model re-targeting or endurance expiry).
    """
    lines = []
    for sk in seekers:
        if not sk.get("alive"):
            continue
        tgt = world.get(sk.get("target"))
        if not tgt or tgt.get("x") is None:
            continue
        tpos = (tgt["x"], tgt["y"])
        if H.hex_distance((sk["x"], sk["y"]), tpos) != 0:
            continue                       # not on the target hex
        # Impact. The drone hits the shield facing where it came FROM (its prev hex).
        wh = seeker_warhead(sk)
        pre = sk.get("prev", (sk["x"], sk["y"]))
        approach = pre if pre != tpos else (sk["x"], sk["y"])
        res = apply_damage(world, sk["target"], approach, wh)
        sk["alive"] = False
        if res is None:
            lines.append(f"{sk['label']} ({sk['kind']}) HIT {sk['target']} "
                         f"for {wh} (no shield model)")
            continue
        hits = (allocate_internals(world, sk["target"], roller)
                if res["leak"] > 0 else [])
        dz = [h["box"] for h in hits if h["result"] == "destroyed"]
        note = f", destroyed {', '.join(dz)}" if dz else ""
        lines.append(f"{sk['label']} ({sk['kind']}) HIT {sk['target']}: {wh} on "
                     f"#{res['shield']} ({res['absorbed']:g} absorbed, "
                     f"{res['leak']:g} internal{note})")
    return lines


_PHASER_COST = {"phaser-1": 1.0, "phaser-2": 1.0, "phaser-4": 1.0, "phaser-3": 0.5,
                "phaser": 1.0}


def drain_capacitor(rec, weapon_token, n):
    """How many phasers can actually fire given the ship's capacitor, draining it.

    Returns the firable count (<= n). Non-phaser weapons or an unmodeled capacitor
    are not constrained (returns n). H6.21: ph-3 costs 0.5, others 1.0.
    """
    tok = str(weapon_token or "").lower()
    if "phaser" not in tok or "capacitor" not in rec:
        return n
    cost = _PHASER_COST.get(tok, 1.0)
    if cost <= 0:
        return n
    affordable = int(rec["capacitor"] // cost)
    fire = min(n, affordable)
    rec["capacitor"] = round(rec["capacitor"] - fire * cost, 2)
    return fire


def end_of_turn(world):
    """MUTATOR (6E3 / turn boundary): reload/repair between turns. Currently the
    phaser capacitor refills toward capacity (H6.1: you refill what you fired;
    modelled as topping to max, which the next EAF pays for - flagged). Drone-rack
    reload (E1.50) and damage-control repair (D9) attach here once rack/repair
    state is tracked. Returns a per-ship report.
    """
    lines = []
    for lab, rec in world.items():
        if "capacitor" in rec and rec["capacitor"] < rec["capacitor_max"]:
            refilled = round(rec["capacitor_max"] - rec["capacitor"], 2)
            rec["capacitor"] = rec["capacitor_max"]
            lines.append(f"{lab}: phaser capacitor +{refilled:g} -> "
                         f"{rec['capacitor_max']:g} (H6.1)")
        # Drone-rack rate-of-fire resets each turn (the 8/12-impulse gap is a
        # cross-turn refinement, flagged; last_impulse is cleared here).
        for rack in rec.get("racks") or []:
            if rack["fired_this_turn"]:
                rack["fired_this_turn"] = 0
                rack["last_impulse"] = None
                lines.append(f"{lab}: {rack['label']} rack ready ({rack['ammo']} left)")
    return lines


def release_esg(rec, radius, charges=None):
    """MUTATOR: raise this ship's ESG as a sphere of `radius`, a one-shot
    depleting pool (G23.222 activation discharges ALL energy; G23.511 the field
    is a depleting pool). The pool = the combined field strength at that radius
    across the ship's generators (G23.512 same-ship fields pool). Returns pool.
    """
    ch = charges if charges is not None else (rec.get("esg_charges") or [])
    try:
        import sfb_actions as ACT
        pool = ACT.esg_combined_field(ch, int(radius))
    except Exception:
        pool = 0
    rec["esg_active"] = {"radius": int(radius), "pool": float(pool), "engaged": []}
    return pool


def resolve_esg_vs_seekers(world, seekers):
    """MUTATOR (6A3.01 / G23.5): each active ESG sphere engages the seekers within
    its radius, once per seeker (G23.511 one hit per unit). The depleting pool
    spends the drone's kill value per drone until exhausted; a partial hit (pool <
    kill) damages the drone so later fire can finish it. Plasma is not killed this
    way in the slice (flagged). Returns a per-hit report; clears a spent sphere.
    """
    lines = []
    for lab, rec in world.items():
        esg = rec.get("esg_active")
        if not esg or esg["pool"] <= 0:
            continue
        R = esg["radius"]
        for sk in seekers:
            if not sk.get("alive") or sk["label"] in esg["engaged"]:
                continue
            if H.hex_distance((rec["x"], rec["y"]), (sk["x"], sk["y"])) > R:
                continue
            kill = seeker_kill_value(sk)
            if kill >= 9999:                       # plasma - not phaser/ESG-killed here
                esg["engaged"].append(sk["label"])
                lines.append(f"{lab} ESG r{R}: {sk['label']} is plasma - not killed "
                             f"by the field in this slice (flagged)")
                continue
            dmg = min(esg["pool"], kill)
            status, taken, kv = damage_seeker(seekers, sk["label"], dmg)
            esg["pool"] -= dmg
            esg["engaged"].append(sk["label"])
            lines.append(f"{lab} ESG r{R} hits {sk['label']}: {dmg:g} dmg ({status}); "
                         f"pool {esg['pool']:g} left")
            if esg["pool"] <= 0:
                break
        if esg["pool"] <= 0:
            rec["esg_active"] = None               # one-shot field spent
    return lines


def apply_energy_balance(rec):
    """MUTATOR (6A3.11 / D22): re-derive a ship's capability from its SURVIVING
    boxes. Destroyed power plant can no longer sustain the plotted speed, and box
    losses can cross the crippling thresholds. Returns a change dict or None.

    D22 speed cap: movement runs on the warp + impulse boxes; a ship cannot move
    faster than that plant sustains. max_speed = floor((warp+impulse)/move_cost).
    This only bites under real damage (undamaged plant far exceeds any speed).
    It is a PLANT ceiling, not a full energy-budget solve - other commitments
    (weapons, shields) are not yet subtracted; see the energy-tracking debt.

    Crippling S2.41 (exact here, using boxes_max as the true original):
      A: surviving warp <= 10% of original warp, OR
      B: >= 50% of interior boxes destroyed.
    """
    inv = rec.get("boxes")
    if not inv:
        return None
    invmax = rec.get("boxes_max") or inv
    changes = {}

    # --- crippling (S2.41)
    orig_total = sum(invmax.values())
    cur_total = sum(inv.values())
    interior_destroyed = ((orig_total - cur_total) / orig_total) if orig_total else 0.0
    warp0 = invmax.get("Warp", 0)
    warp = inv.get("Warp", 0)
    crippled_a = bool(warp0) and warp <= warp0 * 0.1
    crippled_b = interior_destroyed >= 0.5
    if (crippled_a or crippled_b) and not rec.get("crippled"):
        rec["crippled"] = True
        rec["crippled_reason"] = ("<=10% warp (S2.41A)" if crippled_a
                                  else ">=50% interior destroyed (S2.41B)")
        changes["crippled"] = rec["crippled_reason"]

    # --- D22 speed ceiling from the surviving movement plant
    mc = rec.get("move_cost") or 1.0
    move_power = inv.get("Warp", 0) + inv.get("Impulse", 0)
    max_speed = int(move_power // mc) if mc > 0 else move_power
    if rec["speed"] > max_speed:
        changes["speed"] = {"from": rec["speed"], "to": max_speed,
                            "plant": move_power}
        rec["speed"] = max_speed

    return changes or None
