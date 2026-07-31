"""
Deck crew accounting - the real constraint on shuttle preparation.

Deck crews are not SSD boxes in the ordinary sense - their commitments are
paper-tracked - so what follows is a PROJECTION the player should confirm
against their own record sheet, and it is flagged as such rather than presented
as read state.

The save does carry a `deck_crews` field (surfaced as `deck_crews_reported`),
but it does NOT appear to be the complement: it reads 1 for a 12-fighter Kzinti
CV and 1 for a 2-fighter CLE, where the rules give those very different numbers.
It is more likely a count of crews currently committed to a task. It is emitted
for comparison and deliberately NOT used as the complement until that is settled.

Why it matters: "load a scatter-pack" is not an action you take this impulse.
The rulebook's own worked example (FD7.26) is precisely this case -

    "A Kzinti CL has decided to prepare a scatter-pack... The ship has two
     shuttles and, per (J4.814), has two deck crews... Each drone space moved
     from storage to the shuttle bay and installed on the SP requires one deck
     crew action (J4.821). So it will require SIX DECK CREW ACTIONS OVER THREE
     TURNS to move a full load of six drone spaces to the shuttle bay and load
     them on the shuttle."

So a scatter-pack is a three-turn commitment. Advising "load one" as though it
were an impulse action is wrong, and would have the player expecting a weapon
that cannot exist for three turns.
"""
from __future__ import annotations

ACTION_IMPULSES = 32          # J4.817: one deck crew action = 32 consecutive impulses
HALF_ACTION_IMPULSES = 16     # J4.8171: half action (type-VI, ADD round)
MAX_CREWS_PER_SHUTTLE = 2     # J4.8172, applied to SPs via FD7.214
DEFAULT_CREWS = 2             # J4.814: ships not listed in Annex #7G have two
CARRIER_FIGHTER_THRESHOLD = 8 # G33.42: 8+ fighters makes a TRUE carrier
SP_SPACES_ADMIN = 6           # FD7.21
SP_CONVERT_BACK_IMPULSES = 32  # FD7.215: converting an SP back to a shuttle

# J4.821 - deck crew actions per item loaded
ACTIONS_PER_SPACE = {
    "1-space drone": 1.0,
    "2-space drone (type-IV)": 2.0,
    "half-space (type-VI, ADD)": 0.5,
}


# Deck-crew complements from ANNEX #7G (Module G3), column "DC", keyed by
# (race, hull type). Populated from annex7g_deck_crews.json - extracted GEOMETRICALLY from the
# Annex #7G pages (tools/pdf_columns.py: per-row gap splitting, whole-page row
# zipping, column-major race carry), NOT from the flat text extract that broke
# the shield chart. Every entry passed the J4.81 self-check: DC equals fighters
# plus twice the heavy fighters, within the small admin/MRS allowance. Gorn and
# Vudar failed to extract - for those, absence here does NOT mean two crews.
ANNEX_7G_DECK_CREWS = {}
try:
    import json as _json, os as _os
    _p = _os.path.join(_os.path.dirname(_os.path.abspath(__file__)),
                       "annex7g_deck_crews.json")
    for _race, _hulls in _json.load(open(_p)).get("crews", {}).items():
        for _hull, _dc in _hulls.items():
            ANNEX_7G_DECK_CREWS[(_race, _hull.upper())] = int(_dc)
except Exception:
    pass


def deck_crews(ship):
    """(count, basis) - how many deck crews this ship has, and why.

    The rule is a DEFAULT WITH AN ANNEX OVERRIDE, not a formula:

      J4.814  every ship has TWO deck crews unless formally assigned a number by
              Annex #7G. Two is the working figure for anything that is not a
              carrier - it is the case behind the FD7.26 scatter-pack example,
              where two crews take three turns to load six drone spaces.
      J4.81   carriers are assigned their complement by Annex #7G, scaled to the
              fighter group (broadly one per fighter, two per HEAVY fighter),
              and the number is printed on the carrier's SSD.
      J4.814  carrier ESCORTS (ready racks but no fighters of their own) and
              casual carriers get one deck crew per ready rack.

    An earlier version of this read J4.81's "equal to the number of fighters
    carried" as the operative rule and returned one crew per fighter BOX for any
    ship with fighters. That over-reads a general statement whose own text ends
    "(Some ships are designated to have a different number.)" - the Annex is what
    actually assigns it. Where the Annex value is unknown this now says so
    instead of substituting a derived guess.

    J4.811: crews die with the shuttle/fighter box they are working in, and the
    count is re-determined at the start of each turn - so a carrier's complement
    degrades as its bays are shot away, which the `lost` term reflects.
    """
    sysd = ship.get("systems") or {}
    ftr, ftr_max = (sysd.get("fighter") or [0, 0])[:2]
    race = (ship.get("race") or "").strip()
    htype = (ship.get("type") or "").strip().upper()

    if ftr_max:
        annex = ANNEX_7G_DECK_CREWS.get((race, htype))
        # A handful of fighters makes a CASUAL carrier, not a true one, and
        # J4.814 covers those explicitly by the ready-rack rule rather than the
        # Annex. G33.42 sets the threshold at 8 fighters. Without this a
        # two-fighter escort was being reported as an unknown Annex carrier.
        if annex is None and ftr_max < CARRIER_FIGHTER_THRESHOLD:
            racks = (sysd.get("ready_rack") or [0, 0])[0]
            n = max(DEFAULT_CREWS, racks)
            return n, (f"J4.814: {ftr_max} fighter(s) makes this a CASUAL carrier "
                       f"(G33.42: {CARRIER_FIGHTER_THRESHOLD}+ is a true carrier), "
                       f"so one deck crew per ready rack"
                       + (f" ({racks})" if racks else " - no ready-rack count in "
                          "the save") + f", minimum {DEFAULT_CREWS}")
        if annex is None:
            # Do not guess. Report the default and flag that the real number is
            # an Annex lookup, so the advice is visibly provisional rather than
            # confidently wrong.
            return DEFAULT_CREWS, (
                f"UNKNOWN - {race} {htype} is a carrier, so its deck-crew "
                f"complement is assigned by ANNEX #7G (Module G3) and printed on "
                f"its SSD; that value is not yet recorded here. Falling back to "
                f"the J4.814 default of {DEFAULT_CREWS}, which is almost "
                f"certainly TOO LOW for a {ftr_max}-fighter group - treat every "
                f"timing below as a lower bound on capability")
        lost = max(0, ftr_max - ftr)
        avail = max(0, annex - lost)
        basis = f"Annex #7G: {race} {htype} is assigned {annex} deck crews"
        if lost:
            basis += (f"; J4.811: {lost} fighter box(es) destroyed kills {lost} "
                      f"crew(s), leaving {avail}")
        return avail, basis

    racks = (sysd.get("ready_rack") or [0, 0])[0]
    if racks:
        n = max(DEFAULT_CREWS, racks)
        return n, (f"J4.814: a carrier escort has one deck crew per ready rack "
                   f"({racks}), minimum {DEFAULT_CREWS}")
    return DEFAULT_CREWS, "J4.814: ships not listed in Annex #7G have two deck crews"


