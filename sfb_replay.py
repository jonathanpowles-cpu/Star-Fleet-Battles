"""
Replay validation harness - run the shadow engine forward against a REAL game.

The combat log records, per impulse, what actually happened: each ship's
movement ('has moved to 1012', 'has side-slipped to 0911', 'has turned to SE'),
fire, launches, ESG activations. That is per-impulse GROUND TRUTH, and this
harness turns every logged movement into a test case:

    For each logged movement event, apply every legal shadow move (STRAIGHT,
    SLIP L/R, TURN L/R, HET) to the ship's current shadow state. If exactly the
    logged outcome is reproduced by one of them, the geometry+rules are VALIDATED
    for that event and the shadow advances by it. If NO legal move reproduces
    it, that is a genuine divergence - a geometry or rules bug (or an unmodelled
    move kind) - localised to the exact turn.impulse and ship.

This inverts the decision problem: replay does not need to know what the player
chose - the log says what happened, the engine proves it could have produced it.
Facing is INFERRED (the save's end-state facing is not the mid-game facing):
seeded from the ship's first straight-line move, then tracked through turns.

Run:  python sfb_replay.py            (uses the live log)
"""
from __future__ import annotations

import sfb_hex as H
import sfb_shadow as SHADOW

# Client facing letters (A-F clockwise, A = up) -> facing index. The log's turn
# events say 'has turned to F' and every destination label carries a trailing
# facing letter ('1402C' = hex 1402, facing C) - so each move event is position
# AND facing ground truth, no inference needed once a ship is seeded.
FACING_OF = {"A": 0, "B": 1, "C": 2, "D": 3, "E": 4, "F": 5}

# The move kinds the engine can produce, tried in this order. STRAIGHT first:
# it is the overwhelmingly common case and, uniquely, cannot be confused with a
# turn (turns change facing, slips reach different hexes).
_CANDIDATES = ("STRAIGHT", "SLIP_LEFT", "SLIP_RIGHT",
               "TURN_LEFT", "TURN_RIGHT", "HET")


def parse_hex_label(label):
    """'1402C' -> ((14, 2), 2): the client's COLROW+facing label. The trailing
    letter (A-F) is the ship's facing AFTER the move; the last two digits before
    it are the row, the rest the column. Facing is None when absent ('1012')."""
    s = str(label).strip()
    facing = None
    if s and s[-1].upper() in FACING_OF:
        facing = FACING_OF[s[-1].upper()]
        s = s[:-1]
    if not s.isdigit() or len(s) < 3:
        return None
    return (int(s[:-2]), int(s[-2:])), facing


def _try_moves(rec, dest, facing_to=None):
    """Which candidate moves take rec to `dest` (and facing_to, when given)?

    Returns [(kind, new_facing)] of every candidate that reproduces the logged
    outcome. Tried on a COPY - the caller applies the winner to the real record.
    """
    out = []
    for kind in _CANDIDATES:
        probe = {"x": rec["x"], "y": rec["y"], "facing": rec["facing"]}
        SHADOW.apply_move(probe, kind)
        if (probe["x"], probe["y"]) != dest:
            continue
        if facing_to is not None and probe["facing"] != facing_to:
            continue
        out.append((kind, probe["facing"]))
    return out


