"""
Everything you DO on an impulse besides move.

The movement director answers "where do I go this impulse". This answers the
rest of the impulse procedure: fire, launch fighters, launch drones, raise the
ESG, run point defence. Each entry is (headline, [reasons]) so the UI can show
them in the same prominent band as the movement order.

Rules that decide the timing, all cited inline:
  E3.24   disruptors cannot hold a charge across the turn break - use or lose
  E1.50   universal 8-impulse reload between shots from the same weapon
  FD1.51  phasers are UNPENALISED against drones; heavy weapons take +4 ECM
  E1.7    drones gain +2 ECM at 10-19 hexes, +4 at 20+ - kill them inside 9
  J1.50   one shuttle per bay per two impulses (doubled by tunnel decks, J1.58)
  J1.341  a launched fighter cannot guide drones for 16 impulses
  J1.342  ...nor fire direct-fire weapons for 8
  G23.48  raising an ESG VOIDS a wild weasel
  G23.81  the ESG does nothing against plasma, PPD or direct fire
"""
from __future__ import annotations

ESG_RADIUS_BAND = (1, 3)          # G23: sphere radius when raised
RELOAD_IMPULSES = 8               # E1.50


def _abs_imp(turn, imp, per_turn=32):
    return (turn - 1) * per_turn + imp


# E2.151: a gatling phaser may fire up to four times in a turn - the one
# exception to the once-per-turn half of E1.50.
GATLING_SHOTS_PER_TURN = 4


def mount_status(ship, fam, turn, impulse, log, shots_per_turn=1):
    """(ready, cycling, spent) mounts for one weapon family.

    E1.50 verbatim: "No weapon may be fired twice within a period of one-fourth
    of a turn... This rule is NOT to be interpreted as meaning that a weapon can
    be fired more than once per turn."

    That is TWO constraints and the engine only had the first. A mount that
    fired on impulse 2 is free of the 8-impulse gap by impulse 10, but it still
    cannot fire again until the next turn. Reporting it as ready would promise a
    volley that does not exist.
    """
    now = _abs_imp(turn, impulse)
    mounts = ((log or {}).get("units") or {}).get(ship.get("label"), {}).get(fam, {})
    cycling, spent = {}, []
    for wid, st in mounts.items():
        gap = now - _abs_imp(*st)
        if gap < RELOAD_IMPULSES:
            cycling[wid] = RELOAD_IMPULSES - gap
        elif st[0] == turn and shots_per_turn <= 1:
            spent.append(wid)          # gap satisfied, but already fired this turn
    return cycling, spent


# The client's own ESG CHART (read off the in-game chart panel): field strength
# by radius and energy invested. The parenthesised figures on the chart are the
# per-point value at each radius - 4.00 at radius 0 falling to 3.00 at radius 3.
# A bigger sphere is a weaker sphere, and that trade is the whole raise decision.
# Columns x6/x7 exist on the chart but exceed the G23.22 five-point storage cap;
# recorded as printed, not interpreted.
ESG_CHART = {
    0: [4, 8, 12, 16, 20, 24, 28],
    1: [4, 7, 11, 15, 18, 22, 26],
    2: [3, 7, 10, 13, 17, 20, 23],
    3: [3, 6, 9, 12, 15, 18, 21],
}


def esg_field_strength(energy, radius):
    """Field strength for whole points of energy at a radius (client ESG chart)."""
    row = ESG_CHART.get(int(radius))
    e = int(energy)
    if not row or e < 1:
        return 0
    return row[min(e, len(row)) - 1]


ESG_MAX_STORE = 5      # G23.22: up to five points per generator
ESG_EA_RATE = 3        # what the EA advice books per generator per turn

# SCENARIO START, per the player: THIS battle defined no Weapon Status, and in
# practice only the phaser capacitors were charged at the start - ESGs began
# EMPTY, disruptors empty (they cannot hold anyway), batteries full (a standing
# reserve, not affected by weapon status). So the ESG initial charge is 0, NOT
# the WS-II value of 2 the engine had been assuming off a column-drifted G23.23
# read. That assumption overstated turn-1 ESG strength by two points (field 20
# vs the real 12 at radius 0). Change this constant if a future scenario sets a
# real Weapon Status.
ESG_INITIAL_CHARGE = 0
SCENARIO_START_NOTE = ("this game: no Weapon Status defined - phaser capacitors "
                       "started FULL, ESGs EMPTY, batteries full")

def esg_charges_from_eaf(ship):
    """Per-generator ESG charge read STRAIGHT from the client EAF, or None.

    The client stores the real allocation (StateDump surfaces it as ship['eaf'],
    a list of per-turn {row-label: value}). Accumulate the 'ESG (A)', 'ESG (B)'…
    allocations across turns - that is the stored charge - capped at G23.22's
    five. This is ground truth; it retired the hand-maintained override table AND
    was more accurate (the Feral's real charge is 3.17, not the 4 entered by
    hand). Fractional points are kept; esg_field_strength floors to whole units
    itself (G23.223).
    """
    import re
    turns = ship.get("eaf") or []
    if not turns:
        return None
    # Which generators exist (ESG (A), (B), ...) across all turns.
    letters = sorted({m.group(1) for row in turns for k in row
                      if (m := re.match(r"ESG \(([A-Z])\)$", k))})
    if not letters:
        return None
    gens = {L: 0.0 for L in letters}

    # CHRONOLOGICAL: walk turn by turn so a generator that is FIRED and later
    # RE-CHARGED is handled correctly - the fire zeroes only the charge held at
    # that point, and allocation in a later turn builds fresh. Accumulating
    # everything and then zeroing would wrongly wipe a post-fire recharge.
    fires = sorted(ship.get("esg_fires") or [],
                   key=lambda f: (f.get("t", f.get("turn", 0)), f.get("i", 0)))
    fi = 0
    for tidx, row in enumerate(turns, start=1):
        def num(k):
            try:
                return float(row.get(k, 0) or 0)
            except (TypeError, ValueError):
                return 0.0
        for L in letters:
            gens[L] += num(f"ESG ({L})")
        # Reserve/battery power spent THIS turn is booked separately ('Reserve
        # Power Used'), not in the ESG row - for a Lyran facing drones it boosts
        # the spheres, so fold it into a generator toward the G23.22 cap of five.
        reserve = max(0.0, num("Reserve Power Used") - num("Recharge Btty/Reserve Warp")
                      - num("Recharge Btty/Reserve Imp."))
        while reserve > 0.01 and any(gens[L] < ESG_MAX_STORE for L in letters):
            L = max((x for x in letters if gens[x] < ESG_MAX_STORE), key=lambda x: gens[x])
            add = min(reserve, ESG_MAX_STORE - gens[L])
            gens[L] += add
            reserve -= add
        for L in letters:
            gens[L] = min(ESG_MAX_STORE, round(gens[L], 2))
        # Fires that happened in this turn (G23.222: activation discharges the
        # WHOLE generator). Zero the generator whose charge matches the released
        # energy - after the recharge for this turn, so an in-turn top-up-then-
        # fire still ends spent.
        while fi < len(fires) and fires[fi].get("t", fires[fi].get("turn", 0)) <= tidx:
            energy = esg_energy_from_field(fires[fi].get("strength", 0),
                                           fires[fi].get("radius", 0))
            lit = [L for L in letters if gens[L] > 0.01]
            if lit:
                L = min(lit, key=lambda x: abs(gens[x] - energy))
                gens[L] = 0.0
            fi += 1
    return [gens[L] for L in letters]