def scatterpack_eta(ship, spaces=SP_SPACES_ADMIN):
    """Turns to load a scatter-pack, and whether it is worth starting.

    Returns (turns, crews_used, detail). Even a 12-crew carrier cannot beat the
    two-crews-per-shuttle cap (J4.8172), so the floor is spaces/2 turns.
    """
    crews, basis = deck_crews(ship)
    working = min(crews, MAX_CREWS_PER_SHUTTLE)
    actions = spaces * ACTIONS_PER_SPACE["1-space drone"]
    turns = int(-(-actions // working))          # ceil
    return turns, working, basis


def parallel_scatterpacks(ship, shuttle_boxes):
    """How many SPs a ship can prepare AT ONCE, given crews and shuttle boxes.

    A carrier's many deck crews cannot make one shuttle load faster (J4.8172),
    but they can load several shuttles simultaneously - which is the actual way
    a drone race generates saturation.
    """
    crews, _ = deck_crews(ship)
    by_crew = crews // MAX_CREWS_PER_SHUTTLE
    return max(0, min(by_crew, shuttle_boxes))


def scatterpack_advice(ship, shuttle_boxes, rng, turn, log=None):
    """Is loading a scatter-pack actually an available action right now?"""
    if not shuttle_boxes:
        return None
    turns, working, basis = scatterpack_eta(ship)
    crews, _ = deck_crews(ship)
    n_parallel = parallel_scatterpacks(ship, shuttle_boxes)

    why = [
        f"{crews} deck crew(s) - {basis}",
        f"FD7.22: drones must be loaded onto an SP by deck crews at normal rates; "
        f"FD7.26 worked example: {SP_SPACES_ADMIN} spaces = {SP_SPACES_ADMIN} crew "
        f"actions = {turns} TURNS with {working} crews on the job",
        f"J4.8172 caps it at {MAX_CREWS_PER_SHUTTLE} crews per shuttle, so extra crews "
        f"cannot make one SP load faster",
    ]
    if n_parallel > 1:
        why.append(f"but {crews} crews CAN load {n_parallel} scatter-packs simultaneously "
                   f"({shuttle_boxes} shuttle box(es) available) - that is how a drone "
                   f"race generates saturation")
    why.append("J4.8174: any interruption (launching, crew transfer, box destroyed) "
               "CANCELS the action outright and the work is lost")
    why.append("NOT tracked in the save file - deck crews are paper-recorded, so confirm "
               "against your own sheet")

    head = (f"START LOADING {n_parallel} SCATTER-PACK(S) - ready in {turns} turns "
            f"(turn {turn + turns})" if n_parallel else
            "NO scatter-pack - no free shuttle box")
    return head, why


def crew_load_summary(ship, shuttle_boxes, fighters_out=0):
    """What the deck crews are committed to, so competing demands are visible."""
    crews, basis = deck_crews(ship)
    lines = [f"DECK CREWS: {crews} ({basis})"]
    if fighters_out:
        lines.append(f"{fighters_out} fighter(s) airborne - each returning fighter needs "
                     f"~1 full turn of crew time to rearm (J4.8172: no more than "
                     f"{MAX_CREWS_PER_SHUTTLE} crews per fighter, and one crew cannot work "
                     f"on two)")
    if shuttle_boxes:
        lines.append(f"{shuttle_boxes} shuttle box(es) free for SP / wild weasel / suicide "
                     f"shuttle work - all three compete for the SAME crews and boxes")
    return lines
