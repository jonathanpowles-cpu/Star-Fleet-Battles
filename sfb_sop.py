"""
Shadow-state layer - Phase 5 FIRST SLICE: the Sequence-of-Play spine.

This is the executable-turn spine, deliberately minimal. It:

  1. Loads the client's OWN Sequence of Play from client_data/imp.act (the full
     rule-6 procedure, 121 leaf steps across 5 segments) and marks which steps
     are player DECISIONS from decision.act.
  2. Walks those steps IN ORDER for one impulse.
  3. Dispatches each step to a handler keyed by its rule id. Only 6A2.04
     ("move the pieces", C1.31) has a handler in this slice - it calls the
     Phase-1 kinematic mutator. Every other step is LOGGED AS SKIPPED.
  4. The skip log IS the coverage report: nothing is silently dropped, and what
     is not yet handled is visible and countable.

The point of the slice is to prove the spine - table-walk + handler-dispatch +
honest skip-log - reusing Phase 1 wholesale, before any new mutator is written.
Adding a segment later is adding a handler to a spine that already turns.
"""
from __future__ import annotations

import os
import re
import sfb_shadow as SHADOW
import sfb_resolve as RES

_DATA = os.path.join(os.path.dirname(os.path.abspath(__file__)), "client_data")

# variant -> (full SOP file, decision-subset file)
_VARIANTS = {
    "standard": ("imp.act", "decision.act"),
    "tournament": ("tourn_imp.act", "tourn_decision.act"),
}

_SOP_CACHE = {}


def _read_act(path):
    """Rows of an .act file as (id, parent, text). Text may contain commas, so
    only the first two are separators."""
    rows = []
    try:
        with open(path, encoding="utf-8", errors="replace") as f:
            for ln in f:
                parts = ln.rstrip("\n").split(",", 2)
                if len(parts) < 3:
                    continue
                rows.append((parts[0].strip(), parts[1].strip(), parts[2]))
    except OSError:
        pass
    return rows


def load_sop(variant="standard"):
    """Ordered list of leaf SOP steps for a variant.

    A step is a LEAF (an action, not a container) when its id is present and is
    not the parent of any other row - robust to the dotless ids the dogfight
    segment uses (6C05) and to comma-bearing step text. Each step carries its
    segment (6A..6E) and an is_decision flag from the decision-subset file.
    """
    if variant in _SOP_CACHE:
        return _SOP_CACHE[variant]
    full, dec = _VARIANTS.get(variant, _VARIANTS["standard"])
    rows = _read_act(os.path.join(_DATA, full))
    parents = {p for _, p, _ in rows if p}
    dec_ids = {r[0] for r in _read_act(os.path.join(_DATA, dec)) if r[0]}

    steps = []
    for sid, parent, text in rows:
        if not sid or sid in parents:
            continue                       # container / header, not an action
        m = re.match(r"6([A-Z])", sid)
        segment = f"6{m.group(1)}" if m else "?"
        steps.append({
            "id": sid, "parent": parent, "text": text.strip(),
            "segment": segment, "is_decision": sid in dec_ids,
        })
    _SOP_CACHE[variant] = steps
    return steps


# ------------------------------------------------------------------- handlers
# id -> callable(world, impulse, step, ctx) -> short result string (or None).
# A handler MUTATES the shadow world and reports what it did. Registering a new
# id here is how a segment gets wired in; everything unregistered is skipped.

def handle_move(world, impulse, step, ctx):
    """6A2.04 - move the pieces this impulse (C1.31), using Phase 1.

    Applies each ship's advised move (ctx['moves'] = {label: MOVE}) once, gated
    by the C1.4 impulse chart inside advance_impulse. Reports which ships the
    chart actually moved this impulse.
    """
    moves = ctx.get("moves") or {}
    moved = [lab for lab, r in world.items()
             if SHADOW.moves_this_impulse(r["speed"], impulse)]
    SHADOW.advance_impulse(world, impulse, moves)
    # Seekers are playing pieces too - they move in this same stage (C1.31), so
    # the ESG (6A3.01) and impact (6A3.03) steps see their post-move positions.
    sk_moved = SHADOW.advance_seekers(world, ctx.get("seekers") or [], impulse)
    parts = []
    if moved:
        parts.append(f"ships {len(moved)}: "
                     + ", ".join(f"{lab} {moves.get(lab, 'STRAIGHT')}" for lab in moved))
    if sk_moved:
        parts.append(f"seekers {len(sk_moved)} homed")
    return "; ".join(parts) if parts else "nothing moves on this impulse (C1.4)"