def battery_discharge_from_eaf(ship):
    """Battery energy spent (reserve power used minus any recharge), from the EAF."""
    turns = ship.get("eaf") or []
    used = recharged = 0.0
    for row in turns:
        for k, v in row.items():
            try:
                f = float(v)
            except (TypeError, ValueError):
                continue
            if k in ("Reserve Power Used", "Battery Discharged"):
                used += f
            elif k.startswith("Recharge Btty"):
                recharged += f
    return max(0.0, round(used - recharged, 2))


def esg_charges(ship, turn, initial=ESG_INITIAL_CHARGE):
    """(list of per-generator charges, note).

    Reads the ACTUAL charge from the client EAF (ground truth). Falls back to a
    uniform projection only when no EAF data is present.
    """
    n = (ship.get("weapons") or {}).get("esg", [0, 0])[0]
    live = esg_charges_from_eaf(ship)
    if live:
        charges = live[:n] if n else live
        note = (f"ESG charge {charges} per generator - read from the client EAF "
                f"(accumulated ESG allocation across turns, G23.223: whole units used)")
        return charges, note
    per = min(ESG_MAX_STORE, initial + ESG_EA_RATE * max(0, int(turn)))
    charges = [per] * max(1, n)
    note = (f"projected {per}/{ESG_MAX_STORE} per generator (no EAF data yet): "
            f"{initial} initial ({SCENARIO_START_NOTE}) + {ESG_EA_RATE}/turn x "
            f"{turn} - capped at {ESG_MAX_STORE} (G23.22)")
    return charges, note


def esg_energy_from_field(strength, radius):
    """Reverse the client ESG chart: field strength at a radius -> energy that
    produced it. Used to work out how much charge an activation discharged."""
    row = ESG_CHART.get(int(radius))
    if not row:
        return 0
    for e, val in enumerate(row, start=1):
        if val == int(strength):
            return e
    return min(range(1, len(row) + 1), key=lambda e: abs(row[e - 1] - int(strength)))


def _apply_esg_fires(charges, fires):
    """Zero the generators that have been RELEASED (G23.222: activation discharges
    ALL of a generator's energy). Each fire's field strength tells us how much
    energy went out; empty generators strongest-first to match it."""
    for ev in fires or []:
        energy = esg_energy_from_field(ev.get("strength", 0), ev.get("radius", 0))
        for i in sorted(range(len(charges)), key=lambda k: -charges[k]):
            if energy <= 0.01:
                break
            if charges[i] <= energy + 0.5:      # this generator fired - it is spent
                energy -= charges[i]
                charges[i] = 0.0
    return charges


def esg_combined_field(charges, radius):
    """Total field strength at a radius across all a ship's generators.

    G23.512/G23.75: two or more ESG fields from the SAME ship, at the same or
    different radii, count as a SINGLE VOLLEY - so their strengths pool. The old
    advice used one generator's field and silently halved a two-ESG ship's pool.
    """
    return sum(esg_field_strength(c, radius) for c in charges)


ESG_ANNOUNCE_LEAD = 4      # G23.31: release must be announced 4 impulses ahead


def _esg_release_decision(ship, threats, esg_kill_capacity):
    """(release_now, note): save the one-shot ESG for the turn phasers can't handle.

    Drones strung out behind a ship arrive over several turns. Phasers recharge
    every turn (FD1.51); the ESG is one-shot (G23.222). So release it on the turn
    the arrivals would OVERWHELM phasers - not on the leading drone or two, which
    are ordinary point-defence work.
    """
    import math
    w = ship.get("weapons") or {}
    nph = sum(w.get(f, [0, 0])[0] for f in
              ("phaser-1", "phaser-2", "phaser-3", "phaser-4", "phaser"))
    buckets = {}
    for t in threats:
        tti = t.get("turns_to_impact")
        if tti is None:
            continue
        buckets.setdefault(max(1, math.ceil(tti)), []).append(t)
    if not buckets:
        return True, ""            # no schedule info - fall back to window logic
    imminent_turn = min(buckets)
    imminent = len(buckets[imminent_turn])
    peak_turn = max(buckets, key=lambda k: len(buckets[k]))
    peak = len(buckets[peak_turn])
    if peak <= nph:
        # Phasers alone handle even the WORST single turn - the ESG is not needed.
        # Hold a one-shot field you do not have to spend (G23.221: keeps 25 turns).
        return False, (f"HOLD: the heaviest single turn is {peak} drone(s), and your "
                       f"{nph} phasers stop that alone (FD1.51) - keep the ESG in "
                       f"reserve rather than spend a one-shot field you don't need. "
                       f"(If you distrust the phaser rolls, releasing is fair insurance.)")
    if imminent <= nph and peak_turn > imminent_turn:
        # The trickle is phaser work; the real threat is a later concentration.
        return False, (f"HOLD: {imminent} drone(s) arrive first - your {nph} phasers "
                       f"stop that alone (FD1.51). The concentration is {peak} drones "
                       f"on turn +{peak_turn}, which WOULD overwhelm phasers - keep the "
                       f"one-shot field (G23.222) for that wave.")
    return True, (f"RELEASE: {imminent} drone(s) imminent vs {nph} phasers - they "
                  f"exceed point-defence, so the ESG's ~{esg_kill_capacity} extra "
                  f"kills are needed now")


