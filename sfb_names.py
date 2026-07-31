"""
SFB crew roster manager.

Usage:
  sfb_names.py get    <ship_name> <faction> <out.txt>
  sfb_names.py update <ship_name> <role> <new_name> <out.txt>

Roster file: crew_roster.json alongside this script.
"""
import sys, json, os, pathlib
import anthropic

ROSTER_FILE = pathlib.Path(__file__).parent / "crew_roster.json"

FACTION_NAME_PROMPTS = {
    "FEDERATION": (
        "Generate names for four senior Starfleet officers. "
        "Use plausible human names of mixed Earth cultures (no famous fictional characters). "
        "Include rank abbreviations (Lt., Lt. Cmdr., Cmdr., Ensign, etc.)."
    ),
    "KLINGON": (
        "Generate names for four Klingon bridge officers. "
        "Klingon names are harsh, consonant-heavy: Kor, Kruge, Koloth style. "
        "Titles: HoD, la', 'ach."
    ),
    "ROMULAN": (
        "Generate names for four Romulan bridge officers. "
        "Romulan names are Latinate and formal: Tomalak, Tebok, Keras style. "
        "Ranks: Commander, Sub-Commander, Centurion, Uhlan."
    ),
    "GORN": (
        "Generate names for four Gorn bridge officers. "
        "Gorn names are sibilant and reptilian: Sssark, Kazz, Hriss style."
    ),
    "KZINTI": (
        "Generate names for four Kzinti bridge officers. "
        "Kzinti names are feline and harsh: Chuut-Riit, Raargh, Hroth style. "
        "Titles: Ear, Patriarch, Sergeant."
    ),
    "THOLIAN": (
        "Generate names for four Tholian bridge crew. "
        "Tholian names are crystalline and alien: Loskene-style phonemes. "
        "Titles: Grid-Weaver, Web-Singer."
    ),
    "ORION": (
        "Generate names for four Orion pirate crew. "
        "Mix exotic and rough names: Kodo, Veln, Zara style. "
        "Titles informal: First Mate, Gunner, Scout."
    ),
    "HYDRAN": (
        "Generate names for four Hydran warship officers. "
        "Hydran names are knightly and medieval: Aldric, Theron, Caval style. "
        "Titles: Knight-Commander, Knight, Sergeant-at-Arms, Archivist."
    ),
    "LYRAN": (
        "Generate names for four Lyran bridge officers. "
        "Lyran names are feline and proud: Varek, Shar, Korral, Denth style. "
        "Titles: Pride-Lord, Claw, Fang, Scout."
    ),
}

ROLE_TITLES = {
    "FEDERATION":  ["Helm Officer",   "Tactical Officer", "Chief Engineer",  "Science Officer"],
    "KLINGON":     ["Helmsman",        "Weapons Master",   "Engineer",        "Scanner"],
    "ROMULAN":     ["Sub-Commander",   "Centurion",        "Technician",      "Intelligence"],
    "GORN":        ["Helm",            "Weapons",          "Engineering",     "Science"],
    "KZINTI":      ["Helm",            "Weapons",          "Engineering",     "Science"],
    "THOLIAN":     ["Grid Weaver",     "Web Master",       "Energy Node",     "Sensor Web"],
    "ORION":       ["Helmsman",        "Gunner",           "Engineer",        "Scout"],
    "HYDRAN":      ["Helm Knight",     "Cannon Master",    "Engineering",     "Archivist"],
    "LYRAN":       ["Helm",            "Weapons",          "Engineering",     "Science"],
}

ROLES = ["helm", "weapons", "engineering", "science"]


def load_roster():
    if ROSTER_FILE.exists():
        try:
            return json.loads(ROSTER_FILE.read_text(encoding="utf-8"))
        except Exception:
            return {}
    return {}


def save_roster(roster):
    ROSTER_FILE.write_text(json.dumps(roster, indent=2, ensure_ascii=False), encoding="utf-8")


def ensure_api_key():
    if not os.environ.get("ANTHROPIC_API_KEY"):
        try:
            import winreg
            k = winreg.OpenKey(winreg.HKEY_CURRENT_USER, r"Environment")
            val, _ = winreg.QueryValueEx(k, "ANTHROPIC_API_KEY")
            os.environ["ANTHROPIC_API_KEY"] = val
        except Exception:
            pass


def generate_names(ship_name, faction):
    ensure_api_key()
    faction = faction.upper()
    guidance = FACTION_NAME_PROMPTS.get(faction, FACTION_NAME_PROMPTS["FEDERATION"])
    titles = ROLE_TITLES.get(faction, ROLE_TITLES["FEDERATION"])
    prompt = (
        f"Ship: {ship_name}  Faction: {faction}\n\n"
        f"{guidance}\n\n"
        "Respond with EXACTLY these four lines and nothing else:\n"
        f"HELM=<name and rank for {titles[0]}>\n"
        f"WEAPONS=<name and rank for {titles[1]}>\n"
        f"ENGINEERING=<name and rank for {titles[2]}>\n"
        f"SCIENCE=<name and rank for {titles[3]}>"
    )
    client = anthropic.Anthropic()
    msg = client.messages.create(
        model="claude-haiku-4-5-20251001",
        max_tokens=120,
        messages=[{"role": "user", "content": prompt}],
    )
    names = {}
    for line in msg.content[0].text.splitlines():
        if "=" in line:
            key, _, val = line.partition("=")
            key = key.strip().upper()
            if key in ("HELM", "WEAPONS", "ENGINEERING", "SCIENCE"):
                names[key.lower()] = val.strip()
    for i, role in enumerate(ROLES):
        if role not in names:
            names[role] = titles[i]
    return names


def cmd_get(ship_name, faction, out_path):
    roster = load_roster()
    if ship_name not in roster:
        roster[ship_name] = generate_names(ship_name, faction)
        save_roster(roster)
    crew = roster[ship_name]
    with open(out_path, "w", encoding="utf-8") as f:
        for role in ROLES:
            f.write(f"{role.upper()}={crew.get(role, role.capitalize())}\n")


def cmd_update(ship_name, role, new_name, out_path):
    roster = load_roster()
    if ship_name not in roster:
        roster[ship_name] = {}
    roster[ship_name][role.lower()] = new_name
    save_roster(roster)
    with open(out_path, "w", encoding="utf-8") as f:
        f.write("OK\n")


def main():
    if len(sys.argv) < 2:
        sys.exit(1)
    cmd = sys.argv[1].lower()
    if cmd == "get" and len(sys.argv) == 5:
        cmd_get(sys.argv[2], sys.argv[3], sys.argv[4])
    elif cmd == "update" and len(sys.argv) == 6:
        cmd_update(sys.argv[2], sys.argv[3], sys.argv[4], sys.argv[5])
    else:
        sys.exit(1)


if __name__ == "__main__":
    main()
