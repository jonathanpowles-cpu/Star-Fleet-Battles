"""
Combat-log reader for the SFU Online tactical game.

The client writes a plain-text play-by-play to app/log/SFB_Game1.log (live), recording
Energy Allocation, every impulse, movement, turns, weapons fire (weapon/arc/mode/range),
damage allocation by shield, and remaining shields. This module parses it so the AI can
SEE what actually happened - report it, and reason about weapon-cycle timing (e.g. the
8-impulse reload delay E1.x, and disruptors-can't-hold E3.24).

Works offline or online: it reads the local log file, not the network.
"""
from __future__ import annotations
import os, re, glob

CLIENT = r"C:/Users/jonat/AppData/Local/SFU Online Client"
LOGDIR = CLIENT + r"/app/log"

RE_IMP   = re.compile(r"^Impulse (\d+)\.(\d+):")
RE_EA    = re.compile(r"^(\w[\w '-]*?) has (started|finished) Energy Allocation")
RE_MOVE  = re.compile(r"^(\w[\w '-]*?) has moved to (\S+)")
RE_TURN  = re.compile(r"^(\w[\w '-]*?) has turned to (\S+)")
RE_FACE  = re.compile(r"^(\w[\w '-]*?) has changed to facing (\w) after (\d+) move")
RE_FIRES = re.compile(r"^(\w[\w '-]*?) fires (\d+) (\w[\w-]*?)s? at (\w[\w '-]*)$")
# Per-weapon fire lines. The client names the individual mount and its arc:
#   "Kharg fires Disruptor #B (FA) at Enterprise using Overloaded mode (Range: 6)"
#   "Kharg fires Phaser-1 #2 (FX) at Enterprise (Range: 6)"
#   "Kharg fires Phaser-1 #4 (LF+RR+L) at Enterprise (Range: 6)"
# Two defects in the old pattern: "using <mode> mode" was REQUIRED, so no phaser
# line ever matched, and the arc group was (\w+), which cannot match a multi-arc
# mount like "LF+RR+L". Both are permissive now.
RE_FIRE1 = re.compile(
    r"^(\w[\w '-]*?) fires ([\w-]+?) #(\w+) \(([\w+]+)\) at (\w[\w '-]*?)"
    r"(?: using (\w+) mode)? \(Range: (\d+)\)")
RE_DMGH  = re.compile(r"^Allocation of damage for: (\w[\w '-]*)")
RE_DMG   = re.compile(r"^Damage: ([\d/]+) \(Total: (\d+)\)")
RE_SHLD  = re.compile(r"^Remaining Shield: ([\d/]+)")
RE_SLIP  = re.compile(r"^(\w[\w '-]*?) has side-slipped to (\S+)")
# Dice: 'Skylark Rolls 1d6: 6, 1, 1' - the ACTUAL rolls the client made. One
# line per volley (a 3-shot volley logs 3 values), enabling exact-outcome
# replay: feed these into the charts and reproduce the damage numbers.
RE_ROLL  = re.compile(r"^(\w[\w '-]*?) Rolls (\d+)d(\d+): ([\d, ]+)$")
# ESG activation (release). G23.222: activation discharges ALL the energy in the
# generator, so a fired sphere is spent - its charge is gone until re-allocated.
#   "CW Marauder activates ESG radius 0 - 20 pts"
#   "FF Feral activates ESG at radius 1 - 15 pts"
RE_ESG_FIRE = re.compile(
    r"^(\w[\w '-]*?) activates ESG (?:at )?radius (\d+) - (\d+) pts", re.I)
RE_LAUNCH= re.compile(r"^(\w[\w '-]*?) launch\w* .*?(drone|shuttle|fighter|plasma)", re.I)
# The client does NOT write "<ship> launches a fighter". What it actually writes,
# for every seeking unit and every shuttle, is an ADDED line with the launcher
# named at the end:
#   "S01.1.4 (Type:User-Defined Shuttle) has been added at 2225, ... was launched by KHS CV Sabre"
#   "D001(1).2.11 (Type:Klingon Drone) has been added at 1120, ... was launched by Kharg"
# RE_LAUNCH could never match that shape, so launch events were never recorded,
# launched_so_far stayed 0, and the carrier's "fighters still aboard" count never
# moved. Match the real line, and classify by the Type field so a drone launch is
# not counted against the shuttle bay.
RE_ADDED_LAUNCH = re.compile(
    r"^(?P<unit>.+?) \(Type:(?P<type>[^)]*)\) has been added at .*? was launched by (?P<ship>.+?)\s*$",
    re.I)