def esg_radius_advice(ship, charges, threats, enemy_ships):
    """Recommend an ESG radius, and show the pool-vs-reach trade at each.

    `charges` is the list of per-generator charges; same-ship generators pool
    into one volley (G23.512), so the field at a radius is the SUM across them.

    The decision is governed by G23.511: the field is a DEPLETING POOL - it
    scores damage up to what kills a thing OR the field strength, whichever is
    lower, and every point spent reduces the field. A unit is hit ONCE, on
    entering (G23.51). So the total number of drones a field kills is just
    strength / kill-cost, and since a smaller sphere is a STRONGER sphere
    (G23.41, client chart), a small radius kills MORE.

    Bearing-spread does not enter into it: every inbound drone must cross every
    shell on its way to the ship, so a wide sphere gains no extra interceptions -
    only a smaller pool. Radius is therefore chosen for POOL SIZE against drones,
    and only widened for REACH reasons: to ram an enemy ship, or to catch
    fighters loitering at a range that a tight sphere would not touch.

    threats: incoming_seekers-style dicts with 'range','kill'(,'warhead').
    enemy_ships: dicts with x/y, to spot a ram opportunity.
    """
    # kill cost per threat: standard drone 4, heavy 6 (its 'kill' field), else 4.
    costs = sorted((t.get("kill") or 4) for t in threats)
    total_cost = sum(costs)

    def kills_at(r):
        pool = esg_combined_field(charges, r)     # G23.512: generators pool
        got, spent = 0, 0
        for c in costs:                       # cheapest first = most kills
            if spent + c <= pool:
                spent += c
                got += 1
            else:
                break
        return got, pool

    table = {r: kills_at(r) for r in range(4)}
    n = len(costs)

    # A ram opportunity: an enemy SHIP within radius 3, whose facing shield the
    # field could damage (G23.513). Only relevant if we are willing to close.
    import sfb_hex as H
    me = (ship["x"], ship["y"])
    ram = None
    for e in enemy_ships or []:
        d = H.hex_distance(me, (e.get("x", 0), e.get("y", 0)))
        if d <= 3 and (ram is None or d < ram[1]):
            ram = (e, d)

    lines = [f"radius choice (G23.511 - the field is a depleting pool, one hit per "
             f"target on entry):"]
    for r in range(4):
        got, pool = table[r]
        lines.append(f"  r{r}: strength {pool} -> kills ~{got} of {n} inbound"
                     + ("  <-- most" if r == 0 and n else ""))

    if not costs and ram:
        rec = ram[1]
        return rec, [f"no drones inbound; an ESG can RAM {ram[0]['label']} at range "
                     f"{ram[1]} - set radius {ram[1]} to reach its hex (G23.513: "
                     f"lands on its facing shield, does not bypass it)"] + lines

    # Anti-drone: smallest radius, biggest pool. Recommend r0/r1.
    best = max(range(4), key=lambda r: table[r][0])
    # prefer r1 over r0 when they kill the same number: a 1-hex standoff kills the
    # drone one hex out instead of on the hull, and keeps the ship's own hex clear
    # of the field for a landing shuttle.
    if table.get(1, (0,))[0] == table[best][0] and best == 0:
        best = 1
    rec = best
    verdict = [f"RECOMMEND radius {rec}: against a drone wave the field is a pool, "
               f"and a tighter sphere is a bigger pool (G23.41) - it kills the most "
               f"({table[rec][0]} of {n})"]
    if total_cost > table[0][1]:
        verdict.append(f"WARN the wave's total kill-cost is {total_cost} but even "
                       f"the strongest field (r0) holds {table[0][1]} - the ESG "
                       f"CANNOT stop all {n}; layer phasers/ADD behind it")
    if ram:
        verdict.append(f"alternative: {ram[0]['label']} is at range {ram[1]} - a "
                       f"radius-{ram[1]} sphere would RAM it instead, but spends the "
                       f"whole charge on one ship and lets the drones through")
    return rec, verdict + lines