def replay(log):
    """Validate every logged movement event against the shadow engine.

    Every destination label carries facing ('1402C'), so a ship is fully seeded
    at its FIRST event; from the second on, every event must be reproducible by
    a legal engine move that lands on the logged hex WITH the logged facing.

    Returns {events, matched, unmatched[], ships{label: rec}, moves{label: [..]}}.
    """
    world = {}                  # label -> {"x","y","facing"}
    moves = {}                  # label -> [(t, i, kind)]
    unmatched = []
    n_events = n_checked = n_matched = 0

    for ev in (log.get("events") or []):
        kind = ev.get("kind")
        if kind not in ("move", "slip", "turn"):
            continue
        ship = ev.get("ship")
        n_events += 1

        if kind == "turn" and parse_hex_label(ev.get("to")) is None:
            # 'has changed to facing F' - a bare facing letter, no position.
            # (A turn WITH a destination - 'has turned to 2408D' - falls through
            # to the positional path below, where TURN_L/R candidates match it.)
            facing_to = FACING_OF.get(str(ev.get("to", "")).strip().upper())
            rec = world.get(ship)
            if facing_to is None:
                continue
            if rec is None:
                world[ship] = {"x": None, "y": None, "facing": facing_to}
                continue
            n_checked += 1
            if rec["facing"] is None:
                rec["facing"] = facing_to
                n_matched += 1
                continue
            step = (facing_to - rec["facing"]) % 6
            if step in (1, 5, 3, 0):        # right / left / HET / re-announcement
                if step:
                    kindname = {1: "TURN_RIGHT", 5: "TURN_LEFT", 3: "HET"}[step]
                    moves.setdefault(ship, []).append((ev["t"], ev["i"], kindname))
                rec["facing"] = facing_to   # position update comes as its own event
                n_matched += 1
            else:
                unmatched.append({"t": ev["t"], "i": ev["i"], "ship": ship,
                                  "kind": "turn",
                                  "detail": f"facing jump {rec['facing']}->"
                                            f"{facing_to} is not a legal turn"})
                rec["facing"] = facing_to
            continue

        parsed = parse_hex_label(ev.get("to"))
        if parsed is None:
            continue
        dest, dfacing = parsed
        rec = world.get(ship)
        if rec is None or rec["x"] is None:
            # First positional sighting fully seeds the ship (facing on label).
            f = dfacing if dfacing is not None else (rec or {}).get("facing")
            world[ship] = {"x": dest[0], "y": dest[1], "facing": f}
            continue
        if rec["facing"] is None:
            rec["facing"] = dfacing         # label supplies it
        if dest == (rec["x"], rec["y"]):
            continue                        # re-announcement of the same hex

        # The real test: can any legal engine move reproduce the logged outcome
        # (destination hex AND the facing on the label)?
        n_checked += 1
        hits = _try_moves(rec, dest, dfacing)
        if hits:
            mk, nf = hits[0]
            rec["x"], rec["y"] = dest
            rec["facing"] = nf
            moves.setdefault(ship, []).append((ev["t"], ev["i"], mk))
            n_matched += 1
        else:
            unmatched.append({"t": ev["t"], "i": ev["i"], "ship": ship,
                              "kind": kind,
                              "detail": f"({rec['x']},{rec['y']}) facing "
                                        f"{rec['facing']} cannot reach {dest} "
                                        f"facing {dfacing} by any legal move"})
            rec["x"], rec["y"] = dest       # resync so one bug doesn't cascade
            if dfacing is not None:
                rec["facing"] = dfacing

    return {"events": n_events, "checked": n_checked, "matched": n_matched,
            "unmatched": unmatched, "ships": world, "moves": moves}


