"""
SFB tactical brain for the online driver.

Two layers, ported from the home-grown interface:
  * high-level posture + faction personality + bridge chatter  (from sfb_ai.py)
  * deterministic execution: target/heading/speed/EAF           (sfb_tactics.py)

Plugs into sfb_client via make_brain(): the returned callable reads decoded
pieces, builds a TacticalShip view, decides, and issues driver commands.

Claude is OPTIONAL — if the anthropic SDK + API key are present it sets posture
and bridge dialogue; otherwise the deterministic layer runs with HOLD_RANGE.
"""

from __future__ import annotations

import os
from typing import Optional

import sfb_hex as hexlib
import sfb_tactics as tac
import sfb_ssd
import sfb_advanced as adv
from sfb_rules import (Weapon, make_ph1, make_ph2, make_ph3, make_photon,
                       make_disruptor, make_plasma_r, make_plasma_f, make_drone,
                       make_hellbore, make_fusion, SHIELD_NAMES)


# --------------------------------------------------------------------------
# Faction personalities (ported verbatim from sfb_ai.py)
# --------------------------------------------------------------------------

FACTION_FLAVOR = {
    "FEDERATION": ("You command a Starfleet heavy cruiser. Professional, disciplined, scientific. "
                   "Balance offense and defense. Address crew by duty station. Measured tone."),
    "KLINGON": ("You command a Klingon warship. Honor and glory above all. Close-range brawling with "
                "disruptors. Never retreat without extreme necessity. 'Qapla!' to open or close comms."),
    "ROMULAN": ("You command a Romulan warbird. Patient, cunning, deceptive. Long-range plasma ambushes. "
                "Value the cloak. Cold, formal comms. Never reveal more than necessary."),
    "GORN": ("You command a Gorn warship. Slow but immensely powerful. Close to medium range where plasma "
             "devastates. Patient reptilian predator. Brief, cold, precise."),
    "KZINTI": ("You command a Kzinti warship. Fast and ferocious. Speed and drone swarms are your weapons. "
               "Close quickly. Honor demands attacking. Terse, aggressive."),
    "THOLIAN": ("You command a Tholian patrol corvette. Methodical web-spinners. Defensive positioning. "
                "Cold, alien, patient. Every action deliberate."),
    "ORION": ("You command an Orion pirate raider. Opportunistic, mercenary. Target the weakest enemy. "
              "ECM and hit-and-run. Piratical, profit-minded."),
    "HYDRAN": ("You command a Hydran warship. Balanced tactician with Hellbore cannons (best at r5) and "
               "gatling phasers for close defense. Knightly, disciplined."),
    "LYRAN": ("You command a Lyran warship. Aggressive feline warrior. Knife-fight range where Fusion Beams "
              "devastate. Proud, territorial, contemptuous of weakness. Short, sharp."),
}


# Default weapon loadouts by faction (reference ships, mechanics §16) so the fire
# planner has something until SSD-parsed weapons are wired in.
def default_weapons(faction: str) -> list:
    f = faction.upper()
    if f == "FEDERATION":
        return [make_ph1("Ph-1 #1"), make_ph1("Ph-1 #2"), make_ph1("Ph-1 #3"),
                make_photon("Photon A"), make_photon("Photon B")]
    if f == "KLINGON":
        return [make_ph2("Ph-2 #1"), make_ph2("Ph-2 #2"), make_ph2("Ph-2 #3"),
                make_ph2("Ph-2 #4"), make_disruptor("Disr #1"), make_disruptor("Disr #2"),
                make_drone("Drone")]
    if f == "ROMULAN":
        return [make_ph1("Ph-1 #1"), make_ph1("Ph-1 #2"),
                make_plasma_r("Plasma-R L"), make_plasma_r("Plasma-R R")]
    if f == "GORN":
        return [make_ph1("Ph-1 #1"), make_ph1("Ph-1 #2"),
                make_plasma_f("Plasma-F A"), make_plasma_f("Plasma-F B")]
    if f == "KZINTI":
        return [make_ph1("Ph-1 #1"), make_ph1("Ph-1 #2"),
                make_drone("Drone A"), make_drone("Drone B")]
    if f == "HYDRAN":
        return [make_ph3("Ph-3 #1"), make_ph3("Ph-3 #2"),
                make_hellbore("Hellbore A"), make_hellbore("Hellbore B")]
    if f == "LYRAN":
        return [make_ph1("Ph-1 #1"), make_ph1("Ph-1 #2"),
                make_fusion("Fusion A"), make_fusion("Fusion B")]
    return [make_ph1("Ph-1 #1"), make_ph1("Ph-1 #2")]


