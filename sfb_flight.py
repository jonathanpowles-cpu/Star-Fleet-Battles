"""
Orders for airborne fighters, issued BY FLIGHT rather than by airframe.

Eight fighters given eight blocks would swamp the ship orders they exist to
support, and would mostly repeat themselves. Fighters launched together from the
same bay share a position, a heading and a lockout clock, so the flight is the
natural unit of command - and the EXCEPTIONS deserve their own lines.

A fighter is broken out only where it genuinely differs:
  * damaged where the rest are not
  * straggling from the flight centre
  * its own lockout has not expired while the flight's has - each fighter's
    lockout runs from ITS OWN launch impulse, so a staggered launch produces a
    flight that becomes able to shoot in stages, not all at once

Rules shaping the orders:
  J1.342  a launched fighter cannot fire direct-fire weapons for 8 impulses
  J1.341  ...nor launch or guide drones for 16
  J1.61   a fighter cannot land while its ship moves faster than the fighter
"""
from __future__ import annotations

FIRE_LOCKOUT = 8         # J1.342, from this fighter's own launch impulse
DRONE_LOCKOUT = 16       # J1.341
STRIKE_RANGE = 3         # inside this, a fighter's own weapons are worth using

# Attack shuttles carry drones on launch rails but CANNOT control them: R5.F2 /
# J4.24 - "these must be controlled by the ship or another unit." The AAS is, in
# the designers' words, a manned recoverable scatter-pack. So a fighter drone
# launch spends the MOTHERSHIP's control channels (F3.21), not the fighter's,
# and that - not the rail count - is usually the binding limit on the salvo.
ATTACK_SHUTTLE_RAILS = {          # drones carried, by airframe (Kzinti MSSB R5.F2)
    "AAS": 2, "SAS": 2, "HAAS": 2, "TAAS": 2, "DAS": 2, "LKF": 4,
}


def _abs_imp(turn, imp, per_turn=32):
    return (turn - 1) * per_turn + imp


def _launched_at(craft):
    """(turn, impulse) this craft launched, from the client's '1.4' stamp."""
    raw = str(craft.get("launched") or "").strip()
    if "." in raw:
        a, _, b = raw.partition(".")
        try:
            return int(a), int(b)
        except ValueError:
            pass
    return None


def lockouts(craft, turn, impulse):
    """(can_fire, can_drone, impulses_until_fire) for one fighter.

    Each fighter's lockout runs from its OWN launch, which is why a staggered
    launch cannot be treated as a single event: the first pair off the deck is
    free to shoot while the last pair is still locked.
    """
    at = _launched_at(craft)
    if at is None:
        return True, True, 0
    since = _abs_imp(turn, impulse) - _abs_imp(*at)
    return (since >= FIRE_LOCKOUT, since >= DRONE_LOCKOUT,
            max(0, FIRE_LOCKOUT - since))


def flights(state, ship=None, side=None):
    """Group airborne fighters into flights.

    Keyed by (launching ship, airframe, launch impulse): craft that left the
    same bay together are the ones that manoeuvre and shoot together.
    """
    try:
        import sfb_airborne as AB
    except Exception:
        return []
    craft = AB.airborne(state, ship) if ship is not None else AB.airborne(state)
    craft = [c for c in craft if AB.is_fighter(c)]
    if side:
        craft = [c for c in craft if (c.get("race") or "").upper() == side.upper()]
    # Keyed by launcher + airframe only, NOT by launch impulse. A carrier that
    # cycles its bay puts pairs up two impulses apart, and keying on the stamp
    # split nine fighters into seven "flights" of one and two - which is exactly
    # the per-airframe noise this module exists to avoid. The staggered lockouts
    # that result are reported as exceptions within the flight instead.
    groups = {}
    for c in craft:
        groups.setdefault((c.get("launched_by"), c.get("shuttle_type")), []).append(c)
    return list(groups.values())


def _centre(craft):
    return (sum(c.get("x", 0) for c in craft) / len(craft),
            sum(c.get("y", 0) for c in craft) / len(craft))


def _drone_order(group, kind, can_drone, tgt, rng, mothership, free_channels):
    """The fighter-drone-launch line, or None. Attack shuttles only.

    Rails give the ammunition; the mothership's free control channels (F3.21)
    give the real limit, because an attack shuttle cannot guide its own drones
    (R5.F2). A fighter that has already emptied its rails (save flag
    launched_drones) is not re-ordered.
    """
    rails = ATTACK_SHUTTLE_RAILS.get((kind or "").upper())
    if not rails:
        return None                         # not a drone-carrying attack shuttle
    loaded = [c for c in can_drone
              if str(c.get("launched_drones")).lower() != "true"]
    if not loaded:
        return ("fighters: rails EMPTY - drones already launched",
                [f"each {kind} carries {rails} drone(s) on its rails (R5.F2); "
                 f"these flights have shot them - they are now recoverable "
                 f"scatter-packs with nothing left to throw"])
    onboard = len(loaded) * rails
    if free_channels is None:
        firing = onboard
        chan_note = ("mothership control channels unknown - the ship must have a "
                     "free channel (F3.21) for EACH drone; an attack shuttle "
                     "cannot guide its own (R5.F2/J4.24)")
    else:
        firing = min(onboard, free_channels)
        chan_note = (f"{mothership} has {free_channels} free control channel(s) "
                     f"(F3.21); an attack shuttle CANNOT guide its own drones "
                     f"(R5.F2), so that caps the salvo at {firing} even though "
                     f"{onboard} are on the rails")
    if rng is not None and rng > 16:
        return (f"fighters HOLD drones - target {rng} hexes, past a drone's reach",
                [chan_note, "close first; a speed-8 drone launched now is wasted"])
    if firing <= 0:
        return ("fighters HOLD drones - mothership has NO free control channels",
                [chan_note, "the drones would fly uncontrolled - launch only when "
                 "the ship frees a channel (a current seeker hits or is killed)"])
    return (f"LAUNCH {firing} DRONE(S) from the {kind} flight at "
            f"{tgt['label'] if tgt else 'target'}",
            [chan_note,
             f"{len(loaded)} shuttle(s) x {rails} rail(s) = {onboard} available; "
             f"controlled by {mothership}",
             "R5.F2 doctrine: attack shuttles exist to raise the ship's drone "
             "LAUNCH RATE - saturate now, but they exhaust fast and cannot rearm "
             "in flight"])


