"""
Weapon damage charts, parsed from the client's own data rather than transcribed.

The engine used to carry two disagreeing sets of tables: hand-typed ones in
sfb_rules (`_PH1_TBL`, `_PH2_TBL`, `_DISR_TBL`) and sourced ones in sfb_command.
Auditing both against `client_data/weapons.chart` settled it - the sfb_command
tables were exact, the sfb_rules ones were fabricated:

  * `_PH2_TBL` matched the real Phaser-2 chart in ZERO rows, and stopped after
    four range brackets, so it zeroed a weapon that actually reaches range 50.
  * `PH3_EXPECTED` claimed 4.5 at range 0 where the chart averages 3.833, and
    1.5 at range 5 where the real 4-8 bracket averages 0.333 - on a live threat
    path, so the engine thought a ph-3 was ~4x the weapon it is.

Rather than fix the transcription - the same act that introduced the errors -
the charts are READ from the client files at import. One source, the one the
client itself resolves combat with, and it cannot drift.

FILE FORMAT (client_data/weapons.chart and the module_*_weapons.chart files):

    Phaser1,PHASER-1 CHART
    RANGE,0,1,2,3,4,5,6-8,9-15,16-25,26-50,51-75
    1,9,8,7,6,5,5,4,3,2,1,1      <- die roll 1, damage per range bracket
    ...
    6,4,4,3,3,2,2,0,0,0,0,0

    Disr,DISRUPTOR CHART
    RANGE,0,1,2,3-4,5-8,9-15,16-22,23-30,31-40
    STD,NA,1-5,1-5,...           <- hit numbers ("1-5" = hits on a d6 of 1-5)
    DMG-STD,0,5,4,4,3,3,2,2,1    <- damage per hit
    DMG-OVLD,10,10,8,8,6,0,0,0,0

Phasers are rolled per shot, so what the advice wants is the EXPECTED value over
a fair d6. Heavy weapons are (damage, hit-number) pairs.

TWO FORMAT TRAPS, both handled below:
  * row labels differ per weapon - the photon's hit row is "STD/OLVD", the
    hellbore's is "TO HIT", not "STD".
  * the hellbore's to-hit numbers (11,10,9,8,7,6,5) are on a d20 scale, NOT the
    d6 scale every other weapon uses. Mixing them silently would make a hellbore
    look like a certainty. `heavy()` therefore records the scale, and callers
    that want a probability must use `hit_probability()` rather than assume /6.
"""
from __future__ import annotations

import os
import re

CHART_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), "client_data")
CHART_FILES = ("weapons.chart", "module_e2_weapons.chart", "module_e3_weapons.chart",
               "module_c6_weapons.chart", "module_y_weapons.chart")

# Weapons whose to-hit row is a d20 target number rather than a d6 "1-N" span.
D20_HIT_WEAPONS = {"Hellbore"}


def _brackets(header_cells):
    """['0','1','2','3-4','5-8'] -> [0,1,2,4,8]: the UPPER bound of each bracket.

    Upper bounds make lookup a simple "first bracket whose bound >= range",
    which is how every existing consumer already works.
    """
    out = []
    for c in header_cells:
        c = (c or "").strip()
        if not c:
            continue
        m = re.match(r"^(\d+)\s*-\s*(\d+)$", c)
        if m:
            out.append(int(m.group(2)))
        elif c.endswith("+") and c[:-1].isdigit():
            out.append(int(c[:-1]))
        elif c.isdigit():
            out.append(int(c))
    return out


def _hit_number(cell):
    """'1-5' -> 5 (hits on 1..5 of a d6). 'NA' -> 0 (cannot fire at all)."""
    cell = (cell or "").strip().upper()
    if not cell or cell == "NA":
        return 0
    m = re.match(r"^1\s*-\s*(\d+)$", cell)
    if m:
        return int(m.group(1))
    return int(cell) if cell.isdigit() else 0


def _num(cell):
    cell = (cell or "").strip().upper()
    if not cell or cell == "NA":
        return 0
    try:
        return float(cell)
    except ValueError:
        return 0


