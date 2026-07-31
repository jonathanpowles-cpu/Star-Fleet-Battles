"""
Seeking-weapon threat model: drones and plasma treated as the different weapons
they are, with real flight time and (where known) endurance.

This backs three audit items:
  S3  incoming_seekers must branch on KIND. A drone is a fixed warhead that
      needs killing; a plasma torpedo is a warhead that DECAYS with distance
      flown and is REDUCED (not killed) by phaser fire. Treating them alike gave
      plasma a flat strength and described phasers as "killing" it.
  S4  a seeker has finite endurance. An expired drone is off the board (FD1.7)
      and must stop counting as a live threat or blocking disengagement (C7.22).
      Flight time is computed EXACTLY from the launch impulse the client records;
      expiry is only asserted when the endurance is actually known, so an unknown
      endurance never silently removes a threat.
  SC2 disengagement-by-distance is blocked by seekers that can still reach you -
      but not by ones about to run dry.

PROVENANCE
  Drone speeds and warhead/kill values: client alphadrones.expendable
    (speed 8 / S=12 / M=20 / F=32; Type-I..V warhead sizes).
  Plasma launch strengths: client alphaplasma.expendable
    (Plasma-R=50, M=40, S=30, G=25, and the light/sabot variants).
  Plasma decay: client weapons.chart PLASMA TORPEDO TABLE gives three reduction
    bands but the extract does not make the indexing unambiguous, so the decay
    is applied as a clearly-labelled PROJECTION and flagged - never presented as
    the exact warhead. No ship in the current battle fields plasma; this is for
    completeness, not the live fight.
  Endurance-in-turns is NOT cleanly present in the extracts. Only Type-IIIXX
    (100 turns, FD2.x) is attested. Everything else is left UNKNOWN, and unknown
    endurance means "still live", never "assume expired".
"""
from __future__ import annotations

IMPULSES_PER_TURN = 32

# Plasma launch strength by type (client alphaplasma.expendable, col 4).
PLASMA_LAUNCH = {
    "R": 50, "M": 40, "S": 30, "SS": 48, "SL": 16,
    "G": 25, "GL": 10, "F": 20, "D": 12,
}

# weapons.chart PLASMA TORPEDO TABLE - reduction from launch strength. The band
# structure is recorded as printed; its indexing (distance flown vs range to
# target) is NOT certain from the extract, so callers must treat the result as a
# projection and say so. Kept here so the moment a plasma game settles the
# indexing, only this constant changes.
PLASMA_REDUCTION_BANDS = [((1, 2), 35), ((3, 4), 25), ((5, 6), 10)]

# Endurance in turns, keyed by an explicit type marker. Deliberately sparse:
# only values actually attested in the rules go here. Absence => unknown =>
# never treated as expired.
DRONE_ENDURANCE_TURNS = {
    "IIIXX": 100,          # FD2.x: "Type-III drones become IIIXX with endurance 100"
}


def _launch_ti(seeker):
    """(turn, impulse) a seeker was launched, from the client's 'launched' stamp
    (e.g. '1.17'), or None."""
    s = str(seeker.get("launched") or "").strip()
    if "." in s:
        a, b = s.split(".", 1)
        if a.isdigit() and b.isdigit():
            return int(a), int(b)
    return None


def flight_impulses(seeker, turn, impulse):
    """How many impulses this seeker has been in flight, or None if unknown.

    Exact: the client records the launch impulse, so this needs no assumption.
    """
    ti = _launch_ti(seeker)
    if ti is None:
        return None
    return (turn - ti[0]) * IMPULSES_PER_TURN + (impulse - ti[1])


def endurance_turns(seeker):
    """Endurance in turns if known, else None. Never guesses."""
    lo = (seeker.get("loadout") or "") + " " + (seeker.get("name") or "")
    for marker, turns in DRONE_ENDURANCE_TURNS.items():
        if marker in lo:
            return turns
    return None


def expiry(seeker, turn, impulse):
    """(expired: bool|None, turns_left: float|None, note).

    expired is None when endurance is unknown - the caller must then keep
    treating the seeker as live. FD1.4: a drone reaches end of endurance at the
    SAME impulse of a later turn, so expiry is whole turns from launch.
    """
    ti = _launch_ti(seeker)
    end = endurance_turns(seeker)
    if ti is None or end is None:
        return None, None, ("endurance unknown - treated as still live "
                            "(never assume a seeker has expired)")
    expire_turn = ti[0] + end
    turns_left = expire_turn - turn - (1 if impulse > ti[1] else 0)
    if turns_left <= 0:
        return True, turns_left, (f"FD1.7: past its {end}-turn endurance "
                                  f"(launched T{ti[0]}.{ti[1]}) - off the board")
    return False, turns_left, (f"~{turns_left} turn(s) of endurance left "
                               f"({end}-turn drone launched T{ti[0]}.{ti[1]})")


def is_plasma(seeker):
    k = (seeker.get("kind") or "").lower()
    return "plasma" in k or (seeker.get("name") or "").lower().startswith("plasma")


def _plasma_type(seeker):
    import re
    for field in (seeker.get("kind"), seeker.get("name"), seeker.get("loadout")):
        m = re.search(r"plasma[- ]?([RMSGFD]{1,2})", str(field or ""), re.I)
        if m:
            return m.group(1).upper()
    return None


def plasma_strength(seeker, hexes_flown):
    """(strength, note) for a plasma torpedo - a PROJECTION, flagged as such.

    Launch strength is solid (client data); the decay band is applied but
    labelled uncertain, because the extract does not pin down whether the band
    indexes distance flown or range to target.
    """
    t = _plasma_type(seeker)
    base = PLASMA_LAUNCH.get(t)
    if base is None:
        return None, "unknown plasma type - strength not projected"
    reduction = 0
    for (lo, hi), red in PLASMA_REDUCTION_BANDS:
        if lo <= hexes_flown <= hi:
            reduction = red
            break
    else:
        if hexes_flown > PLASMA_REDUCTION_BANDS[-1][0][1]:
            reduction = PLASMA_REDUCTION_BANDS[-1][1]
    strength = max(0, base - reduction)
    return strength, (f"Plasma-{t}: launches at {base} (client data); "
                      f"PROJECTED ~{strength} after ~{hexes_flown} hexes "
                      f"(weapons.chart decay band, indexing UNVERIFIED - confirm "
                      f"against the client before trusting the number)")


def describe(seeker, turn, impulse, my_pos=None):
    """A per-seeker threat line branching on kind, for incoming_seekers to attach."""
    flown = flight_impulses(seeker, turn, impulse)
    exp, left, exp_note = expiry(seeker, turn, impulse)
    out = {"expired": exp, "turns_left": left, "endurance_note": exp_note,
           "flown_impulses": flown}
    if is_plasma(seeker):
        hexes = flown if flown is not None else 0     # ~1 hex/impulse at plasma speed
        strength, note = plasma_strength(seeker, hexes)
        out.update(seeker_class="plasma", projected_strength=strength,
                   strength_note=note,
                   defence="phasers REDUCE a plasma warhead (FP1.611: 2 phaser "
                           "damage = -1 warhead), they do not kill it; a wild "
                           "weasel distracts it (J3), and its own decay thins it "
                           "as it flies")
    else:
        out.update(seeker_class="drone",
                   defence="phasers/ADD KILL a drone outright (FD1.51: unpenalised "
                           "by ECM); engage inside 9 hexes before it gains ECM "
                           "(E1.7)")
    return out