RE_DEST  = re.compile(r"(\w[\w '-]*?) (has been destroyed|is destroyed|destroyed)")
# "KHS Sabre (Type:User-Defined Ship) has been added at 2326, direction A, speed 8"
RE_ADDED = re.compile(r"^(.+?) \(Type:.*?\) has been added at ")


def find_log(game_hint="SFB_Game1"):
    """Newest tactical combat log (match the open game; skip campaign/FNE/bullpen)."""
    cands = []
    for p in glob.glob(os.path.join(LOGDIR, "*.log")):
        b = os.path.basename(p)
        if any(x in b for x in ("Campaign", "FNE", "Bullpen", "Cadet")):
            continue
        try:
            cands.append((os.path.getmtime(p), p))
        except OSError:
            pass
    if not cands:
        return None
    # prefer one matching the hint, else newest
    for _, p in sorted(cands, reverse=True):
        if game_hint in os.path.basename(p):
            return p
    return max(cands)[1]


def _maneuver(man, ship, kind, turn, imp):
    """Track hexes since last turn / last sideslip, per SFB movement rules:
       - move: counts toward BOTH turn mode and slip mode
       - slip: counts as forward movement for Turn Mode (C4.32), but resets slip mode;
               the hex entered during the slip does not count (C4.31)
       - turn: resets Turn Mode, and ALSO resets sideslip mode to zero (C4.33)"""
    d = man.setdefault(ship, {"since_turn": 0, "since_slip": 0,
                              "last_turn": None, "last_slip": None})
    if kind == "move":
        d["since_turn"] += 1
        d["since_slip"] += 1
    elif kind == "slip":
        d["since_turn"] += 1          # C4.32: counts as forward movement for turn mode
        d["since_slip"] = 0           # C4.31/C4.1: restart the slip count
        d["last_slip"] = (turn, imp)
    elif kind == "turn":
        d["since_turn"] = 0
        d["since_slip"] = 0           # C4.33: a turn resets sideslip mode
        d["last_turn"] = (turn, imp)