def handle_direct_fire(world, impulse, step, ctx):
    """6D2.04 - Direct-Fire Step (E1.11): resolve declared direct-fire volleys.

    Fire DECISIONS are provided (ctx['fire'] = [{attacker, target, weapon,
    count}]) - the SOP's fire-declaration steps (6D1.02/03) are the decision
    points; this step just RESOLVES them, which is the engine's job. Damage is
    rolled with a seeded Roller (reproducible: ctx['roller'] or ctx['seed']) so a
    replay is diagnosable, then landed on the target's facing shield via the
    Phase-5 mutator. Reports each volley.
    """
    fire = ctx.get("fire") or []
    if not fire:
        return "no direct-fire declared this impulse"
    roller = ctx.get("roller") or RES.Roller(ctx.get("seed", 0))
    ctx["roller"] = roller                          # share one roller across steps
    lines = []
    seekers = ctx.get("seekers") or []
    for v in fire:
        atk, tgt = v.get("attacker"), v.get("target")
        key = RES.weapon_key(v.get("weapon"))
        n = int(v.get("count") or 0)
        ar = world.get(atk)
        # A volley may target a SEEKER (anti-drone defensive fire) or a ship.
        sk = SHADOW.find_seeker(seekers, tgt)
        tr = world.get(tgt)
        if not ar or (tr is None and sk is None) or not key or n <= 0:
            lines.append(f"{atk}->{tgt} {v.get('weapon')}: unresolvable (skipped)")
            continue
        # Cap the volley by the attacker's SURVIVING weapon boxes. rec['boxes']
        # reflects damage from PRIOR impulses only (this impulse's damage is not
        # resolved until 6D4.02), so a weapon killed earlier this impulse still
        # fires - exactly the 6D4.02 rule.
        avail = SHADOW.surviving_weapon_boxes(ar, v.get("weapon"))
        cap_note = ""
        if avail is not None and avail < n:
            if avail <= 0:
                lines.append(f"{atk}->{tgt} {v.get('weapon')}: NO surviving mounts "
                             f"(all destroyed) - cannot fire")
                continue
            cap_note = f" [capped {n}->{avail}: mounts destroyed]"
            n = avail
        # Phaser fire also draws the capacitor (H6.21); a drained ship fires fewer.
        fired = SHADOW.drain_capacitor(ar, v.get("weapon"), n)
        if fired < n:
            if fired <= 0:
                lines.append(f"{atk}->{tgt} {v.get('weapon')}: capacitor empty - "
                             f"cannot fire")
                continue
            cap_note += f" [capacitor: {fired}/{n} fire]"
            n = fired

        # --- ANTI-DRONE: phaser fire at a seeker (FD1.51: unpenalised vs drones)
        if sk is not None:
            rng = SHADOW._hex_range((ar["x"], ar["y"]), (sk["x"], sk["y"]))
            dmg = RES.resolve_volley(key, n, rng, roller)
            status, taken, kill = SHADOW.damage_seeker(seekers, tgt, dmg)
            if status == "killed":
                lines.append(f"{atk} kills {tgt} with {n}x{v.get('weapon')} @{rng}: "
                             f"{dmg:g} dmg (needed {kill}){cap_note}")
            elif status == "damaged":
                lines.append(f"{atk} hits {tgt}: {dmg:g}/{kill} to kill "
                             f"({taken} accumulated){cap_note}")
            elif status == "immune":
                lines.append(f"{atk}->{tgt}: plasma cannot be phaser-killed, only "
                             f"reduced (FP1.611){cap_note}")
            else:
                lines.append(f"{atk}->{tgt}: seeker gone{cap_note}")
            continue

        rng = SHADOW._hex_range((ar["x"], ar["y"]), (tr["x"], tr["y"]))
        dmg = RES.resolve_volley(key, n, rng, roller)
        res = SHADOW.apply_damage(world, tgt, (ar["x"], ar["y"]), dmg)
        if res is None:
            lines.append(f"{atk}->{tgt} {n}x{v.get('weapon')} @{rng}: "
                         f"{dmg:g} dmg (target has no shield model){cap_note}")
        else:
            leak = (f", {res['leak']:g} recorded as internals (-> 6D4)"
                    if res["leak"] > 0 else "")
            lines.append(f"{atk}->{tgt} {n}x{v.get('weapon')} @{rng}: {dmg:g} dmg "
                         f"on #{res['shield']} ({res['absorbed']:g} absorbed{leak})"
                         f"{cap_note}")
    return "; ".join(lines)