def flight_order(group, enemies, turn, impulse, mothership=None, free_channels=None):
    """(headline, [reasons], [exception lines]) for one flight."""
    import sfb_hex as H

    n = len(group)
    kind = group[0].get("shuttle_type") or "fighter"
    cx, cy = _centre(group)
    ctr = (round(cx), round(cy))

    tgt, rng = None, None
    for e in enemies or []:
        d = H.hex_distance(ctr, (e["x"], e["y"]))
        if rng is None or d < rng:
            tgt, rng = e, d

    if tgt is None:
        return f"{n}x {kind} FLIGHT: no target on the board", ["hold formation"], []

    can_fire = [c for c in group if lockouts(c, turn, impulse)[0]]
    can_drone = [c for c in group if lockouts(c, turn, impulse)[1]]
    soonest = min((lockouts(c, turn, impulse)[2] for c in group), default=0)

    why = []
    if rng > STRIKE_RANGE:
        head = f"{n}x {kind} FLIGHT: CLOSE on {tgt['label']} - range {rng}"
        why.append(f"fighter weapons are short-ranged; get inside {STRIKE_RANGE} "
                   f"before spending them")
    else:
        head = f"{n}x {kind} FLIGHT: ATTACK {tgt['label']} - range {rng}"

    if len(can_fire) == n:
        why.append(f"all {n} are clear of the J1.342 lockout and may fire")
    elif can_fire:
        why.append(f"only {len(can_fire)} of {n} may fire - J1.342 runs 8 impulses "
                   f"from each fighter's OWN launch; the rest clear in {soonest}")
    else:
        why.append(f"NONE may fire yet - J1.342 clears in {soonest} impulse(s); "
                   f"close the range meanwhile")

    if len(can_drone) < n:
        why.append(f"{n - len(can_drone)} still inside the J1.341 16-impulse drone "
                   f"lockout - they cannot launch or guide drones")

    # The drone-launch order, if this is a drone-carrying attack shuttle flight.
    drone_line = _drone_order(group, kind, can_drone, tgt, rng,
                              mothership or "the mothership", free_channels)
    if drone_line:
        head = head + "  ||  " + drone_line[0]
        why = drone_line[1] + why

    spread = max((H.hex_distance(ctr, (c.get("x", 0), c.get("y", 0)))
                  for c in group), default=0)
    exceptions = []
    for c in group:
        tags = []
        if c.get("damage_taken"):
            tags.append(f"damaged ({c['damage_taken']})")
        d = H.hex_distance(ctr, (c.get("x", 0), c.get("y", 0)))
        if spread > 1 and d >= spread:
            tags.append(f"{d} hexes off the flight centre - rejoin")
        cf, _cd, w = lockouts(c, turn, impulse)
        if not cf and can_fire:
            tags.append(f"cannot fire for {w} more impulse(s)")
        if tags:
            exceptions.append(f"{c.get('label')}: " + "; ".join(tags))
    return head, why, exceptions


def _free_channels(mother, state, log):
    """Control channels the mothership has left (F3.21).

    Attack-shuttle drones are guided by the ship, so the ship's spare channels
    gate the fighter salvo - the SAME channels its own racks use. This defers to
    the one shared ledger in sfb_ew, so a rack launch and a fighter-drone launch
    in the same impulse cannot each ignore the other's channel use. The previous
    version keyed on a `launched_by` field that seeker records do not carry, so
    it always found zero guided and silently over-counted the free channels.
    """
    if not mother:
        return None
    try:
        import sfb_ew as EW
        return EW.free_channels(mother, state, log)
    except Exception:
        return None


def all_flight_orders(state, enemies, turn, impulse, side=None, ship=None, log=None):
    """Flight orders for every airborne group of one side.

    Each flight is tied back to its launching ship so a drone-launch order can be
    capped by that ship's free control channels - an attack shuttle cannot guide
    its own drones (R5.F2).
    """
    by_id = {s.get("id"): s for s in (state or {}).get("ships") or []}
    out = []
    for g in flights(state, ship=ship, side=side):
        mother = by_id.get(g[0].get("launched_by")) if g else None
        mname = mother.get("label") if mother else None
        head, why, exc = flight_order(g, enemies, turn, impulse,
                                      mothership=mname,
                                      free_channels=_free_channels(mother, state, log))
        # Return the home carrier's label too, so the UI can file each flight
        # under its mothership rather than orphaning it beneath whatever ship
        # block happened to precede it.
        out.append((mname, head, why, exc))
    return out