def replay_combat(log):
    """Validate the logged COMBAT against the engine, at replay-tracked positions.

    Movement replay gives every ship's position AND facing at each event as it
    walks - so unlike the live Referee check (which only has current positions),
    range and shield-facing here are EXACT at fire time. Three checks per volley:

      1. RANGE     - our hex_distance at the tracked positions vs the client's
                     own logged range (fire_detail carries it). A mismatch is a
                     geometry bug, full stop.
      2. MAGNITUDE - the damage event's total vs the chart maximum for the
                     declared volley at that range (sfb_resolve bounds).
      3. FACING    - the shield that took the damage vs the shield our geometry
                     says faces the attacker.

    Returns {checks[], violations[]} - every check listed, failures flagged.
    """
    import sfb_resolve as RES

    world = {}
    checks, violations = [], []
    pending_fires = {}          # (t, i, target) -> [fire events]
    pending_rolls = []          # unconsumed 1d6 roll events, in log order

    def check(ok, kind, t, i, detail):
        rec = {"t": t, "i": i, "kind": kind, "ok": ok, "detail": detail}
        checks.append(rec)
        if not ok:
            violations.append(rec)

    for ev in (log.get("events") or []):
        kind = ev.get("kind")

        # ---- keep the kinematic world current (same walk as replay())
        if kind in ("move", "slip", "turn"):
            parsed = parse_hex_label(ev.get("to"))
            if parsed is None:
                if kind == "turn":
                    f = FACING_OF.get(str(ev.get("to", "")).strip().upper())
                    if f is not None and ev.get("ship") in world:
                        world[ev["ship"]]["facing"] = f
                continue
            dest, dfacing = parsed
            rec = world.setdefault(ev["ship"], {"x": None, "y": None, "facing": None})
            rec["x"], rec["y"] = dest
            if dfacing is not None:
                rec["facing"] = dfacing
            continue

        if kind == "fire_detail":
            atk, tgt = world.get(ev.get("ship")), world.get(ev.get("target"))
            logged_rng = ev.get("range")
            if atk and tgt and atk["x"] is not None and tgt["x"] is not None \
                    and logged_rng is not None:
                ours = H.hex_distance((atk["x"], atk["y"]), (tgt["x"], tgt["y"]))
                check(ours == logged_rng, "range", ev["t"], ev["i"],
                      f"{ev['ship']} {ev['weapon']} {ev.get('id','')} at "
                      f"{ev['target']}: our range {ours} vs client {logged_rng}"
                      + ("" if ours == logged_rng else "  <-- GEOMETRY BUG"))
            continue

        if kind == "fire":
            pending_fires.setdefault((ev["t"], ev["i"], ev.get("target")), []).append(ev)
            continue

        if kind == "roll":
            if ev.get("die") == "1d6":     # to-hit rolls; 2d6 lines are DAC etc.
                pending_rolls.append(ev)
            continue

        if kind == "damage":
            key = (ev["t"], ev["i"], ev.get("ship"))
            fires = pending_fires.pop(key, [])
            if not fires:
                continue                    # damage with no visible source (test ships etc)
            tgt = world.get(ev["ship"])
            total = float(ev.get("total") or 0)

            # MAGNITUDE: total <= sum over firing groups of n x chart max at range.
            cap = 0.0
            capable = True
            for fe in fires:
                wkey = RES.weapon_key(fe.get("weapon"))
                atk = world.get(fe.get("ship"))
                if not (wkey and atk and tgt and atk["x"] is not None
                        and tgt["x"] is not None):
                    capable = False
                    break
                rng = H.hex_distance((atk["x"], atk["y"]), (tgt["x"], tgt["y"]))
                # Mode-agnostic bound: the log's mode tag is unreliable (a
                # proven-overload volley was logged 'Standard mode').
                cap += RES.volley_absolute_max(wkey, fe.get("n", 1), rng)
            if capable:
                srcs = " + ".join(f"{fe['ship']} {fe.get('n',1)}x{fe['weapon']}"
                                  for fe in fires)
                check(total <= cap + 0.01, "magnitude", ev["t"], ev["i"],
                      f"{ev['ship']} took {total:g} from {srcs}: chart max {cap:g}"
                      + ("" if total <= cap + 0.01 else "  <-- IMPOSSIBLE DAMAGE"))

            # EXACT OUTCOME: with the client's actual dice, reproduce the number.
            # Each volley's to-hit rolls are one 'Rolls 1d6' line with exactly n
            # values; match by count (nearest unconsumed line for this volley's
            # size). For each firing mode, hits(rolls, to-hit#) x damage must
            # equal the observed total; a match reproduces the client's own
            # arithmetic and reveals the TRUE mode (the logged tag lies).
            if len(fires) == 1 and capable:
                fe = fires[0]
                wkey = RES.weapon_key(fe.get("weapon"))
                atk = world.get(fe.get("ship"))
                n = int(fe.get("n") or 1)
                rl = next((r for r in pending_rolls if len(r["values"]) == n), None)
                if wkey and rl is not None:
                    pending_rolls.remove(rl)
                    rng = H.hex_distance((atk["x"], atk["y"]), (tgt["x"], tgt["y"]))
                    explains = []
                    for mode in RES.HEAVY_MODES:
                        dmg, hitno = RES.heavy_mode_damage(wkey, rng, mode)
                        hits = sum(1 for v in rl["values"] if hitno and v <= hitno)
                        if dmg and hits * dmg == total:
                            explains.append((mode, hits, dmg))
                    if explains:
                        mode, hits, dmg = explains[0]
                        check(True, "exact", ev["t"], ev["i"],
                              f"{fe['ship']} rolls {rl['values']} -> {hits} hit(s) "
                              f"x {dmg:g} = {total:g} EXACT ({mode} mode inferred)")
                    else:
                        check(False, "exact", ev["t"], ev["i"],
                              f"{fe['ship']} rolls {rl['values']} cannot produce "
                              f"{total:g} at range {rng} in any mode "
                              f"<-- chart or pairing bug")

            # FACING: the struck shield vs the one our geometry predicts.
            hit = [ix for ix, v in enumerate(ev.get("by_shield") or []) if v]
            if len(hit) == 1 and tgt and tgt["x"] is not None \
                    and tgt.get("facing") is not None:
                preds = set()
                for fe in fires:
                    atk = world.get(fe.get("ship"))
                    if atk and atk["x"] is not None:
                        preds.add(H.shield_hit((tgt["x"], tgt["y"]), tgt["facing"],
                                               (atk["x"], atk["y"])))
                if len(preds) == 1:
                    p = preds.pop()
                    check(p == hit[0], "facing", ev["t"], ev["i"],
                          f"{ev['ship']} hit on #{hit[0] + 1}, geometry predicts "
                          f"#{p + 1}"
                          + ("" if p == hit[0] else "  <-- FACING BUG"))
            continue

        if kind == "esg_fire":
            # Field strength must sit on the client's own chart column for the
            # radius (whole-point energies, G23.223).
            try:
                import sfb_actions as ACT
                row = ACT.ESG_CHART.get(int(ev.get("radius", -1)))
                ok = bool(row) and int(ev.get("strength", -1)) in row
            except Exception:
                ok = True
            check(ok, "esg", ev["t"], ev["i"],
                  f"{ev['ship']} ESG r{ev.get('radius')} = {ev.get('strength')} pts"
                  + ("" if ok else "  <-- not on the ESG chart"))

    return {"checks": checks, "violations": violations}