def handle_damage_resolution(world, impulse, step, ctx):
    """6D4.02 - allocate each ship's pending internal damage to real boxes via
    the DAC (D4.0).

    The internals were RECORDED in 6D2; this resolves them, destroying boxes.
    Uses the seeded Roller so allocation is reproducible with the volley rolls.
    """
    roller = ctx.get("roller") or RES.Roller(ctx.get("seed", 0))
    ctx["roller"] = roller                          # share one roller across steps
    lines = []
    for lab, rec in world.items():
        if not rec.get("pending_internals"):
            continue
        pts = rec["pending_internals"]
        hits = SHADOW.allocate_internals(world, lab, roller)
        if not hits:
            continue
        destroyed = [h["box"] for h in hits if h["result"] == "destroyed"]
        other = [h["box"] for h in hits if h["result"] != "destroyed"]
        bits = []
        if destroyed:
            bits.append("destroyed " + ", ".join(destroyed))
        if other:
            bits.append(f"{len(other)} on untracked/excess ({', '.join(other)})")
        lines.append(f"{lab}: {pts:g} internals -> " + "; ".join(bits))
    return "; ".join(lines) if lines else "no pending internals to resolve"


def handle_drone_launch(world, impulse, step, ctx):
    """6B06.05 - Launch drones (FD1.2). Launch DECISIONS are provided:
    ctx['launch'] = [{ship, rack, target}] where rack is a rack designator (or
    its index). Consumed per impulse. A launched drone starts homing next
    impulse (6B is after 6A movement), so it moves from 6A2 of impulse+1.
    """
    reqs = ctx.get("launch") or []
    if not reqs:
        return "no drone launches declared"
    seekers = ctx.setdefault("seekers", [])
    lines = []
    for req in reqs:
        ship, target, rackref = req.get("ship"), req.get("target"), req.get("rack")
        rec = world.get(ship)
        racks = (rec or {}).get("racks") or []
        if not racks:
            lines.append(f"{ship}: no drone racks")
            continue
        rack = None
        for i, rk in enumerate(racks):
            if rk.get("designator") == rackref or i == rackref:
                rack = rk
                break
        if rack is None:
            rack = next((rk for rk in racks if rk["ammo"] > 0), None)
        if rack is None:
            lines.append(f"{ship}: rack {rackref} not found / all empty")
            continue
        sk, note = SHADOW.launch_drone(world, seekers, ship, rack, target, impulse)
        lines.append(f"{ship}: {note}")
    ctx["launch"] = []                                  # one-shot per impulse
    return "; ".join(lines)


def handle_esg(world, impulse, step, ctx):
    """6A3.01 - Resolve actions of ESGs (G23.5): release any ESG requested this
    impulse, then engage seekers within each active sphere.

    Release is a DECISION: ctx['esg_release'] = {ship_label: radius} (or
    {label: {'radius': R, 'charges': [...]}}). It is consumed (one-shot). Runs
    before 6A3.03, so a drone killed by the field never reaches impact.
    """
    for lab, req in (ctx.get("esg_release") or {}).items():
        rec = world.get(lab)
        if not rec:
            continue
        radius = req.get("radius") if isinstance(req, dict) else req
        charges = req.get("charges") if isinstance(req, dict) else None
        SHADOW.release_esg(rec, radius, charges)
    ctx["esg_release"] = {}                          # one-shot: consume releases
    seekers = ctx.get("seekers") or []
    lines = SHADOW.resolve_esg_vs_seekers(world, seekers)
    return "; ".join(lines) if lines else "no ESG engagement this impulse"


def handle_seeker_impact(world, impulse, step, ctx):
    """6A3.03 - resolve damage from seeking weapons (F2.3).

    Seekers live in ctx['seekers'] (SHADOW.build_seekers at turn start). This
    advances them toward their targets and detonates any that arrive, using the
    shared seeded roller so the run stays reproducible.
    """
    seekers = ctx.get("seekers")
    if not seekers:
        return "no seekers on the board"
    roller = ctx.get("roller") or RES.Roller(ctx.get("seed", 0))
    ctx["roller"] = roller
    lines = SHADOW.resolve_seeker_impacts(world, seekers, impulse, roller)
    return "; ".join(lines) if lines else "no seeker impacts this impulse"


def handle_postcombat(world, impulse, step, ctx):
    """6E3 - postcombat. On impulses 1-31 this is a no-op placeholder; on impulse
    32 it is the TURN BOUNDARY, where reload/repair happens (the impulse procedure
    itself has no reload step - reload/repair is a turn-level activity).
    """
    if impulse < 32:
        return "postcombat: nothing to reload mid-turn"
    lines = SHADOW.end_of_turn(world)
    return ("end of turn: " + "; ".join(lines)) if lines else \
        "end of turn: nothing to reload/repair"


