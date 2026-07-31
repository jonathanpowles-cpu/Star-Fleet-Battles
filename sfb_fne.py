"""
Federation & Empire (F&E / SFB Campaign) reader.

Two data paths, mirroring the SFB tooling:

  * LIVE (clean): F&E runs through the same relay; fleets arrive as normal
    objects (addobj/setattr with base64(gzip(JSON)) values) and are already
    decoded into sfb_client.Piece. interpret_fleet() turns a Piece into a
    structured Fleet (name, faction, hex, kind).

  * SAVE (offline fallback): the campaign auto-save is base64 + Java
    serialization. javaobj reads the board metadata; the custom game-piece
    classes it can't fully decode, so fleets + the campaign event log are pulled
    by targeted extraction from the serialization stream.

F&E object model (confirmed from a live campaign save):
    gub.plugins.game.fne.gp.FleetGamePiece with attributes:
      Name          e.g. "Lyran Fleet"
      counter_file  e.g. "Lyran/Fleet.gif"   -> faction = "Lyran"
      designator    e.g. "Fleet"
      unit_type, num_of_pdus, movement_turns, boardID, ...
    Ship classes appear as "<Faction> - <Hull>" (e.g. "Kzinti - CVL", "- FRD").
    The campaign log is dated narrative: "168.2.32 0801 Kzinti Count's Fleet ..."
"""

from __future__ import annotations

import re
import base64
from dataclasses import dataclass, field
from typing import Optional


# ==========================================================================
# LIVE path — interpret a decoded online Piece as an F&E fleet
# ==========================================================================

@dataclass
class Fleet:
    obj_id: str
    name: str
    faction: str
    designator: str          # "Fleet", "BATS", ship hull, ...
    hex: Optional[str]       # "0801" style, or None if off-board
    xy: Optional[tuple]
    unit_type: str = ""
    ship_class: str = ""     # e.g. "Lyran - CA+"
    hull: str = ""           # e.g. "CA+", "DN", "BATS"
    condition: Optional[int] = None   # 100 = undamaged
    epv: Optional[int] = None         # economic point value (strategic worth)
    crew: str = ""
    ships: list = field(default_factory=list)

    def __repr__(self):
        loc = self.hex or "off-board"
        return f"Fleet({self.name} [{self.faction}] @{loc} {self.hull} cond={self.condition})"


def _faction_from_counter(counter_file: str) -> str:
    # "Lyran/Fleet.gif" -> "Lyran"
    if counter_file and "/" in counter_file:
        return counter_file.split("/", 1)[0]
    return ""


def interpret_fleet(piece) -> Optional[Fleet]:
    """Turn a decoded sfb_client.Piece (an F&E object) into a Fleet, or None if
    it doesn't look like a fleet/unit piece."""
    a = piece.attrs
    counter = a.get("counter_file", "") or ""
    name = a.get("Name") or a.get("Label") or piece.obj_id
    faction = _faction_from_counter(counter) or (a.get("race") or "")
    designator = a.get("designator", "") or a.get("unit_type", "")
    if not (counter or a.get("designator")):
        return None
    # location: campaign units store position in the savedMoves trail (last
    # entry = current hex); fall back to boardLocation / full_location.
    xy = piece.xy
    sm = a.get("savedMoves")
    if isinstance(sm, dict) and sm.get("queue"):
        last = sm["queue"][-1]
        loc = last.get("loc") if isinstance(last, dict) else None
        if isinstance(loc, dict) and "x" in loc:
            xy = (loc["x"], loc["y"])
    hexlbl = None
    fl = a.get("full_location") or a.get("hex")
    if isinstance(fl, str) and fl.strip():
        hexlbl = fl.strip()
    elif xy is not None:
        hexlbl = f"{xy[0]:02d}{xy[1]:02d}"
    ship_class = str(a.get("Name", "") or "")
    hull = ship_class.split("-", 1)[-1].strip() if "-" in ship_class else designator
    def _int(v):
        return v if isinstance(v, int) else None
    return Fleet(
        obj_id=piece.obj_id, name=str(name), faction=faction,
        designator=str(designator), hex=hexlbl, xy=xy,
        unit_type=a.get("unit_type", "") or "",
        ship_class=ship_class, hull=hull,
        condition=_int(a.get("condition")), epv=_int(a.get("epv")),
        crew=str(a.get("crew", "") or ""),
    )