def esg_actions(ship, rng, cmd, turn=1, state=None):
    esg = (ship.get("weapons") or {}).get("esg", [0, 0])[0]
    if not esg:
        return []
    lo, hi = ESG_RADIUS_BAND
    charges, chg_note = esg_charges(ship, turn)
    per = max(charges) if charges else 0        # strongest single generator, for display

    # The trigger is INBOUND DRONES AND FIGHTERS, not the enemy SHIP. The ESG is
    # an anti-seeker/anti-fighter weapon (G23.81: nothing against direct fire),
    # so the mothership's own distance is irrelevant - what matters is when a
    # seeker or fighter will ENTER the sphere. And G23.31 requires the release to
    # be announced FOUR impulses in advance, so the decision point is when the
    # nearest threat is within (radius + its 4-impulse approach).
    threats = []
    if state is not None:
        try:
            threats = cmd.incoming_seekers(ship, state) or []
        except Exception:
            threats = []
    # fighters inside a short reach are threats too (they get scythed, G23)
    ftr_rng = None
    if state is not None:
        try:
            import sfb_airborne as AB
            for c in AB.airborne(state):
                if (c.get("race") or "").upper() != (ship.get("race") or "").upper():
                    d = H.hex_distance((ship["x"], ship["y"]),
                                       (c.get("x", 0), c.get("y", 0)))
                    ftr_rng = d if ftr_rng is None else min(ftr_rng, d)
        except Exception:
            pass

    # closing seekers only - one that cannot catch us will not enter the sphere
    nearest = None
    for t in threats:
        if t.get("closing", 1) is None or t.get("closing", 1) > 0 or t["range"] <= hi + 1:
            nearest = t["range"] if nearest is None else min(nearest, t["range"])
    threat_rng = min([r for r in (nearest, ftr_rng) if r is not None], default=None)

    strengths = ", ".join(f"{esg_combined_field(charges, r)}@r{r}" for r in range(4))
    field_note = (f"combined field ({len(charges)} generator(s) at "
                  f"{'+'.join(str(c) for c in charges)} pts, pooled as one volley "
                  f"per G23.512): "
                  f"{', '.join(f'r{r}={esg_combined_field(charges, r)}' for r in range(4))} "
                  f"- radius 0 hugs the hull at full strength, radius 3 reaches "
                  f"further but a third weaker")
    static = ["G23.222: activation releases ALL stored energy - a half-charged "
              "sphere is half a sphere, and the charge cannot be split",
              "G23.81: no effect on plasma, PPD or direct fire - anti-drone, "
              "anti-fighter, anti-mine only",
              "WARN raising it VOIDS any wild weasel (G23.48) and damages anything "
              "inside the sphere, including our own fighters"]

    # No seeker/fighter threat: hold and keep charging, whatever the ship range.
    if threat_rng is None:
        return [(f"ESG: hold - {'+'.join(str(c) for c in charges)} pts across "
                 f"{len(charges)} generator(s), no seekers or fighters inbound",
                 [chg_note,
                  "the ESG does nothing to enemy SHIPS (G23.81), so ship range is "
                  "irrelevant - it waits for drones or fighters",
                  "G23.211: keep topping up at EA until each generator holds "
                  f"{ESG_MAX_STORE} - it accumulates and keeps 25 turns (G23.221)"])]

    # Within the announcement window. BUT the ESG is a ONE-SHOT pool (G23.222) and
    # phasers RECHARGE every turn, so releasing it against the first few drones is
    # usually wrong when a bigger concentration is coming - the trickle is phaser
    # work; the ESG earns its keep on the turn phasers would be overwhelmed.
    if threat_rng <= hi + ESG_ANNOUNCE_LEAD:
        enemy_ships = [e for e in (state or {}).get("ships", [])
                       if (e.get("race") or "").upper() != (ship.get("race") or "").upper()] \
            if state else []
        rec_r, rec_why = esg_radius_advice(ship, charges, threats, enemy_ships)
        esg_cap = esg_combined_field(charges, rec_r) // 4     # rough drones-killed
        release, dec_note = _esg_release_decision(ship, threats, esg_cap)
        if not release:
            return [(f"ESG: HOLD at {'+'.join(str(c) for c in charges)} pts - do NOT "
                     f"release yet ({esg} generator(s), field "
                     f"{esg_combined_field(charges, 0)}@r0)",
                     [dec_note, chg_note,
                      "G23.221: the charge keeps for 25 turns, so holding costs "
                      "nothing; phasers point-defend the trickle meanwhile (FD1.51)",
                      "announce the release ~4 impulses before the concentrated wave "
                      "reaches the sphere (G23.31)"])]
        return [(f"ANNOUNCE ESG RELEASE NOW at RADIUS {rec_r} - {esg} generator(s) "
                 f"({'+'.join(str(c) for c in charges)} pts = "
                 f"{esg_combined_field(charges, rec_r)} strength)",
                 [dec_note,
                  f"nearest drone/fighter at {threat_rng} - G23.31 requires the "
                  f"release be announced {ESG_ANNOUNCE_LEAD} impulses in advance, "
                  f"so the sphere is up before it enters"]
                 + rec_why + [chg_note] + static)]

    # Threat exists but is still beyond the window: hold, ready to announce.
    return [(f"ESG: hold - ~{per}/{ESG_MAX_STORE} per gen, nearest threat {threat_rng} "
             f"(announce at {hi + ESG_ANNOUNCE_LEAD})",
             [chg_note,
              f"a seeker/fighter is inbound but at {threat_rng} it is outside the "
              f"radius {lo}-{hi} plus the {ESG_ANNOUNCE_LEAD}-impulse G23.31 lead - "
              f"announce when it reaches {hi + ESG_ANNOUNCE_LEAD}"])]


