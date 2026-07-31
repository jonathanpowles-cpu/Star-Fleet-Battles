"""
In-character tactical advisor for a live SFB Online battle.

Connects to the tactical room as an observer, reads the board (positions, facings,
speeds - all available live over the object protocol), computes the tactical picture
with the engine (sfb_hex geometry + sfb_advanced where SSD data is available), and
speaks to the human through the game's own chat channel, in the voice of the flagship
bridge officers of the advised race.

    SFB_PASSWORD=x python sfb_advisor.py --room "#SFB_Game1" --side Lyran [--loop 20]

Chat output is proven to apply live even though board writes do not, so this is a
read-only advisor that talks to the player - never touches their ships.
"""
from __future__ import annotations
import os, sys, time, argparse
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from sfb_client import SFBGameClient
import sfb_hex as H

# --------------------------------------------------------------------------
# Bridge personas per race (the officers who address the commander)
# --------------------------------------------------------------------------

BRIDGE = {
    "LYRAN": {
        "address": "My Lord",
        "helm": "Helm",
        "weps": "Master Gunner",
        "sci": "Sensors",
        "flavor": "proud, aggressive Lyran count's-fleet officers; ESG walls and the coordinated pounce",
    },
    "KZINTI": {
        "address": "Admiral",
        "helm": "Navigation",
        "weps": "Weapons",
        "sci": "Sensors",
        "flavor": "disciplined Kzinti Hegemony officers; drone saturation and carrier-group doctrine",
    },
}
DEFAULT_BRIDGE = {"address": "Captain", "helm": "Helm", "weps": "Weapons", "sci": "Sensors", "flavor": "professional starship officers"}

SHIELD_NAMES = {0: "#1 (fore)", 1: "#2 (fore-stbd)", 2: "#3 (aft-stbd)", 3: "#4 (aft)", 4: "#5 (aft-port)", 5: "#6 (fore-port)"}


def bearing_clock(src, dst):
    """Rough clock bearing (1-6 sextant) of dst from src, absolute."""
    return H.relative_sextant(src, dst, 0)


def facing_of(p):
    return p.facing if p.facing is not None else 0


def speed_of(p):
    try:
        return int(p.speed) if p.speed is not None else 0
    except Exception:
        return 0


def assess_pair(me, foe):
    """Geometry-only tactical read of one of my ships vs one enemy."""
    rng = H.hex_distance(me.xy, foe.xy)
    # which of the enemy's shields faces me (what I'd hit)
    tgt_shield = H.shield_hit(foe.xy, facing_of(foe), me.xy)
    # which of MY shields faces the enemy (what he'd hit)
    my_shield = H.shield_hit(me.xy, facing_of(me), foe.xy)
    # is the enemy in my forward firing arc (FA/FH ~ facing sextant +/-1)?
    in_fa = H.target_in_arc(me.xy, facing_of(me), 0b111, foe.xy) if hasattr(H, "target_in_arc") else True
    return {
        "range": rng,
        "tgt_shield": tgt_shield,
        "my_shield": my_shield,
        "in_arc": in_fa,
    }


def officer_lines(side, my_ship, foe, a):
    """Templated in-character advice with real numbers (LLM polish optional)."""
    b = BRIDGE.get(side.upper(), DEFAULT_BRIDGE)
    addr = b["address"]
    rng = a["range"]
    lines = []
    # Sensors: contact report
    lines.append(f'{b["sci"]}: Contact - {foe.label} bearing on {my_ship.label}, range {rng} hexes. '
                 f'His {SHIELD_NAMES[a["tgt_shield"]]} shield faces us.')
    # Weapons: firing recommendation by range band
    if rng <= 3:
        lines.append(f'{b["weps"]}: Point-blank, {addr}! Overloads will double damage but mind the feedback. '
                     f'Concentrate on his down shield if he shows one.')
    elif rng <= 8:
        lines.append(f'{b["weps"]}: In overload range ({rng} hexes). Recommend overloaded heavy weapons on his '
                     f'{SHIELD_NAMES[a["tgt_shield"]]}; hold phasers to finish.')
    elif rng <= 15:
        lines.append(f'{b["weps"]}: Medium range. Standard loads only - disruptors still bite, photons weaken. '
                     f'Wear down that facing shield before we close.')
    else:
        lines.append(f'{b["weps"]}: Range {rng} - beyond effective fire. Hold and let them come, or close under EW.')
    # Helm: relative-shield / posture note
    lines.append(f'{b["helm"]}: We present our {SHIELD_NAMES[a["my_shield"]]} to {foe.label}. '
                 + ("Recommend a turn to keep a fresh shield toward him." if a["my_shield"] in (3,) else "Attitude acceptable for now."))
    return lines