def read_live_fleets(game_state) -> list:
    """All fleets currently in a sfb_client.GameState (F&E game via the relay)."""
    out = []
    for p in game_state.snapshot():
        f = interpret_fleet(p)
        if f:
            out.append(f)
    return out


# ==========================================================================
# SAVE path — parse the Java-serialized campaign auto-save
# ==========================================================================

_LOG_RE = re.compile(r"\d{3}\.\d+\.\d+\s+\d{4}\b.*")  # "168.2.32 0801 ..."
_SHIPCLASS_RE = re.compile(r"^([A-Z][a-zA-Z]+)\s*-\s*([A-Z][A-Za-z0-9+]{1,5})$")


@dataclass
class CampaignSave:
    board_class: str = ""
    board_meta: dict = field(default_factory=dict)
    fleets: list = field(default_factory=list)         # fleet names found
    ship_classes: list = field(default_factory=list)   # ("Faction","Hull")
    log: list = field(default_factory=list)            # campaign event lines
    factions: list = field(default_factory=list)


def _java_strings(raw: bytes) -> list:
    return [s.decode("latin1") for s in re.findall(rb"[\x20-\x7e]{3,}", raw)]


def parse_fne_save(path: str) -> CampaignSave:
    data = open(path, "rb").read()
    try:
        raw = base64.b64decode(data)
    except Exception:
        raw = data
    out = CampaignSave()

    # board metadata via javaobj (works even when pieces don't)
    try:
        import javaobj
        obj = javaobj.loads(raw)
        cd = getattr(obj, "classdesc", None)
        out.board_class = getattr(cd, "name", "") if cd else ""
        for k in ("iID", "lID", "cellSize", "lTerrainId", "showMapLabels",
                  "isOffBoardListShown"):
            if hasattr(obj, k):
                out.board_meta[k] = getattr(obj, k)
    except Exception as e:
        out.board_meta["_javaobj_error"] = str(e)

    strings = _java_strings(raw)

    # fleets: a Name string immediately following a FleetGamePiece marker region
    # (heuristic: collect the value after each "Name" key that reads like a fleet)
    for i, s in enumerate(strings):
        if s == "Name" and i + 1 < len(strings):
            v = strings[i + 1]
            if v and v[0].isupper() and ("Fleet" in v or "Squadron" in v or v.endswith("t")):
                out.fleets.append(v.rstrip("t"))
    out.fleets = sorted(set(out.fleets))

    # ship classes "<Faction> - <Hull>"
    facs = set()
    for s in strings:
        s2 = s.rstrip("t")
        m = _SHIPCLASS_RE.match(s2)
        if m:
            out.ship_classes.append((m.group(1), m.group(2)))
            facs.add(m.group(1))
    out.ship_classes = sorted(set(out.ship_classes))
    out.factions = sorted(facs)

    # campaign event log (dated narrative)
    for s in strings:
        s2 = s.rstrip("t")
        if _LOG_RE.match(s2):
            out.log.append(s2)

    return out


# ==========================================================================
# Situation report
# ==========================================================================

# strategic-value classes (for "capital ship" flags)
_CAPITAL = ("DN", "BB", "BC", "CC", "CVA", "CV", "SB", "BATS", "BS")
_BASE = ("BATS", "BS", "SB", "MB", "FRD", "BATS")


def _region(xy):
    """Coarse map region key for concentration grouping (3-hex blocks)."""
    if not xy:
        return "off-board"
    return f"{xy[0]//3*3:02d}{xy[1]//3*3:02d}"