def fighter_actions(ship, impulse, rng, cmd, turn=1, log=None,
                    mission=None, consort=None, state=None):
    """Launch guidance - which differs sharply by WHY the ship carries fighters.

    A carrier's group is its weapon and wants to be up early (the deck takes
    impulses to clear, and each fighter is locked out for 8-16 impulses after
    ITS OWN launch). An ESCORT's two or three fighters are not a strike: they
    are close-in defence for the carrier, and G33.42 is explicit that a carrier
    must have an escort assigned. Putting them up 25 hexes from the enemy just
    strands them - they cannot land while the group moves faster than they do
    (J1.61) and they add nothing at that range.
    """
    sysd = ship.get("systems") or {}
    ftr = sysd.get("fighter", [0, 0])[0]
    if not ftr:
        return []
    t = cmd._base_type((ship.get("type") or "").upper())
    is_carrier = t in cmd.CARRIER_TYPES or ftr >= 8      # G33.42: 8+ qualifies as a carrier
    try:
        import sfb_carrier as CAR
        system = CAR.bay_system_for(ship.get("race"), ship.get("type"))
        per = 2 if system == "tunnel" else 1
        gap = CAR.BAY_IMPULSES_PER_OP
        fspd, _known = CAR.fighter_speed(ship.get("race"))
    except Exception:
        system, per, gap, fspd = None, 1, 2, 8

    # What is still aboard comes from the BOARD, not from a running tally of
    # launch events. Counting launches can only ever go down: a recovered
    # fighter never comes back, and one shot down is still counted as flying.
    # The save lists every airborne craft, so subtracting what is actually up is
    # self-correcting and needs no landing event.
    #
    # It also separates fighters from wild weasels, suicide shuttles and
    # scatter-packs by `mission`. Those share the shuttle boxes but are NOT
    # fighters, and debiting them from the fighter complement made a 2-fighter
    # escort report its fighters gone the moment it charged a weasel.
    remaining, out_now = ftr, 0
    try:
        import sfb_airborne as AB
        remaining, _cap = AB.fighters_aboard(state, ship)
        out_now = len(AB.fighters_out(state, ship))
    except Exception:
        pass

    # The bay-cycle timer (J1.50) still needs the last launch INSTANT, which the
    # board does not record beyond a coarse impulse stamp, so that one value
    # still comes from the log.
    last_launch = None
    for e in ((log or {}).get("events") or []):
        if (e.get("kind") == "launch" and e.get("ship") == ship["label"]
                and e.get("what") in (None, "shuttle", "fighter")):
            last_launch = _abs_imp(e.get("t", turn), e.get("i", 0))
    now = _abs_imp(turn, impulse)

    if not is_carrier:
        if remaining <= 0:
            return [(f"fighters: all {ftr} airborne - deck clear",
                     [f"{out_now} craft up, nothing left aboard to launch",
                      "J1.61: they cannot land while the ship moves faster than they do"])]
        # A handful of fighters on an escort or casual carrier.
        if rng > fspd * 2:
            return [(f"HOLD FIGHTERS ABOARD ({remaining}) - not a strike group",
                     [f"{ftr} fighters is close-in defence, not an attack (G33.42: 8+ "
                      f"fighters is what makes a carrier)",
                      f"at range {rng} a speed-{fspd} fighter cannot reach him and cannot "
                      f"land back while we move faster than it (J1.61) - it would simply "
                      f"be stranded",
                      "launch these when drones or fighters actually threaten the group"])]
        return [(f"LAUNCH {min(per, remaining)} FIGHTER(S) - close defence",
                 [f"range {rng} is inside a speed-{fspd} fighter's useful reach",
                  f"escort fighters exist to kill seekers and fighters aimed at the "
                  f"carrier, not to make an attack run"])]

    # A real carrier. The bay is ready when it has not launched within the last
    # `gap` impulses (J1.50) - NOT on a fixed parity of the impulse number. The
    # old parity test disagreed with the printed schedule and could never match
    # it, so the schedule said "imp 4: launch" while the order line said "no
    # launch this impulse". Read the actual launch history instead.
    if remaining <= 0:
        return [(f"fighters: group is up ({out_now} airborne) - deck clear",
                 ["D12.0 chain-reaction risk no longer applies with no armed "
                  "fighters aboard"])]
    if last_launch is None or (now - last_launch) >= gap:
        note = "J1.50: one per bay per two impulses"
        if system == "tunnel":
            note += "; tunnel decks run both hatches, doubling it (J1.58)"
        return [(f"LAUNCH {min(per, remaining)} FIGHTER(S) NOW ({remaining} still aboard)",
                 [note,
                  "each fighter's lockout runs from ITS OWN launch: 8 impulses before "
                  "it can fire (J1.342), 16 before it can use drones (J1.341)"])]
    wait = gap - (now - last_launch)
    return [(f"fighters: bay cycling - next launch in {wait} impulse(s)",
             [f"J1.50: a bay may not launch again within {gap} impulses; "
              f"{remaining} fighter(s) still aboard"])]


