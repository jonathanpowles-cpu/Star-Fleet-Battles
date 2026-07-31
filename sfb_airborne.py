"""
What is actually flying, read from the save rather than tallied from the log.

The client keeps every launched fighter, wild weasel, suicide shuttle and
scatter-pack as a real on-board piece, so the save answers "what is up right
now" directly. StateDump exposes them as state["shuttles"], each carrying:

    launched_by   the launching ship's ID  - exact attribution
    mission       "Manned" for a fighter; anything else is a shuttle mission
    shuttle_type  e.g. AAS, BMR            - the airframe
    max_speed     that airframe's speed    - per craft, not per race
    damage_taken, bay, launched

This replaces launch-minus-landing bookkeeping. That approach was wrong twice
over: launches were never parsed at all (so counts never moved), and even once
parsed it can only ever count DOWN - a recovered fighter, or one shot down,
never comes back. Reading the board is self-correcting and needs no landing
event at all.

J1.61 recovery speed keys off the SLOWEST airframe actually airborne, not a
per-race constant: a group of speed-8 AAS with one speed-6 BMR in it recovers
at 6.
"""
from __future__ import annotations

# The only mission value observed in a live save is "Manned". The full
# vocabulary is not recoverable from the client jar, so rather than guess at
# names for the shuttle missions, treat "Manned" as the known fighter value and
# report anything else as a non-fighter - and record it, so the vocabulary can
# be learned from real games instead of invented.
FIGHTER_MISSION = "manned"
_seen_missions = set()


def airborne(state, ship=None):
    """Craft currently on the board, optionally only those from one ship."""
    craft = (state or {}).get("shuttles") or []
    if ship is None:
        return list(craft)
    sid = ship.get("id")
    if not sid:
        return []
    return [c for c in craft if c.get("launched_by") == sid]


def is_fighter(craft):
    m = (craft.get("mission") or "").strip().lower()
    if m and m != FIGHTER_MISSION:
        _seen_missions.add(craft.get("mission"))
    return m == FIGHTER_MISSION


def unknown_missions():
    """Mission strings seen that are not the known fighter value - so a real
    game teaches us the vocabulary rather than us inventing it."""
    return sorted(_seen_missions)


def fighters_out(state, ship):
    """Fighters from this ship currently airborne."""
    return [c for c in airborne(state, ship) if is_fighter(c)]


def shuttles_out(state, ship):
    """Non-fighter craft (weasel / suicide / scatter-pack) from this ship.

    These occupy shuttle boxes and deck crews but must NOT be debited from the
    fighter complement - doing so made a 2-fighter escort report its fighters
    gone the moment it launched a wild weasel.
    """
    return [c for c in airborne(state, ship) if not is_fighter(c)]


def fighters_aboard(state, ship):
    """(remaining, capacity) fighters still in the bay.

    Capacity is undestroyed fighter boxes (J4.811 kills the crew with the box);
    remaining subtracts only what is actually flying.
    """
    cap = ((ship.get("systems") or {}).get("fighter") or [0, 0])[0]
    return max(0, cap - len(fighters_out(state, ship))), cap


def recovery_speed(state, ship, default=8):
    """J1.61 speed limit while a group is out: the SLOWEST airframe airborne.

    Returns (speed, note) or (None, "") when nothing is out. Reading the actual
    airframes beats a per-race constant - a mixed group recovers at the speed of
    its slowest member, which no race-level table can express.
    """
    out = fighters_out(state, ship)
    if not out:
        return None, ""
    speeds = [c.get("max_speed") or default for c in out]
    slow = min(speeds)
    types = sorted({c.get("shuttle_type") or "?" for c in out})
    mixed = ""
    if len(set(speeds)) > 1:
        slowest = sorted({c.get("shuttle_type") or "?" for c in out
                          if (c.get("max_speed") or default) == slow})
        mixed = (f" The group is MIXED ({'/'.join(types)}); the "
                 f"{'/'.join(slowest)} at speed {slow} sets the limit for all of them.")
    return slow, (f"J1.61: fighters cannot land while the ship moves faster than they do. "
                  f"With {len(out)} craft out, speed must not exceed {slow} or they are "
                  f"stranded - not just trailing, but unable to recover at all.{mixed}")