def parse(path=None, tail_lines=4000):
    """Parse the combat log -> {turn, impulse, events[], fired{}, shields{}}.
    events are dicts tagged with the impulse they occurred on."""
    path = path or find_log()
    if not path or not os.path.exists(path):
        return None
    with open(path, "r", encoding="utf-8", errors="replace") as f:
        lines = f.readlines()[-tail_lines:]

    turn, imp = 1, 0
    events = []
    fired = {}
    units = {}    # per-MOUNT last-fired stamps: ship -> family -> id -> (turn, imp)
    arcs = {}     # ship -> 'family#id' -> firing arc, straight from the log
    shields = {}    # ship -> [6] most recent remaining shields
    pending_dmg = None
    man = {}       # ship -> maneuver counters (turn/slip modes)

    for ln in lines:
        s = ln.strip()
        m = RE_IMP.match(s)
        if m:
            new_turn, new_imp = int(m.group(1)), int(m.group(2))
            # A turn number going BACKWARDS means a new game began in the same log
            # file (the client appends across games). Everything before it belongs
            # to a dead battle - discard it, or the console reports the previous
            # game's damage as if it were current.
            if new_turn < turn:
                events, fired, shields, man, pending_dmg = [], {}, {}, {}, None
                units, arcs = {}, {}
            turn, imp = new_turn, new_imp
            continue
        # Ships being added after combat has been logged = a new game being set up
        # following a finished one. Same reasoning as the turn-decrease reset, but
        # fires during setup, BEFORE the first impulse of the new game.
        m = RE_ADDED_LAUNCH.match(s)
        if m:
            ty = m.group("type").lower()
            what = ("drone" if "drone" in ty else
                    "plasma" if "plasma" in ty else
                    "shuttle" if "shuttle" in ty else "fighter")
            events.append({"t": turn, "i": imp, "kind": "launch",
                           "ship": m.group("ship").strip(), "what": what,
                           "unit": m.group("unit").strip()})
            continue
        m = RE_ADDED.match(s)
        if m:
            # Do NOT reset history here. This used to wipe everything when a
            # piece was added after combat, on the theory that a new game was
            # being set up - but creating a TEST ship mid-battle (a Lion to
            # measure a shield cost) hit the same trigger and erased the live
            # game's launch and fire history; the drone racks then looked
            # unfired and were re-ordered to launch. The turn-going-backwards
            # check above catches a genuinely new game once its impulses start,
            # and restrict_to_ships() drops any events for ships that are not
            # on the current board.
            events.append({"t": turn, "i": imp, "kind": "added", "ship": m.group(1)})
            continue
        m = RE_EA.match(s)
        if m:
            events.append({"t": turn, "i": imp, "kind": "ea", "ship": m.group(1), "what": m.group(2)})
            continue
        m = RE_FIRES.match(s)
        if m:
            ship, n, weap, tgt = m.group(1), int(m.group(2)), m.group(3), m.group(4)
            events.append({"t": turn, "i": imp, "kind": "fire", "ship": ship, "n": n,
                           "weapon": weap, "target": tgt})
            # record (turn, impulse) so reload timing can cross the turn boundary
            fired.setdefault(ship, {}).setdefault(weap.lower(), []).append((turn, imp))
            continue
        m = RE_FIRE1.match(s)
        if m:
            ship, weap, wid, arc = m.group(1), m.group(2), m.group(3), m.group(4)
            events.append({"t": turn, "i": imp, "kind": "fire_detail", "ship": ship,
                           "weapon": weap, "id": wid, "arc": arc,
                           "target": m.group(5), "mode": m.group(6) or "Standard",
                           "range": int(m.group(7))})
            # E1.50 reload is PER MOUNT, not per weapon family. This detail was
            # parsed and then discarded, so `fired` only ever held family-level
            # stamps - and one ranging disruptor muted all six. The client's own
            # EAF proves the model: it allocates Disruptor (A)..(E) separately.
            units.setdefault(ship, {}).setdefault(weap.lower(), {})[wid] = (turn, imp)
            arcs.setdefault(ship, {})[f"{weap.lower()}#{wid}"] = arc
            continue
        m = RE_DMGH.match(s)
        if m:
            pending_dmg = m.group(1)
            continue
        m = RE_DMG.match(s)
        if m and pending_dmg:
            per = [int(x) for x in m.group(1).split("/")]
            events.append({"t": turn, "i": imp, "kind": "damage", "ship": pending_dmg,
                           "by_shield": per, "total": int(m.group(2))})
            continue
        m = RE_SHLD.match(s)
        if m and pending_dmg:
            shields[pending_dmg] = [int(x) for x in m.group(1).split("/")]
            pending_dmg = None
            continue
        m = RE_MOVE.match(s)
        if m:
            events.append({"t": turn, "i": imp, "kind": "move", "ship": m.group(1), "to": m.group(2)})
            _maneuver(man, m.group(1), "move", turn, imp)
            continue
        m = RE_SLIP.match(s)
        if m:
            events.append({"t": turn, "i": imp, "kind": "slip", "ship": m.group(1), "to": m.group(2)})
            _maneuver(man, m.group(1), "slip", turn, imp)
            continue
        m = RE_ROLL.match(s)
        if m:
            events.append({"t": turn, "i": imp, "kind": "roll",
                           "roller": m.group(1), "die": f"{m.group(2)}d{m.group(3)}",
                           "values": [int(v) for v in m.group(4).replace(" ", "").split(",") if v]})
            continue
        m = RE_ESG_FIRE.match(s)
        if m:
            events.append({"t": turn, "i": imp, "kind": "esg_fire",
                           "ship": m.group(1).strip(), "radius": int(m.group(2)),
                           "strength": int(m.group(3))})
            continue
        m = RE_TURN.match(s)
        if m:
            events.append({"t": turn, "i": imp, "kind": "turn", "ship": m.group(1), "to": m.group(2)})
            _maneuver(man, m.group(1), "turn", turn, imp)
            continue
        m = RE_FACE.match(s)
        if m:
            events.append({"t": turn, "i": imp, "kind": "turn", "ship": m.group(1),
                           "to": m.group(2), "after": int(m.group(3))})
            _maneuver(man, m.group(1), "turn", turn, imp)
            continue
        m = RE_LAUNCH.match(s)
        if m:
            events.append({"t": turn, "i": imp, "kind": "launch", "ship": m.group(1), "what": m.group(2)})
            continue
        m = RE_DEST.search(s)
        if m:
            events.append({"t": turn, "i": imp, "kind": "destroyed", "ship": m.group(1)})

    return {"turn": turn, "impulse": imp, "events": events, "fired": fired,
            "units": units, "arcs": arcs,
            "shields": shields, "maneuver": man}