def drone_actions(ship, tgt, rng, cmd, turn=1, impulse=1, log=None,
                  state=None, enemies=None):
    drone = (ship.get("weapons") or {}).get("drone", [0, 0])[0]
    if not drone or not tgt:
        return []
    # A RACK's drone, not a seeker already in flight. drone_profile() reads
    # `speed` off whatever piece it is handed, so passing the ship made the
    # launch gate key off OUR OWN THROTTLE - slow down, and the engine decided
    # its drones had become slow weapons and refused to launch them.
    speed, spd_known = 8, False
    try:
        prof = cmd.rack_drone_profile(ship)
        speed, spd_known = prof.get("speed", 8), prof.get("known", False)
    except Exception:
        pass
    qual = "" if spd_known else " (assumed - no speed module in the save)"

    # Drone racks bear forward. Ordering a launch at a target astern is an
    # illegal order however good the range looks.
    #
    # The per-rack firing arc is NOT in the save file, so this gates on the
    # 180-degree forward arc rather than the narrower FA a specific rack may
    # actually have. That is deliberately conservative: it refuses only targets
    # that are unambiguously astern, so it can never suppress a legal launch.
    # Tighten it if per-rack arcs ever become readable.
    try:
        import sfb_hex as H
        import sfb_rules as R
        if not H.target_in_arc((ship["x"], ship["y"]), ship.get("facing", 0),
                               R.ARC_FWD, (tgt["x"], tgt["y"])):
            return [("DRONES: cannot bear - target is astern",
                     [f"drone racks launch forward; {tgt['label']} is behind us",
                      "TURN TO BEAR first, then launch - or hold the racks for a "
                      "target that is actually in front"])]
    except Exception:
        pass

    # E1.50 governs the rack as much as any other launcher: do not re-issue the
    # same rack every impulse while it is still cycling.
    fired = ((log or {}).get("fired") or {}).get(ship["label"], {})
    stamps = fired.get("drone") or []
    if stamps:
        since = _abs_imp(turn, impulse) - _abs_imp(*stamps[-1])
        if since < RELOAD_IMPULSES:
            return [("DRONES: rack cycling - cannot launch",
                     [f"launched T{stamps[-1][0]}.{stamps[-1][1]}; E1.50 requires "
                      f"{RELOAD_IMPULSES} impulses, {RELOAD_IMPULSES - since} to go"])]

    if prof.get("empty"):
        return [("DRONES: racks are EMPTY - nothing to launch",
                 ["every rack's ammunition list is exhausted in the save",
                  "reloading needs deck crew actions (J4.821) - see the deck crew panel"])]

    if rng <= speed * 2:
        load = ", ".join(f"rack {r['designator']}: {r['n']}x Type-{'/'.join(r['types'])}"
                         for r in prof.get("racks", []) if r["n"])

        # F3.21: "SHIPS can control a number of seeking weapons equal to their
        # SENSOR RATING at any given time (usually six)." The headline used to
        # quote the AMMUNITION count - "8 rounds in 2 racks" - which reads like
        # an order to launch eight, when the ship may only be able to guide four.
        # Ammunition is what you own; channels are what you can use.
        channels, aloft = None, 0
        try:
            import sfb_ew as EW
            channels = EW.sensor_rating(ship)[0]
            aloft = EW.guiding(ship, state, log)      # shared ledger (see sfb_ew.guiding)
        except Exception:
            pass

        # What this ship has ALREADY launched this turn. Rate of fire is a
        # per-turn allowance, so seekers currently on the board are the wrong
        # measure - one that has already hit or been shot down still consumed
        # its rack's shot. Without this the order repeated "LAUNCH 4 DRONES"
        # every impulse after the racks were spent.
        launched_this_turn, last_launch_imp = 0, None
        for e in ((log or {}).get("events") or []):
            if (e.get("kind") == "launch" and e.get("ship") == ship.get("label")
                    and e.get("what") == "drone" and e.get("t") == turn):
                launched_this_turn += 1
                li = _abs_imp(e.get("t", turn), e.get("i", 0))
                last_launch_imp = li if last_launch_imp is None else max(last_launch_imp, li)

        free = max(0, (channels - aloft)) if channels is not None else None
        stock = prof.get("rounds", drone)
        # FD3.x rate of fire is usually the BINDING limit, ahead of both
        # ammunition and control channels: a type-A rack fires one drone per
        # turn however full it is, so four type-A racks put four in the air, not
        # twelve. Take the smallest of the three.
        rof = prof.get("per_turn") or 0
        rof_left = max(0, rof - launched_this_turn) if rof else 0

        # FD3.0: "no drone rack can fire two drones within 1/4 turn of each
        # other, EVEN IF ON DIFFERENT TURNS." So a launch late in one turn can
        # still be blocking early in the next.
        gap_wait = 0
        if last_launch_imp is not None:
            gap = min((r.get("gap") or RELOAD_IMPULSES)
                      for r in (prof.get("racks") or [{}])) or RELOAD_IMPULSES
            since = _abs_imp(turn, impulse) - last_launch_imp
            if since < gap:
                gap_wait = gap - since

        if rof and rof_left <= 0:
            return [(f"DRONES: racks SPENT for this turn - {launched_this_turn} "
                     f"already launched",
                     [f"FD3.1: each rack fires once per turn; all {rof} shot(s) are "
                      f"used and the racks reload next turn",
                      f"{stock} round(s) remain in the magazine, but nothing can "
                      f"fire them until turn {turn + 1}"])]
        if gap_wait:
            return [(f"DRONES: rack cycling - {gap_wait} impulse(s) to go",
                     [f"FD3.0: no rack may fire two drones within a quarter turn "
                      f"of each other, even across the turn break",
                      f"{rof_left} shot(s) still available this turn once the gap "
                      f"clears"])]

        limits = [x for x in (stock, free, rof_left or None) if x is not None]
        n_now = min(limits) if limits else stock

        why = [f"range {rng} is within a speed-{speed} drone's reach{qual}"]
        if load:
            why.append(f"magazine: {load} ({stock} round(s) total)")
        if rof:
            why.append(f"FD3.x RATE OF FIRE: {rof} drone(s) per turn across "
                       f"{len(prof.get('racks', []))} rack(s) - a type-A fires ONE "
                       f"per turn however full it is (FD3.1), and no rack may fire "
                       f"twice within a quarter turn (FD3.0). This is usually the "
                       f"binding limit, not the magazine")
        if channels is not None:
            why.append(f"F3.21: control channels = sensor rating {channels}, "
                       f"{aloft} already under guidance, so {free} free")
        if prof.get("adds"):
            why.append(f"separately: {prof['adds']} ADD round(s) in "
                       f"{prof['add_racks']} box(es) - anti-drone DEFENCE (E5.0), "
                       f"not launchable at a ship, so not part of this salvo")
        if prof.get("heavy"):
            why.append(f"{prof['heavy']} HEAVY round(s) aboard (24-point warhead, "
                       f"6 damage to kill vs 4 for a standard) - spend these on the "
                       f"target you actually want dead, not on a ranging shot")
        why.append("p18: a target CLOSING on you is the better choice - less "
                   "reaction time for him, and the drone arrives sooner")

        if free is not None and free <= 0:
            return [(f"DRONES: HOLD - all {channels} control channels in use",
                     [f"F3.21: {aloft} seeker(s) already under guidance and the "
                      f"sensor rating is {channels}",
                      f"{stock} round(s) still in the racks, but nothing can guide "
                      f"them until a current seeker hits, is killed, or is released"]
                     + ([f"magazine: {load}"] if load else []))]

        # RACK-LEVEL TARGETING: which rack at which target. Heavy warheads want
        # the ship you mean to kill; cheap Type-Is are for saturating a target
        # that must burn phasers on them, and for stripping a Lyran ESG sphere
        # (each drone the sphere eats is one less for the heavies behind it).
        # Which racks are free to fire: the log names the rack in the drone label
        # (D003(3) = rack 3), so a rack that has already fired this turn is
        # excluded from the targeting suggestion.
        import re as _re
        spent_desig = set()
        for e in ((log or {}).get("events") or []):
            if (e.get("kind") == "launch" and e.get("ship") == ship["label"]
                    and e.get("what") == "drone" and e.get("t") == turn):
                m = _re.match(r"D\d+\((\d+)\)", e.get("unit") or "")
                if m:
                    spent_desig.add(m.group(1))
        ready_racks = [r for r in (prof.get("racks") or [])
                       if r.get("n") and str(r.get("designator")) not in spent_desig]
        aim = _rack_targeting(ready_racks, enemies or [tgt], tgt, ship, cmd)
        head = f"LAUNCH {n_now} DRONE(S) at {tgt['label']}"
        if aim:
            head = "LAUNCH DRONES - " + "; ".join(a[0] for a in aim)
            why = [a[1] for a in aim] + why
        return [(head, why)]