def handle_energy_balance(world, impulse, step, ctx):
    """6A3.11 - Energy Balance Due to Damage (D22.0): re-derive each ship's
    capability from its surviving boxes (speed cap + crippling).

    Runs in the movement segment, so a ship's direct-fire damage from the PRIOR
    impulse (resolved in that impulse's 6D4) takes effect here on the next - which
    is exactly when D22 bites in the sequence of play.
    """
    lines = []
    for lab, rec in world.items():
        ch = SHADOW.apply_energy_balance(rec)
        if not ch:
            continue
        if "speed" in ch:
            s = ch["speed"]
            lines.append(f"{lab}: D22 speed {s['from']}->{s['to']} "
                         f"(warp+impulse plant {s['plant']})")
        if "crippled" in ch:
            lines.append(f"{lab}: CRIPPLED - {ch['crippled']}")
    return "; ".join(lines) if lines else "all ships within energy balance"


HANDLERS = {
    "6A2.04": handle_move,
    "6B06.05": handle_drone_launch,
    "6A3.01": handle_esg,
    "6A3.03": handle_seeker_impact,
    "6A3.11": handle_energy_balance,
    "6D2.04": handle_direct_fire,
    "6D4.02": handle_damage_resolution,
    "6E3": handle_postcombat,
}


# --------------------------------------------------------------------- runner
def run_impulse(world, impulse, ctx=None, variant="standard"):
    """Walk the whole SOP for one impulse, dispatching handled steps.

    Returns a report:
        {impulse, applied[{id,segment,result}], skipped[{id,segment,is_decision}],
         handled_count, skipped_count, decision_skipped_count}
    `applied` are steps a handler ran; `skipped` are everything else, tagged so
    the decision steps we are not yet answering are countable.
    """
    ctx = ctx or {}
    applied, skipped = [], []
    for step in load_sop(variant):
        h = HANDLERS.get(step["id"])
        if h is not None:
            try:
                res = h(world, impulse, step, ctx)
            except Exception as e:
                res = f"handler error: {e}"
            applied.append({"id": step["id"], "segment": step["segment"],
                            "result": res})
        else:
            skipped.append({"id": step["id"], "segment": step["segment"],
                            "is_decision": step["is_decision"]})
    return {
        "impulse": impulse,
        "applied": applied,
        "skipped": skipped,
        "handled_count": len(applied),
        "skipped_count": len(skipped),
        "decision_skipped_count": sum(1 for s in skipped if s["is_decision"]),
    }


def coverage(variant="standard"):
    """(handled_ids, total_leaf_steps, decision_steps) - the honest coverage
    snapshot for a variant, so 'how much of the turn can we run' is a number."""
    steps = load_sop(variant)
    handled = [s["id"] for s in steps if s["id"] in HANDLERS]
    decisions = sum(1 for s in steps if s["is_decision"])
    return handled, len(steps), decisions


def run_report_lines(report, variant="standard"):
    """Human summary of a run_impulse report for the Referee/console."""
    handled, total, _ = coverage(variant)
    out = [f"SOP impulse {report['impulse']}: "
           f"{report['handled_count']} handled, "
           f"{report['skipped_count']} skipped "
           f"({report['decision_skipped_count']} of them decisions) "
           f"| coverage {len(handled)}/{total} SOP steps"]
    for a in report["applied"]:
        out.append(f"  [{a['id']}] {a['result']}")
    # Skips are summarised per segment, not listed line by line (there are ~120).
    per_seg = {}
    for s in report["skipped"]:
        per_seg.setdefault(s["segment"], 0)
        per_seg[s["segment"]] += 1
    seg = "  ".join(f"{k}:{v}" for k, v in sorted(per_seg.items()))
    out.append(f"  skipped by segment: {seg}")
    return out


if __name__ == "__main__":
    # Self-test: one-impulse executable turn on rails, reconciled against a
    # hand-made "client" outcome. Two ships, one told to TURN_RIGHT.
    start = {"ships": [
        {"label": "Bushido", "x": 10, "y": 12, "facing": 0, "speed": 8},
        {"label": "Sorcerer", "x": 20, "y": 20, "facing": 3, "speed": 8},
    ]}
    world = SHADOW.build(start)
    ctx = {"moves": {"Bushido": "STRAIGHT", "Sorcerer": "TURN_RIGHT"}}
    # impulse 8: speed-8 ships move (chart 4,8,12,...)
    rep = run_impulse(world, 8, ctx)
    print("\n".join(run_report_lines(rep)))
    print("shadow now:", {l: (r["x"], r["y"], r["facing"]) for l, r in world.items()})
    handled, total, dec = coverage()
    print(f"\ncoverage: {len(handled)}/{total} leaf steps handled, "
          f"{dec} decision steps in the SOP")