# Row labels that must never be mistaken for a section key.
ROW_LABELS = {"RANGE", "ROLL", "HIT", "HIT#", "TO HIT", "STD", "OVLD", "UIM",
              "DERFACS", "UIM-OVLD", "PROX", "BOLT", "NOTE", "REINF",
              "TARGET SIZE", "ADJUSTMENT", "DAMAGE", "STD/OLVD"}


def _is_title(cells):
    """Does this line START a new weapon section?

    Matching the word "CHART" alone is not enough: sibling sections are titled
    "...TABLE" ("Axion,AXION TORPEDO FIRING TABLE") or carry no keyword at all
    ("Carronade, PLASMA CARRONADE"). Missing them folded every later chart into
    the Photon section, whose RANGE row was then overwritten by an unrelated
    weapon's - so a photon at range 6 reported 9 damage on a roll of 1-5 instead
    of 8 on 1-3.

    A title is a short single-token key followed by prose. Data rows carry
    numeric / "1-5" / "NA" cells, and their labels sit in column 0.
    """
    if len(cells) < 2:
        return False
    key, second = cells[0], cells[1].strip()
    if not key or " " in key or key.upper() in ROW_LABELS:
        return False
    if not second or not re.search(r"[A-Za-z]", second):
        return False
    if re.match(r"^(NA|\d+(\s*-\s*\d+)?)$", second, re.I):
        return False
    # Prose in column 1 that is really a value, e.g. "+1 DMG PER 1 POWER > COST".
    if second.startswith(("+", "-")) or "PER" in second.upper():
        return False
    return True


def parse_charts(paths=None):
    """{weapon_key: {"ranges": [...], "rows": {LABEL: [cells]}}}"""
    charts, cur = {}, None
    for fn in (paths or CHART_FILES):
        p = os.path.join(CHART_DIR, fn)
        if not os.path.exists(p):
            continue
        with open(p, encoding="utf-8", errors="replace") as fh:
            for ln in fh:
                cells = [c.strip() for c in ln.rstrip("\n").split(",")]
                if not cells or not cells[0]:
                    continue
                head = cells[0]
                if _is_title(cells):
                    cur = head
                    charts[cur] = {"ranges": [], "rows": {}}
                    continue
                if cur is None:
                    continue
                lbl = head.upper()
                if lbl in ("RANGE", "ROLL"):
                    # First RANGE wins. A stray second one belongs to a section
                    # the splitter missed, and silently replacing another
                    # weapon's brackets is exactly the failure described above.
                    if not charts[cur]["ranges"]:
                        charts[cur]["ranges"] = _brackets(cells[1:])
                elif lbl not in charts[cur]["rows"]:
                    charts[cur]["rows"][lbl] = cells[1:]
    return charts


_CHARTS = None


def charts():
    global _CHARTS
    if _CHARTS is None:
        _CHARTS = parse_charts()
    return _CHARTS


def expected_phaser(key):
    """[(range_bound, expected_damage)] averaged over a fair d6.

    A phaser is rolled per shot, so the number the advice wants is the mean,
    not any single row of the chart.
    """
    ch = charts().get(key)
    if not ch or not ch["ranges"]:
        return []
    rolls = [ch["rows"].get(str(d)) for d in range(1, 7)]
    rolls = [r for r in rolls if r]
    if not rolls:
        return []
    out = []
    for i, bound in enumerate(ch["ranges"]):
        vals = [_num(r[i]) for r in rolls if i < len(r)]
        if vals:
            out.append((bound, round(sum(vals) / len(vals), 3)))
    return out


def _first_row(ch, names):
    for n in names:
        if n.upper() in ch["rows"]:
            return ch["rows"][n.upper()]
    return None


def heavy(key, dmg_rows=("DMG-STD",), hit_rows=("STD",)):
    """[(range_bound, damage, hit_number)] for a heavy weapon.

    Row labels vary per weapon, so both are tried in order.
    """
    ch = charts().get(key)
    if not ch or not ch["ranges"]:
        return []
    dmg = _first_row(ch, dmg_rows)
    hit = _first_row(ch, hit_rows)
    out = []
    for i, bound in enumerate(ch["ranges"]):
        d = _num(dmg[i]) if dmg and i < len(dmg) else 0
        h = _hit_number(hit[i]) if hit and i < len(hit) else 0
        out.append((bound, d, h))
    return out