def _rack_targeting(ready_racks, enemies, default_tgt, ship, cmd):
    """(order-fragment, reason) per rack - warhead matched to target.

    Heavies at the highest-value / hardest target; standards at whoever is best
    saturated (closest, or an ESG ship whose sphere must be stripped first).
    """
    if not ready_racks:
        return []
    import sfb_hex as H
    me = (ship["x"], ship["y"])

    def val(e):                     # crude target value: bigger + closer = better
        sc = e.get("size_class") or 3
        rng = H.hex_distance(me, (e["x"], e["y"]))
        return (5 - sc) * 10 - rng
    hardest = max(enemies, key=val)
    esg_ship = next((e for e in enemies
                     if (e.get("weapons") or {}).get("esg", [0, 0])[0]), None)
    nearest = min(enemies, key=lambda e: H.hex_distance(me, (e["x"], e["y"])))

    out = []
    for r in ready_racks:
        types = r.get("types") or []
        heavy = any(t in ("IV", "V", "H") for t in types)
        if heavy:
            t = hardest
            why = (f"rack {r.get('designator')} ({'/'.join(types)}, HEAVY): at "
                   f"{t['label']} - a 24-pt warhead is spent on the target you "
                   f"want dead (needs 6 to kill vs a standard's 4)")
        elif esg_ship is not None:
            t = esg_ship
            why = (f"rack {r.get('designator')} ({'/'.join(types) or 'std'}): at "
                   f"{t['label']} - it runs an ESG; cheap drones strip the sphere "
                   f"(G23.4x: each drone the field eats is one it cannot spend on "
                   f"a heavy behind it)")
        else:
            t = nearest
            why = (f"rack {r.get('designator')} ({'/'.join(types) or 'std'}): at "
                   f"{t['label']} - closest, so least reaction time (p18)")
        out.append((f"rack {r.get('designator')}->{t['label']}", why))
    return out
    return [("HOLD DRONES",
             [f"range {rng} is beyond a speed-{speed} drone's useful reach; "
              f"launching now wastes them and ties up control channels (F3.21)"])]


def fire_actions(ship, tgt, rng, impulse, turn, log, cmd):
    out = []
    if not tgt:
        return out
    w = ship.get("weapons") or {}
    fired = ((log or {}).get("fired") or {}).get(ship["label"], {})
    try:
        lo, hi = cmd.optimal_band(ship, tgt)
    except Exception:
        lo, hi = 1, 8
    es = 0
    try:
        es = cmd.facing_shield(tgt, ship)
    except Exception:
        pass

    for fam, label in (("disruptor", "DISRUPTORS"), ("photon", "PHOTONS"),
                       ("fusion", "FUSION BEAMS"), ("hellbore", "HELLBORES"),
                       ("plasma", "PLASMA")):
        n = w.get(fam, [0, 0])[0]
        if not n:
            continue
        # E1.50 reload is PER MOUNT. Tracking it per FAMILY meant one ranging
        # disruptor silenced all six for eight impulses - the single most
        # expensive kind of bad advice, because it withholds a whole volley at
        # the moment of decision. The client's own EAF proves the model: it
        # allocates Disruptor (A)..(E) as separate lines, and the log names the
        # mount that fired ("Kharg fires Disruptor #B (FA) ...").
        now = _abs_imp(turn, impulse)
        mounts = ((log or {}).get("units") or {}).get(ship["label"], {}).get(fam, {})
        cycling, spent = mount_status(ship, fam, turn, impulse, log)
        ready = max(0, n - len(cycling) - len(spent))
        if mounts and not ready and spent and not cycling:
            out.append((f"{label}: ALL {n} ALREADY FIRED THIS TURN",
                        [f"E1.50: a weapon may not fire more than once per turn, even "
                         f"once the quarter-turn gap has passed",
                         f"spent: {', '.join('#' + w for w in sorted(spent))} - they come "
                         f"back next turn"]))
            continue
        if mounts and not ready:
            soonest = min(cycling.values())
            out.append((f"{label}: ALL {n} RELOADING - cannot fire",
                        [f"every mount fired within the last {RELOAD_IMPULSES} impulses "
                         f"(E1.50); the first comes back in {soonest}",
                         f"cycling: {', '.join(f'#{k} ({v} to go)' for k, v in sorted(cycling.items()))}"]))
            continue
        if cycling:
            # Some ready, some not - fire the ones that can, and say so. The
            # order must name the number actually firable, not the number the
            # ship owns, or the player over-commits a volley that isn't there.
            n = ready
            label = f"{label} ({ready} of {ready + len(cycling)} ready)"
        elif not mounts:
            # No per-mount detail yet this game: fall back to the family stamp
            # rather than silently claiming everything is ready.
            stamps = fired.get(fam) or []
            if stamps:
                since = now - _abs_imp(*stamps[-1])
                if since < RELOAD_IMPULSES:
                    out.append((f"{label}: RELOADING - cannot fire",
                                [f"fired T{stamps[-1][0]}.{stamps[-1][1]}; E1.50 requires "
                                 f"{RELOAD_IMPULSES} impulses, {RELOAD_IMPULSES - since} to go",
                                 "per-mount detail not seen yet this game - treating the "
                                 "whole battery as cycling, which is the cautious read"]))
                    continue
        # E4.14: "photons cannot be fired at a true range of one hex or less.
        # Exception, see overloads (E4.43)." The band test alone has no lower
        # bound, so at the exact moment of an overrun the advisor was issuing an
        # order the client will reject - and the human loses the volley.
        if fam == "photon" and rng <= 1:
            overloaded = False
            try:
                overloaded = cmd.is_overloaded(ship, "photon")
            except Exception:
                pass
            if not overloaded:
                out.append(("PHOTONS: HOLD - inside minimum range",
                            [f"E4.14: photons cannot fire at a true range of 1 or less; "
                             f"we are at {rng}",
                             "the shot would simply be refused - open the range or use "
                             "phasers for this pass"]))
                continue
            why = [f"E4.43: OVERLOADED photons are the one exception to the E4.14 "
                   f"minimum range and may fire at range {rng}"]
            try:
                fb = cmd.feedback_warning("photon", rng, True)
                if fb:
                    why.append(fb)
            except Exception:
                pass
            out.append((f"FIRE {n} {label} at {tgt['label']} - his {cmd.SHIELD.get(es, es)}", why))
            continue

        if rng <= hi:
            why = [f"range {rng} is inside the {lo}-{hi} band; aim at his "
                   f"{cmd.SHIELD.get(es, es)}"]
            if fam == "disruptor":
                why.append("E3.24: disruptors cannot hold a charge across the turn "
                           "break - an unfired disruptor at end of turn is wasted")
            out.append((f"FIRE {n} {label} at {tgt['label']} - his {cmd.SHIELD.get(es, es)}", why))
        elif fam == "disruptor":
            # "HOLD until range 8" is only half the order for a use-or-lose
            # weapon. E3.24 wipes the arming energy at the turn break, so the
            # hold is a bet that the range arrives before impulse 32 - say when
            # it will, and say when it will NOT, instead of betting silently.
            # The deadline is computed for the OVERLOAD reach because holding
            # inside the standard chart only makes sense on an overload; a
            # standard round at this range should simply be fired late in the
            # turn if nothing better shows up.
            dl = None
            try:
                dl = cmd.use_or_lose_deadline(ship, [tgt], rng, turn, impulse,
                                              overloaded=True)
            except Exception:
                pass
            why = [f"range {rng}, effective band {lo}-{hi} - firing now wastes it"]
            head = f"{label}: HOLD until range {hi}"
            if dl:
                head = f"{label}: HOLD - {dl[0].split(': ', 1)[-1]}"
                why += dl[1]
            out.append((head, why))
        else:
            out.append((f"{label}: HOLD until range {hi}",
                        [f"range {rng}, effective band {lo}-{hi} - firing now wastes it"]))
    return out