# --------------------------------------------------------------------------
# Claude posture layer (optional)
# --------------------------------------------------------------------------

def build_prompt(me: tac.TacticalShip, enemies: list, turn: int, max_turns: int) -> str:
    flavor = FACTION_FLAVOR.get(me.faction, "You command a warship. Fight well.")
    contacts = "\n".join(
        f"  {e.obj_id} ({e.faction}) — range {hexlib.hex_distance(me.pos, e.pos)}, hull {e.hull_pct}%"
        for e in enemies) or "  None visible"
    return f"""FLEET ACTION — Turn {turn}/{max_turns}
You command: {me.obj_id} ({me.faction})
{flavor}

SHIP STATUS
  Hull: {me.hull}/{me.hull_max} ({me.hull_pct}%)   Power: {me.power}   Speed: {me.speed}
  Position: hex {me.pos[0]:02d}{me.pos[1]:02d}, facing {me.facing}

CONTACTS
{contacts}

Respond ONLY in this format:
POSTURE=<AGGRESSIVE|DEFENSIVE|EVASIVE|HOLD_RANGE>
SPEED=<integer, or -1 for tactical computer>
LOG=<Role>: "<in-character line>"
LOG=<Role>: "<in-character line>"
Stay in character for {me.faction}."""


def parse_response(text: str) -> dict:
    out = {"posture": "HOLD_RANGE", "speed": -1, "log": []}
    for line in text.splitlines():
        if "=" not in line:
            continue
        k, _, v = line.partition("=")
        k, v = k.strip().upper(), v.strip()
        if k == "POSTURE" and v in ("AGGRESSIVE", "DEFENSIVE", "EVASIVE", "HOLD_RANGE"):
            out["posture"] = v
        elif k == "SPEED":
            try: out["speed"] = int(v)
            except ValueError: pass
        elif k == "LOG":
            out["log"].append(v)
    return out


def _load_api_key() -> Optional[str]:
    if os.environ.get("ANTHROPIC_API_KEY"):
        return os.environ["ANTHROPIC_API_KEY"]
    try:  # user-env fallback (Windows), as in sfb_ai.py
        import winreg
        k = winreg.OpenKey(winreg.HKEY_CURRENT_USER, r"Environment")
        val, _ = winreg.QueryValueEx(k, "ANTHROPIC_API_KEY")
        return val
    except Exception:
        return None


def claude_posture(me: tac.TacticalShip, enemies: list, turn: int, max_turns: int) -> Optional[dict]:
    key = _load_api_key()
    if not key:
        return None
    try:
        import anthropic
        os.environ["ANTHROPIC_API_KEY"] = key
        client = anthropic.Anthropic()
        msg = client.messages.create(
            model="claude-haiku-4-5-20251001", max_tokens=400,
            messages=[{"role": "user", "content": build_prompt(me, enemies, turn, max_turns)}],
        )
        return parse_response(msg.content[0].text)
    except Exception as e:
        print(f"[brain] Claude unavailable ({e}); using deterministic posture", flush=True)
        return None


# --------------------------------------------------------------------------
# LLM-to-engine wiring: the deterministic engine computes the numbers; the LLM
# supplies judgment + personality. It never sees raw rules tables — only the
# doctrine (system prompt) and the engine's computed brief (user turn).
# --------------------------------------------------------------------------
import sfb_doctrine as DOC

# Better than Haiku for tactical judgment; override per call. See note in README.
MODEL_DEFAULT = "claude-sonnet-5"       # balanced; "claude-opus-4-8" for the hardest AI
MODEL_FAST = "claude-haiku-4-5-20251001"