def hit_probability(key, hit_number):
    """Turn a chart to-hit number into a probability on the RIGHT die.

    The hellbore's to-hit column is a d20 target number; everything else is a
    d6 "hits on 1-N" span. Treating 11 as a d6 span would report a >100% chance.
    """
    if not hit_number:
        return 0.0
    if key in D20_HIT_WEAPONS:
        return max(0.0, min(1.0, hit_number / 20.0))
    return max(0.0, min(1.0, hit_number / 6.0))


def plasma(kind="R"):
    """[(range_bound, warhead)] for a plasma torpedo type (R/M/S/G/L/F/D).

    The engine modelled plasma as linear decay formulas and every one was wrong:
    Plasma-R was "30 - 2*range" when it LAUNCHES AT 50, Plasma-F "20-2*range
    max 10" when it is 20, Plasma-G "10-range" when it is 20. Real decay is a
    14-bracket table, not a line.

    The chart's final column is SHOTGUN (a firing mode, not a range), so it is
    dropped here - see shotgun() for that value.
    """
    ch = charts().get("Plasma")
    if not ch or not ch["ranges"]:
        return []
    row = ch["rows"].get(kind.upper())
    if not row:
        return []
    return [(b, _num(row[i])) for i, b in enumerate(ch["ranges"]) if i < len(row)]


def shotgun(kind="R"):
    """The SHOTGUN column (F&E-style scatter mode), last cell of the plasma row."""
    ch = charts().get("Plasma")
    row = (ch or {}).get("rows", {}).get(kind.upper())
    if not ch or not row or len(row) <= len(ch["ranges"]):
        return 0
    return _num(row[len(ch["ranges"])])


def max_range(key):
    ch = charts().get(key)
    return ch["ranges"][-1] if ch and ch["ranges"] else 0


# ---------------------------------------------------------------- public tables
# Built once at import, straight from the client data. If a chart file is
# missing these come back EMPTY and callers fall back to their own constants -
# the engine degrades visibly rather than silently reporting zero damage.
PH1_EXPECTED = expected_phaser("Phaser1")
PH2_EXPECTED = expected_phaser("Phaser2")
PH3_EXPECTED = expected_phaser("Phaser3")
PH4_EXPECTED = expected_phaser("Phaser4")

DISR_STD = heavy("Disr", ("DMG-STD",), ("STD",))
DISR_OVL = heavy("Disr", ("DMG-OVLD",), ("OVLD",))
PHOT_STD = heavy("Photon", ("DMG-STD",), ("STD/OLVD", "STD"))
PHOT_PROX = heavy("Photon", ("DMG-PROX",), ("PROX",))
HELLBORE = heavy("Hellbore", ("DMG-STD",), ("TO HIT", "STD"))
HELLBORE_OVL = heavy("Hellbore", ("DMG-OVLD",), ("TO HIT", "STD"))
FUSION = heavy("Fusion", ("DMG-STD",), ("STD",))
FUSION_OVL = heavy("FusionOL", ("DMG-STD",), ("STD",))

# Plasma warheads by type. R launches at 50, not the 30 the engine assumed.
PLASMA = {k: plasma(k) for k in ("R", "M", "S", "G", "L", "F", "D")}

PHASER_TABLE = {"phaser-1": PH1_EXPECTED, "phaser-2": PH2_EXPECTED,
                "phaser-3": PH3_EXPECTED, "phaser-4": PH4_EXPECTED,
                "phaser-G": PH3_EXPECTED,   # E2.152: ph-G fires on the ph-3 table
                "phaser": PH1_EXPECTED}     # bare "phaser" defaults to Ph-1


def lookup(table, rng):
    """First bracket whose upper bound covers `rng`; None beyond the last."""
    if not table:
        return None
    for row in table:
        if rng <= row[0]:
            return row
    return None