def phaser_actions(ship, tgt, rng, state, cmd, turn=1, impulse=1, log=None):
    w = ship.get("weapons") or {}
    PH_FAMS = ("phaser-1", "phaser-2", "phaser-3", "phaser-4", "phaser")
    nph = sum(w.get(f, [0, 0])[0] for f in PH_FAMS)
    if not nph:
        return []

    # E1.50 applies to phasers exactly as to heavy weapons - the rule's own
    # worked example IS a phaser - but no lockout was applied here at all, so
    # the advice happily re-ordered the same mounts every impulse. Ph-G is the
    # exception: E2.151 allows it four shots a turn.
    ready, cyc_all, spent_all = 0, {}, {}
    for f in PH_FAMS:
        cnt = w.get(f, [0, 0])[0]
        if not cnt:
            continue
        cyc, spent = mount_status(ship, f, turn, impulse, log)
        ready += max(0, cnt - len(cyc) - len(spent))
        if cyc:
            cyc_all[f] = cyc
        if spent:
            spent_all[f] = spent
    nphg = w.get("phaser-G", [0, 0])[0]
    if nphg:
        cyc_g, _spent_g = mount_status(ship, "phaser-G", turn, impulse, log,
                                       shots_per_turn=GATLING_SHOTS_PER_TURN)
        ready += max(0, nphg - len(cyc_g))
    if (cyc_all or spent_all) and not ready:
        why = ["E1.50: no weapon may fire twice within a quarter-turn, and not "
               "more than once per turn at all"]
        if spent_all:
            why.append("already fired this turn: " + ", ".join(
                f"{f} {'/'.join('#' + w2 for w2 in sorted(v))}" for f, v in spent_all.items()))
        if cyc_all:
            why.append("still cycling: " + ", ".join(
                f"{f} {'/'.join(f'#{k} ({v})' for k, v in sorted(x.items()))}"
                for f, x in cyc_all.items()))
        return [("PHASERS: none available - all mounts spent or cycling", why)]
    if cyc_all or spent_all:
        nph = ready
    seekers = []
    try:
        seekers = cmd.incoming_seekers(ship, state) if state else []
    except Exception:
        pass
    if seekers:
        return [(f"PHASERS to POINT DEFENCE - {len(seekers)} seeker(s) inbound",
                 ["FD1.51: phasers are UNPENALISED against drones while heavy "
                  "weapons take a 4-point ECM penalty (FD1.52)",
                  "E1.7: drones gain +2 ECM at 10-19 hexes and +4 at 20+ - "
                  "engage them inside 9"])]
    if tgt and rng <= 5:
        return [(f"FIRE {nph} PHASERS at {tgt['label']}",
                 [f"range {rng} - inside the ph-1 wall at 5; crossing to 6 costs "
                  f"38% of a ph-1's expected damage"])]
    if tgt and rng <= 8:
        return [("PHASERS: hold for range 5",
                 [f"range {rng}; a ph-1 is worth far more at 5 than at {rng}, and "
                  f"the capacitor holds its charge for free (H6.1)"])]
    return []


def impulse_actions(ship, enemies, tgt, rng, impulse, turn, log, state, cmd):
    """All non-movement actions for this impulse, most decisive first."""
    acts = []
    if not enemies:
        return acts
    acts += fire_actions(ship, tgt, rng, impulse, turn, log, cmd)
    acts += phaser_actions(ship, tgt, rng, state, cmd, turn=turn,
                           impulse=impulse, log=log)
    acts += drone_actions(ship, tgt, rng, cmd, turn=turn, impulse=impulse,
                          log=log, state=state, enemies=enemies)
    acts += fighter_actions(ship, impulse, rng, cmd, turn=turn, log=log, state=state)
    acts += esg_actions(ship, rng, cmd, turn=turn, state=state)
    try:
        import sfb_shuttles as SH
        acts += SH.shuttle_actions(ship, enemies, tgt, rng, impulse,
                                   turn, log, cmd)
    except Exception:
        pass
    return acts