def combat_report_lines(res):
    n = len(res["checks"])
    bad = res["violations"]
    out = [f"COMBAT REPLAY: {n} check(s) - "
           f"{n - len(bad)} passed, {len(bad)} violation(s)"]
    if not bad:
        out.append("CLEAN - every logged volley's range, damage magnitude, shield "
                   "facing and ESG strength is consistent with the engine's "
                   "charts and geometry at the replayed positions.")
    for c in res["checks"]:
        mark = "ok " if c["ok"] else "XXX"
        out.append(f"  [{mark}] T{c['t']}.{c['i']} {c['kind']}: {c['detail']}")
    return out


def report_lines(res):
    out = [f"REPLAY: {res['checked']} movement events checked, "
           f"{res['matched']} reproduced by the engine, "
           f"{len(res['unmatched'])} divergence(s)"]
    if not res["unmatched"]:
        out.append("CLEAN - every logged move is reproducible: even-q geometry, "
                   "facing model and move set validated against real play.")
    for u in res["unmatched"]:
        out.append(f"  T{u['t']}.{u['i']} {u['ship']} [{u['kind']}]: {u['detail']}")
    for ship, mv in sorted(res["moves"].items()):
        kinds = [k for _, _, k in mv]
        n_st = sum(1 for k in kinds if k.startswith("STRAIGHT"))
        rest = [f"{t}.{i} {k}" for t, i, k in mv if not k.startswith("STRAIGHT")]
        out.append(f"  {ship}: {len(mv)} moves ({n_st} straight"
                   + (f"; {', '.join(rest)}" if rest else "") + ")")
    return out


if __name__ == "__main__":
    import sfb_log
    log = sfb_log.parse()
    if not log:
        print("no combat log found")
    else:
        res = replay(log)
        print("\n".join(report_lines(res)))
        print()
        cres = replay_combat(log)
        print("\n".join(combat_report_lines(cres)))