def build_system_prompt(faction: str) -> str:
    """Compact tactical doctrine (the 'skill' content) — principles, not tables.
    The engine already did the arithmetic; the model applies judgment."""
    f = faction.upper()
    return (
        "You are the tactical mind of a Star Fleet Battles captain. A rules engine "
        "has already computed all the hard numbers (damage, EW shifts, alpha strikes, "
        "overload/HET viability) — trust its brief; do not recompute or invent tables.\n\n"
        "DOCTRINE (apply, don't recite):\n"
        f"- {DOC.MIZIA['principle']}\n"
        "- Knock a shield down, then concentrate mini-volleys on it (Mizia). A medium-range "
        "shot on a DOWN shield beats a short-range shot on a strong one.\n"
        "- Energy: pre-plot housekeeping; never waste power; phasers beat heavy weapons except "
        "the last arming turn or bad range/EW; don't dump spare energy into shield reinforcement.\n"
        "- 1 reserve ECM can save ~10 shield reinforcement. Overloads only reach 8 hexes.\n"
        "- Fire by Impulse #25 (Impulse of Decision) so weapons recycle. Withhold a phaser to "
        "complicate his planning.\n"
        f"- Your doctrine ({f}): {DOC.RACE_DOCTRINE.get(f, 'fight well')}\n\n"
        "Given the engine's brief, decide: target, posture, whether to overload/HET, and speak "
        "3-5 lines of in-character bridge dialogue. Be decisive."
    )


def brief_to_text(me, brief) -> str:
    lines = [f"ENGINE BRIEF for {me.obj_id}:", brief.summary, "", "Recommendations:"]
    lines += [f"  - {r}" for r in brief.recommendations]
    if brief.fire_plan:
        lines.append("Fire solutions (weapon -> exp dmg on shield):")
        lines += [f"  {s.weapon.label}: {s.expected:.0f} on shield #{s.target_shield+1} (r{s.rng})"
                  for s in brief.fire_plan[:8]]
    return "\n".join(lines)


def battle_advice(me, enemies, *, turn: int = 1, impulse: int = 1,
                  model: str = MODEL_DEFAULT) -> Optional[dict]:
    """Full sophisticated decision: engine brief -> LLM judgment + dialogue.
    Returns parse_response()-style dict, or None if the API is unavailable."""
    brief = adv.assess(me, enemies, impulse, turn)
    key = _load_api_key()
    if not key:
        # engine-only fallback: still fully playable, just no dialogue
        return {"posture": "HOLD_RANGE", "speed": -1, "ecm": -1, "repair": -1,
                "log": [f'Tactical: "{r}"' for r in brief.recommendations[:3]],
                "brief": brief}
    try:
        import anthropic
        os.environ["ANTHROPIC_API_KEY"] = key
        client = anthropic.Anthropic()
        msg = client.messages.create(
            model=model, max_tokens=600,
            system=build_system_prompt(me.faction),
            messages=[{"role": "user", "content": brief_to_text(me, brief) +
                       "\n\nIssue orders in the format:\nPOSTURE=<AGGRESSIVE|DEFENSIVE|"
                       "EVASIVE|HOLD_RANGE>\nSPEED=<int or -1>\nECM=<int or -1>\n"
                       "REPAIR=<int or -1>\nLOG=<Role>: \"<line>\" (3-5 lines)"}],
        )
        out = parse_response(msg.content[0].text)
        out["brief"] = brief
        return out
    except Exception as e:
        print(f"[brain] LLM unavailable ({e}); engine-only decision", flush=True)
        return {"posture": "HOLD_RANGE", "speed": -1, "ecm": -1, "repair": -1,
                "log": [f'Tactical: "{r}"' for r in brief.recommendations[:3]],
                "brief": brief}


# --------------------------------------------------------------------------
# Build a TacticalShip from a decoded online Piece (+ optional SSD)
# --------------------------------------------------------------------------

def piece_to_tactical(piece, power: int = 20) -> Optional[tac.TacticalShip]:
    if piece.xy is None:
        return None
    faction = (piece.race or "FEDERATION").upper()
    hull, hull_max = 100, 100
    shields = [0] * 6
    shields_max = [0] * 6
    weapons = default_weapons(faction)

    ssd = piece.attrs.get("SSD")
    if sfb_ssd.ssd_available(ssd):
        parsed = sfb_ssd.parse_ssd(ssd)
        hull, hull_max = parsed.hull, parsed.hull_max
        shields, shields_max = parsed.shields, parsed.shields_max
        if parsed.weapons:                 # real SSD weapons override defaults
            weapons = parsed.weapons

    return tac.TacticalShip(
        obj_id=piece.label or piece.obj_id,
        faction=faction,
        pos=piece.xy,
        facing=piece.facing if piece.facing is not None else 0,
        speed=piece.speed if piece.speed is not None else 0,
        hull=hull, hull_max=hull_max, power=power,
        last_speed=piece.speed if piece.speed is not None else 0,
        turn_mode_cat=4,
        weapons=weapons,
        shields=shields, shields_max=shields_max,
    )