def canonical_label(name, labels):
    """Map a log ship name to the board label it refers to, or None.

    The client logs ships in ABBREVIATED form ('CW Marauder') while the save
    uses the full label ('CW 705 Marauder' - hull number inserted). Exact
    matching drops every logged event for such ships, silently erasing their
    fire/damage/launch history. Match by word-subset instead: a log name refers
    to a board ship when all of its words appear in that ship's label. Exact
    matches (equal word sets) are preferred so an abbreviation cannot be stolen
    by a longer label that merely contains it.
    """
    if not name:
        return None
    nw = set(str(name).lower().split())
    if not nw:
        return None
    best = None
    for lab in labels:
        lw = set(lab.lower().split())
        if nw == lw:
            return lab
        if nw < lw and (best is None or len(lw) < len(set(best.lower().split()))):
            best = lab
    return best


def restrict_to_ships(parsed, labels):
    """Drop anything referring to a ship that is not on the board right now,
    and rewrite surviving ship references to the canonical board label.

    Belt-and-braces against cross-game contamination: even if a boundary marker
    is missed, a dead game's ships cannot leak into the current report. The
    canonicalisation also repairs the abbreviated-vs-full name mismatch across
    ALL event kinds (fire, damage, launch, maneuver), not just ESG - previously
    those events were dropped outright by exact matching.
    """
    if not parsed or not labels:
        return parsed
    labels = list(labels)
    parsed = dict(parsed)

    def canon(name):
        return canonical_label(name, labels) if name else None

    kept = []
    for e in parsed["events"]:
        if e.get("ship") is None:
            kept.append(e)
            continue
        lab = canon(e["ship"])
        if lab is not None:
            e = dict(e)
            e["ship"] = lab
            kept.append(e)
    parsed["events"] = kept

    def remap(d):
        out = {}
        for k, v in d.items():
            lab = canon(k)
            if lab is not None:
                out[lab] = v
        return out
    parsed["fired"] = remap(parsed["fired"])
    parsed["shields"] = remap(parsed["shields"])
    parsed["maneuver"] = remap(parsed["maneuver"])
    return parsed


def recent_combat(parsed, this_turn_only=True, kinds=("fire", "damage", "destroyed", "launch")):
    """Human-readable recent combat lines."""
    if not parsed:
        return []
    out = []
    for e in parsed["events"]:
        if this_turn_only and e["t"] != parsed["turn"]:
            continue
        if e["kind"] == "fire":
            out.append(f'T{e["t"]}.{e["i"]}: {e["ship"]} fired {e["n"]} {e["weapon"]} at {e["target"]}')
        elif e["kind"] == "damage":
            faces = ",".join(f"#{i+1}:{v}" for i, v in enumerate(e["by_shield"]) if v)
            out.append(f'T{e["t"]}.{e["i"]}: {e["ship"]} took {e["total"]} damage ({faces or "internals"})')
        elif e["kind"] == "destroyed":
            out.append(f'T{e["t"]}.{e["i"]}: {e["ship"]} DESTROYED')
        elif e["kind"] == "launch":
            out.append(f'T{e["t"]}.{e["i"]}: {e["ship"]} launched {e["what"]}')
    return out


if __name__ == "__main__":
    p = parse()
    if p:
        print(f"log: turn {p['turn']} impulse {p['impulse']}")
        print("fired:", p["fired"])
        print("shields:", p["shields"])
        print("\nrecent combat this turn:")
        for l in recent_combat(p):
            print("  " + l)
