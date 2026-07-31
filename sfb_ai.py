"""
SFB bridge AI — called by the game once per AI ship per planning phase.
Reads battle state JSON from argv[1], writes key=value response to argv[2].

Output format (one per line):
  POSTURE=AGGRESSIVE|DEFENSIVE|EVASIVE|HOLD_RANGE
  SPEED=<int>       (desired speed, -1 = let deterministic AI decide)
  ECM=<int>         (desired ECM, -1 = let deterministic AI decide)
  REPAIR=<int>      (desired repair, -1 = let deterministic AI decide)
  LOG=Role: "line"  (multiple allowed, displayed as bridge chatter)
"""
import sys, json, os, re
import anthropic

FACTION_FLAVOR = {
    "FEDERATION": (
        "You command a Starfleet heavy cruiser. Your crew is professional, disciplined, and scientific. "
        "Balance offense and defense. Seek tactically sound engagements. "
        "Address crew by duty station. Moderate, measured tone."
    ),
    "KLINGON": (
        "You command a Klingon warship. Honor and glory above all. Close range brawling is preferred — "
        "disruptors at knife-fight range. Never suggest retreat without extreme necessity. "
        "Battle cries and challenges are appropriate. 'Qapla!' to open or close comms."
    ),
    "ROMULAN": (
        "You command a Romulan warbird. Patient, cunning, deceptive. Prefer long-range engagements "
        "and plasma torpedo ambushes. Value the cloak. Cold, formal communications. "
        "Never reveal more than necessary."
    ),
    "GORN": (
        "You command a Gorn warship. Slow but immensely powerful. Prefer to close to medium range "
        "where plasma weapons are devastating. Patient, reptilian predator. "
        "Brief, cold, precise communications."
    ),
    "KZINTI": (
        "You command a Kzinti warship. Fast and ferocious. Speed and aggression are your weapons. "
        "Close quickly. Honor demands attacking. Terse, aggressive communications."
    ),
    "THOLIAN": (
        "You command a Tholian patrol corvette. Methodical web-spinners. Prefer defensive positioning. "
        "Cold, alien communications. Patient. Every action is deliberate."
    ),
    "ORION": (
        "You command an Orion pirate raider. Opportunistic and mercenary. "
        "Target the weakest enemy. ECM and hit-and-run. Colorful, piratical attitude — "
        "profit motive always present."
    ),
    "HYDRAN": (
        "You command a Hydran warship. Balanced tacticians with unique Hellbore cannon technology. "
        "Hellbores are most effective at medium range (r5). Gatling phasers for close defense. "
        "Disciplined, knightly communications style."
    ),
    "LYRAN": (
        "You command a Lyran warship. Aggressive feline warriors. Prefer knife-fight range "
        "where Fusion Beams devastate. Proud, territorial, contemptuous of weakness. "
        "Short sharp communications."
    ),
}

SHIELD_NAMES = ["Fwd", "FwdR", "AftR", "Aft", "AftL", "FwdL"]

def build_prompt(state: dict) -> str:
    ship = state["ship"]
    enemies = state.get("enemies", [])
    turn = state.get("turn", 1)
    max_turns = state.get("max_turns", 8)
    faction = ship["faction"].upper()
    flavor = FACTION_FLAVOR.get(faction, "You command a warship. Fight well.")

    hull_pct = int(ship["hull"] * 100 / max(1, ship["hull_max"]))
    shields_desc = ", ".join(
        f"{SHIELD_NAMES[i]}:{ship['shields'][i]}/{ship['shields_max'][i]}"
        for i in range(6)
    )
    weapons_desc = ", ".join(
        f"{w['label']}({'armed' if w['armed'] else 'charging' if not w['disabled'] else 'DESTROYED'})"
        for w in ship["weapons"]
    )

    if enemies:
        enemy_lines = []
        for e in enemies:
            enemy_lines.append(
                f"  {e['name']} ({e['faction']}) — range {e['range']}, hull {e['hull_pct']}%"
            )
        enemies_str = "\n".join(enemy_lines)
    else:
        enemies_str = "  None visible"

    turns_left = max_turns - turn + 1

    return f"""FLEET ACTION BRIEFING — Turn {turn}/{max_turns} ({turns_left} turns remaining)
You command: {ship['name']} ({faction})
{flavor}

SHIP STATUS
  Hull: {ship['hull']}/{ship['hull_max']} ({hull_pct}%)
  Power available: {ship['power']}
  Shields: {shields_desc}
  Weapons: {weapons_desc}

CONTACTS
{enemies_str}

ORDERS:
Issue your tactical posture for this turn and brief bridge crew dialogue.
Respond ONLY with this exact format (no other text):

POSTURE=<AGGRESSIVE|DEFENSIVE|EVASIVE|HOLD_RANGE>
SPEED=<integer, or -1 to leave to tactical computer>
ECM=<0-4, or -1 to leave to tactical computer>
REPAIR=<0-9 in multiples of 3, or -1 to leave to tactical computer>
LOG=<Role>: "<in-character dialogue line>"
LOG=<Role>: "<in-character dialogue line>"
LOG=<Role>: "<in-character dialogue line>"

Use 3-5 LOG lines. Roles: Captain, Helm, Weapons, Engineering, Science, Comms, Shields.
Keep each line under 80 characters. Stay strictly in character for {faction}."""


def parse_response(text: str) -> dict:
    result = {
        "posture": "HOLD_RANGE",
        "speed": -1,
        "ecm": -1,
        "repair": -1,
        "log": [],
    }
    for line in text.splitlines():
        line = line.strip()
        if "=" not in line:
            continue
        key, _, val = line.partition("=")
        key = key.strip().upper()
        val = val.strip()
        if key == "POSTURE" and val in ("AGGRESSIVE", "DEFENSIVE", "EVASIVE", "HOLD_RANGE"):
            result["posture"] = val
        elif key == "SPEED":
            try: result["speed"] = int(val)
            except ValueError: pass
        elif key == "ECM":
            try: result["ecm"] = int(val)
            except ValueError: pass
        elif key == "REPAIR":
            try: result["repair"] = int(val)
            except ValueError: pass
        elif key == "LOG":
            result["log"].append(val)
    return result


def main():
    if len(sys.argv) < 3:
        print("usage: sfb_ai.py <in.json> <out.txt>", file=sys.stderr)
        sys.exit(1)

    with open(sys.argv[1], encoding="utf-8-sig") as f:
        state = json.load(f)

    prompt = build_prompt(state)

    # Load API key from user environment if not already set
    if not os.environ.get("ANTHROPIC_API_KEY"):
        import winreg
        try:
            key = winreg.OpenKey(winreg.HKEY_CURRENT_USER, r"Environment")
            val, _ = winreg.QueryValueEx(key, "ANTHROPIC_API_KEY")
            os.environ["ANTHROPIC_API_KEY"] = val
        except Exception:
            pass

    client = anthropic.Anthropic()
    msg = client.messages.create(
        model="claude-haiku-4-5-20251001",
        max_tokens=400,
        messages=[{"role": "user", "content": prompt}],
    )
    text = msg.content[0].text
    result = parse_response(text)

    with open(sys.argv[2], "w", encoding="utf-8") as f:
        f.write(f"POSTURE={result['posture']}\n")
        f.write(f"SPEED={result['speed']}\n")
        f.write(f"ECM={result['ecm']}\n")
        f.write(f"REPAIR={result['repair']}\n")
        for line in result["log"]:
            f.write(f"LOG={line}\n")


if __name__ == "__main__":
    main()