# --------------------------------------------------------------------------
# The Brain callable for sfb_client
# --------------------------------------------------------------------------

def tactical_briefs(client, state, my_nick: str, turn: int = 1, impulse: int = 1):
    """Build a sophisticated Assessment (sfb_advanced) for each of my ships."""
    pieces = state.snapshot()
    ships = [(p, piece_to_tactical(p)) for p in pieces if p.obj_id.startswith("gp*")]
    ships = [(p, t) for p, t in ships if t]
    mine = [t for p, t in ships if (t.obj_id and my_nick.lower() in (p.owner or "").lower())]
    enemies = [t for p, t in ships if not (my_nick.lower() in (p.owner or "").lower())]
    return [(t, adv.assess(t, enemies, impulse, turn)) for t in mine]


def advisor_brain(my_nick: str, *, turn: int = 1, impulse: int = 1):
    """A read-only Brain that prints sophisticated per-ship tactical advice
    (Mizia, EW, overload, power-curve, timing). Plug into SFBGameClient.run()."""
    def brain(client, state):
        briefs = tactical_briefs(client, state, my_nick, turn, impulse)
        if not briefs:
            print("[advisor] no owned ships in view")
            return
        for me, b in briefs:
            print(f"\n=== ADVICE: {me.obj_id} ===")
            print(b.summary)
            for r in b.recommendations:
                print("  • " + r)
            if b.fire_plan:
                fp = ", ".join(f"{s.weapon.label}->#{s.target_shield+1}({s.expected:.0f})"
                               for s in b.fire_plan[:6])
                print("  FIRE: " + fp)
    return brain


def make_brain(my_nick: str, *, use_claude: bool = True, max_turns: int = 8,
               dry_run: bool = True):
    """Return a brain(client, state) for SFBGameClient.run().

    dry_run=True issues no moves — it only reports the plan (safe for testing
    against a live human game). Set dry_run=False to actually drive ships.
    """
    turn_counter = {"n": 0}

    def brain(client, state):
        turn_counter["n"] += 1
        pieces = state.snapshot()
        mine = [p for p in pieces if p.owner == my_nick and p.xy is not None]
        foes = [p for p in pieces if p.owner and p.owner != my_nick and p.xy is not None]
        if not mine:
            print("[brain] no owned ships yet", flush=True)
            return
        enemies_t = [t for t in (piece_to_tactical(p) for p in foes) if t]
        for p in mine:
            me = piece_to_tactical(p)
            if me is None:
                continue
            posture, logs = "HOLD_RANGE", []
            if use_claude:
                r = claude_posture(me, enemies_t, turn_counter["n"], max_turns)
                if r:
                    posture, logs = r["posture"], r["log"]
            plan = tac.make_plan(me, enemies_t, posture,
                                 speed_override=(r["speed"] if use_claude and r else -1))
            new_face = tac.limited_turn(me, plan.heading)
            _report(me, plan, new_face, logs)
            if not dry_run:
                client.set_speed(p.obj_id, plan.speed)
                if new_face != me.facing:
                    client.turn(p.obj_id, new_face)
                # NOTE: actual weapon fire uses the game's activity/fire protocol,
                # wired in a later step; plan.fire lists the intended shots.
    return brain


def _report(me: tac.TacticalShip, plan, new_face: int, logs: list) -> None:
    print(f"[brain] {me.obj_id} ({me.faction}) hex {me.pos[0]:02d}{me.pos[1]:02d} "
          f"face {me.facing}->{new_face} | posture {plan.posture} "
          f"{'RETREAT ' if plan.retreating else ''}speed {plan.speed}", flush=True)
    for fo in plan.fire:
        print(f"        FIRE {fo.weapon.label} -> {fo.target.obj_id} "
              f"@r{fo.range} ~{fo.expected_damage}dmg", flush=True)
    for line in logs:
        print(f"        {line}", flush=True)