def build_brief(state, side):
    pieces = [p for p in state.snapshot() if p.obj_id.startswith("gp*") and p.xy and p.label]
    mine = [p for p in pieces if (p.race or "").upper() == side.upper()]
    foes = [p for p in pieces if (p.race or "").upper() != side.upper()]
    if not mine or not foes:
        return None, mine, foes
    # For the brief: each of my ships vs its nearest enemy
    reports = []
    for m in mine:
        nearest = min(foes, key=lambda f: H.hex_distance(m.xy, f.xy))
        a = assess_pair(m, nearest)
        reports.append((m, nearest, a))
    # Sort by most urgent (closest engagement first)
    reports.sort(key=lambda r: r[2]["range"])
    return reports, mine, foes


def llm_polish(side, reports):
    """Optional: let Claude voice the bridge in-character. Falls back to templates."""
    try:
        import anthropic
    except Exception:
        return None
    key = os.environ.get("ANTHROPIC_API_KEY")
    if not key:
        return None
    b = BRIDGE.get(side.upper(), DEFAULT_BRIDGE)
    facts = []
    for m, foe, a in reports[:6]:
        facts.append(f"- {m.label}: nearest enemy {foe.label} at range {a['range']}, "
                     f"his {SHIELD_NAMES[a['tgt_shield']]} faces us, we show our {SHIELD_NAMES[a['my_shield']]}.")
    prompt = ("You are the bridge crew of the flagship of a " + side + " fleet in a Star Fleet Battles engagement. "
              "Speak to your commander (" + b["address"] + ") IN CHARACTER as " + b["flavor"] + ". "
              "Give a SHORT tactical brief (4-6 lines max, officers by station) based ONLY on these facts, "
              "with concrete range/shield advice. Do not invent systems or damage.\n\n" + "\n".join(facts))
    try:
        c = anthropic.Anthropic(api_key=key)
        r = c.messages.create(model="claude-sonnet-5", max_tokens=400,
                              messages=[{"role": "user", "content": prompt}])
        return r.content[0].text.strip()
    except Exception as e:
        print(f"[advisor] LLM unavailable ({e}); using templated voice")
        return None


def run_once(client, side, use_llm=True):
    reports, mine, foes = build_brief(client.state, side)
    if not reports:
        print(f"[advisor] need both sides in view (mine={len(mine)} foes={len(foes)})")
        return
    print(f"[advisor] {len(mine)} {side} ships vs {len(foes)} enemy; nearest engagement at range {reports[0][2]['range']}")
    text = llm_polish(side, reports) if use_llm else None
    if text is None:
        # templated: brief the two most urgent engagements
        lines = [f"-- {side} flagship tactical brief -"]
        for m, foe, a in reports[:2]:
            lines += officer_lines(side, m, foe, a)
        text = "\n".join(lines)
    room = client.room
    for line in text.splitlines():
        line = line.strip()
        if line:
            client.conn.say(room, line)
            time.sleep(0.4)
    print("[advisor] brief sent to chat")


def main():
    ap = argparse.ArgumentParser(description="In-character SFB tactical advisor")
    ap.add_argument("--host", default="127.0.0.1")
    ap.add_argument("--port", type=int, default=6668)
    ap.add_argument("--room", default="#SFB_Game1")
    ap.add_argument("--side", default="Lyran", help="race to advise (Lyran/Kzinti)")
    ap.add_argument("--nick", default="FlagBridge")
    ap.add_argument("--loop", type=int, default=0, help="seconds between briefs (0 = one-shot)")
    ap.add_argument("--no-llm", action="store_true")
    args = ap.parse_args()

    pw = os.environ.get("SFB_PASSWORD", "relaydummy")
    c = SFBGameClient(args.host, args.port, args.nick, args.room, verbose=False)
    c.start(pw)
    time.sleep(2.5)
    try:
        run_once(c, args.side, use_llm=not args.no_llm)
        while args.loop > 0:
            time.sleep(args.loop)
            run_once(c, args.side, use_llm=not args.no_llm)
    finally:
        c.conn.disconnect()


if __name__ == "__main__":
    main()