def situation_report(fleets: list, log_events: list = None) -> str:
    from collections import Counter, defaultdict
    import sfb_hex as H
    log_events = log_events or []
    on_board = [f for f in fleets if f.xy]
    factions = sorted({f.faction for f in fleets if f.faction})
    out = []
    out.append("=" * 66)
    out.append("SFB CAMPAIGN — SITUATION REPORT")
    out.append("=" * 66)

    # ---- order of battle ----
    out.append("\nORDER OF BATTLE")
    for fac in factions:
        ff = [f for f in fleets if f.faction == fac]
        epv = sum(f.epv or 0 for f in ff)
        hulls = Counter(f.hull for f in ff)
        caps = [f for f in ff if any(f.hull.startswith(c) for c in _CAPITAL)]
        out.append(f"  {fac}: {len(ff)} units, total EPV {epv}, "
                   f"{len(caps)} capital-class")
        top = ", ".join(f"{h}×{n}" for h, n in hulls.most_common(8))
        out.append(f"     hulls: {top}")

    # ---- fleet condition ----
    out.append("\nFLEET CONDITION")
    for fac in factions:
        ff = [f for f in fleets if f.faction == fac and f.condition is not None]
        if not ff:
            continue
        avg = sum(f.condition for f in ff) / len(ff)
        hurt = sorted([f for f in ff if f.condition < 90], key=lambda f: f.condition)
        out.append(f"  {fac}: avg condition {avg:.0f}%, {len(hurt)} damaged")
        for f in hurt[:6]:
            out.append(f"     {f.condition:3}%  {f.name}  @{f.hex}")

    # ---- force concentrations ----
    out.append("\nFORCE CONCENTRATIONS (by hex)")
    byhex = defaultdict(list)
    for f in on_board:
        byhex[f.hex].append(f)
    for hx, grp in sorted(byhex.items(), key=lambda kv: -sum(g.epv or 0 for g in kv[1]))[:8]:
        facs = Counter(g.faction for g in grp)
        epv = sum(g.epv or 0 for g in grp)
        who = "/".join(f"{k}:{v}" for k, v in facs.items())
        out.append(f"  {hx}: {len(grp)} units (EPV {epv}) [{who}]")

    # ---- front / contested space ----
    out.append("\nFRONT ANALYSIS")
    if len(factions) >= 2:
        a_units = [f for f in on_board if f.faction == factions[0]]
        b_units = [f for f in on_board if f.faction == factions[1]]
        if a_units and b_units:
            closest = min(
                ((H.hex_distance(a.xy, b.xy), a, b) for a in a_units for b in b_units),
                key=lambda t: t[0])
            d, a, b = closest
            out.append(f"  nearest contact: {a.name} ({factions[0]}) @{a.hex} "
                       f"<-> {b.name} ({factions[1]}) @{b.hex}  = {d} hexes")
            # count each side's units within 3 hexes of any enemy = the front
            def front(units, enemy):
                return [u for u in units if any(H.hex_distance(u.xy, e.xy) <= 3 for e in enemy)]
            fa, fb = front(a_units, b_units), front(b_units, a_units)
            out.append(f"  forces engaged at the front (<=3 hexes): "
                       f"{factions[0]} {len(fa)} / {factions[1]} {len(fb)}")
    else:
        out.append("  only one faction on-board (no contested front)")

    # ---- strategic assets ----
    out.append("\nSTRATEGIC ASSETS")
    for f in on_board:
        if any(f.hull.startswith(b) for b in _BASE) or "FRD" in f.hull or "TG" in f.hull:
            out.append(f"  {f.faction:8} {f.name}  @{f.hex}")

    # ---- campaign log ----
    if log_events:
        out.append("\nRECENT CAMPAIGN EVENTS")
        for e in log_events[-8:]:
            out.append(f"  {e[:92]}")

    out.append("\n" + "=" * 66)
    return "\n".join(out)


def summarize(save: CampaignSave) -> str:
    lines = [f"F&E campaign save — board: {save.board_class or '?'}"]
    lines.append(f"  factions: {', '.join(save.factions) or '(none)'}")
    lines.append(f"  fleets:   {len(save.fleets)}  {save.fleets[:6]}")
    lines.append(f"  ship classes: {len(save.ship_classes)}")
    lines.append(f"  campaign-log events: {len(save.log)}")
    for ev in save.log[:5]:
        lines.append(f"     {ev[:90]}")
    return "\n".join(lines)


if __name__ == "__main__":
    import sys
    p = sys.argv[1] if len(sys.argv) > 1 else \
        r"C:/Users/jonat/AppData/Local/SFU Online Client/app/restore/game#FNE_Game1"
    print(summarize(parse_fne_save(p)))
