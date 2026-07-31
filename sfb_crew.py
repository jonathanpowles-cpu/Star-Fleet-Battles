"""
SFB crew advisory layer — called during the player's planning phase.
Four crew roles each give a focused in-character recommendation.

Input:  argv[1] = JSON state file (same schema as sfb_ai.py)
Output: argv[2] = key=value pairs

Output format:
  HELM=<line>          (speed/heading advice)
  WEAPONS=<line>       (what to charge or fire)
  ENGINEERING=<line>   (power allocation note)
  SCIENCE=<line>       (threat assessment / enemy analysis)
  CLOAK=0|1            (recommend cloaking this turn)
  SPEED=<int>          (recommended speed, -1 = no opinion)
  CHARGE=<weapon_label> (recommend charging this weapon)
"""
import sys, json, os
import anthropic

FACTION_CREW = {
    "FEDERATION": {
        "helm":        ("Helm",        "precise, cool-headed navigator"),
        "weapons":     ("Tactical",    "eager, professional tactical officer"),
        "engineering": ("Engineering", "passionate, dedicated chief engineer"),
        "science":     ("Science",     "coldly logical science officer"),
    },
    "KLINGON": {
        "helm":        ("Helmsman",    "aggressive, honour-driven"),
        "weapons":     ("Gunner",      "bloodthirsty, terse"),
        "engineering": ("Engineer",    "proud of the ship's power"),
        "science":     ("Scanner",     "blunt tactician, no patience for weakness"),
    },
    "ROMULAN": {
        "helm":        ("Sub-Commander", "patient, calculating"),
        "weapons":     ("Centurion",   "cold precision, prefers ambush"),
        "engineering": ("Technician",  "formal, efficient"),
        "science":     ("Intelligence","analytical, suspicious, paranoid"),
    },
    "GORN": {
        "helm":        ("Helm",        "slow deliberate reptilian"),
        "weapons":     ("Weapons",     "patient, powerful, waits for the right shot"),
        "engineering": ("Engineering", "proud of plasma capacity"),
        "science":     ("Science",     "cold predator's assessment"),
    },
    "KZINTI": {
        "helm":        ("Helm",        "fierce, speed-obsessed"),
        "weapons":     ("Weapons",     "aggressive, wants blood"),
        "engineering": ("Engineering", "pushing the engines hard"),
        "science":     ("Science",     "threat-hunter"),
    },
    "THOLIAN": {
        "helm":        ("Grid Weaver", "methodical, alien logic"),
        "weapons":     ("Web Master",  "patient, deliberate"),
        "engineering": ("Energy Node", "precise, alien"),
        "science":     ("Sensor Web",  "alien observation, no emotion"),
    },
    "ORION": {
        "helm":        ("Helmsman",    "opportunistic, watching exit routes"),
        "weapons":     ("Gunner",      "mercenary, targets value not honor"),
        "engineering": ("Engineer",    "stretching the engines for profit"),
        "science":     ("Scout",       "appraising enemy ship value as salvage"),
    },
    "HYDRAN": {
        "helm":        ("Helm Knight", "disciplined, knightly"),
        "weapons":     ("Cannon Master","precise, proud of Hellbore tech"),
        "engineering": ("Engineering", "measured, professional"),
        "science":     ("Science",     "analytical, tactical"),
    },
    "LYRAN": {
        "helm":        ("Helm",        "aggressive feline, wants to close"),
        "weapons":     ("Weapons",     "eager to fire ESG and Fusion beams"),
        "engineering": ("Engineering", "proud of power capacity"),
        "science":     ("Science",     "predator assessing prey"),
    },
}

SHIELD_NAMES = ["Fwd", "FwdR", "AftR", "Aft", "AftL", "FwdL"]


def build_crew_prompt(state: dict) -> str:
    ship = state["ship"]
    enemies = state.get("enemies", [])
    turn = state.get("turn", 1)
    max_turns = state.get("max_turns", 8)
    faction = ship["faction"].upper()
    crew = FACTION_CREW.get(faction, FACTION_CREW["FEDERATION"])
    # Override role display names with persisted crew names if provided
    crew_names = ship.get("crew", {})
    if crew_names:
        crew = {
            role: (crew_names.get(role, crew[role][0]), crew[role][1])
            for role in ("helm", "weapons", "engineering", "science")
        }

    hull_pct = int(ship["hull"] * 100 / max(1, ship["hull_max"]))
    shields = [f"{SHIELD_NAMES[i]}:{ship['shields'][i]}/{ship['shields_max'][i]}"
               for i in range(6)]

    weapons_lines = []
    for w in ship["weapons"]:
        status = "DESTROYED" if w["disabled"] else ("armed" if w["armed"] else f"charging {w['charge']}/{w['arming_turns']}" if w["arming_turns"] > 0 else "instant")
        weapons_lines.append(f"  {w['label']} [{w['type']}] - {status}")

    enemies_lines = []
    for e in enemies:
        enemies_lines.append(
            f"  {e['name']} ({e['faction']}) range={e['range']} hull={e['hull_pct']}%"
            + (f" shields: " + "/".join(str(s) for s in e.get("shields", [])) if e.get("shields") else "")
        )

    return f"""You are the bridge crew of {ship['name']} ({faction}), turn {turn}/{max_turns}.

SHIP STATUS
  Hull: {ship['hull']}/{ship['hull_max']} ({hull_pct}%)
  Power: {ship['power']} available
  Shields: {", ".join(shields)}
  Speed EAF: {ship.get('speed', 0)}

WEAPONS
{chr(10).join(weapons_lines)}

CONTACTS
{chr(10).join(enemies_lines) if enemies_lines else "  None"}

Each crew role gives ONE short recommendation. Respond in EXACTLY this format (no other text):

HELM={crew['helm'][0]} ({crew['helm'][1]}): <speed/heading advice, max 80 chars>
WEAPONS={crew['weapons'][0]} ({crew['weapons'][1]}): <weapon charge or fire advice, max 80 chars>
ENGINEERING={crew['engineering'][0]} ({crew['engineering'][1]}): <power allocation note, max 80 chars>
SCIENCE={crew['science'][0]} ({crew['science'][1]}): <threat assessment, max 80 chars>
SPEED=<recommended speed integer, or -1>
CHARGE=<exact weapon label to prioritise charging, or NONE>
CLOAK=<1 to recommend cloaking this turn, 0 otherwise>"""


def main():
    if len(sys.argv) < 3:
        print("usage: sfb_crew.py <in.json> <out.txt>", file=sys.stderr)
        sys.exit(1)

    with open(sys.argv[1], encoding="utf-8-sig") as f:
        state = json.load(f)

    prompt = build_crew_prompt(state)

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
        max_tokens=512,
        messages=[{"role": "user", "content": prompt}],
    )
    text = msg.content[0].text

    with open(sys.argv[2], "w", encoding="utf-8") as out:
        for line in text.splitlines():
            line = line.strip()
            if any(line.startswith(k) for k in
                   ("HELM=", "WEAPONS=", "ENGINEERING=", "SCIENCE=",
                    "SPEED=", "CHARGE=", "CLOAK=")):
                out.write(line + "\n")


if __name__ == "__main__":
    main()
