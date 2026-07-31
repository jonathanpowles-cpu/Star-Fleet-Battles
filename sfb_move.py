"""
Impulse-by-impulse movement director.

Turns a tactical intent ("close to band 5-8", "hold station on the carrier")
into the one thing you can actually type into the client this impulse:

    STRAIGHT  |  SIDESLIP LEFT/RIGHT  |  TURN LEFT/RIGHT to <facing>  |  HET

Facing convention matches sfb_hex: 0=N, 1=NE, 2=SE, 3=S, 4=SW, 5=NW, increasing
CLOCKWISE. So facing+1 is a turn to the RIGHT, facing-1 a turn to the LEFT.

The two constraints that decide whether you may do what you want:
  C3.31  Turn Mode  - hexes of straight movement required between 60-degree turns
  C4.1   Slip Mode  - 1 for all units at all speeds
  C4.32  a sideslip COUNTS as straight movement for turn mode
  C4.33  a turn RESETS the sideslip count
"""
from __future__ import annotations
import sfb_hex as H

FACING_NAME = {0: "N", 1: "NE", 2: "SE", 3: "S", 4: "SW", 5: "NW"}


def turn_delta(facing, desired):
    """(steps, direction) - how far to turn, and which way is shorter.
    direction is +1 for RIGHT (clockwise), -1 for LEFT."""
    d = (desired - facing) % 6
    if d == 0:
        return 0, 0
    if d <= 3:
        return d, +1              # right/clockwise
    return 6 - d, -1              # left/anticlockwise


def maneuver_state(ship, log):
    """(hexes since last turn, hexes since last sideslip) from the combat log."""
    d = ((log or {}).get("maneuver") or {}).get(ship["label"])
    if not d:
        return None, None         # no history yet - treat as unconstrained
    return d.get("since_turn"), d.get("since_slip")


def move_order(ship, target_xy, log, turn_mode, moves_now, want_speed=None,
               oblique=True):
    """The explicit order for THIS impulse.

    target_xy is where we are trying to get to (an enemy, a station-keeping hex).
    Returns (headline, [reasons]).
    """
    pos = (ship["x"], ship["y"])
    facing = ship.get("facing", 0)
    if not moves_now:
        return ("HOLD - no movement this impulse",
                [f"speed {ship.get('speed', 0)} does not move on this impulse (C1.4 chart)"])

    direct = H.absolute_bearing(pos, target_xy)
    # THE OBLIQUE APPROACH (pp41-42). Pointing straight at him puts him on our
    # #1 shield, and doctrine is explicit that taking the opening salvo on #1 is
    # never a good idea (p16, p113). Aim one hexside off instead: the forward arc
    # still bears, but he sits on our #2 or #6.
    if oblique:
        cur_off = (direct - facing) % 6
        # pick whichever oblique heading is the smaller turn from where we are
        left_opt, right_opt = (direct - 1) % 6, (direct + 1) % 6
        lsteps, _ = turn_delta(facing, left_opt)
        rsteps, _ = turn_delta(facing, right_opt)
        desired = left_opt if lsteps <= rsteps else right_opt
    else:
        desired = direct
    steps, direction = turn_delta(facing, desired)
    since_turn, since_slip = maneuver_state(ship, log)
    may_turn = since_turn is None or since_turn >= turn_mode
    may_slip = since_slip is None or since_slip >= 1
    side = "RIGHT" if direction > 0 else "LEFT"
    reasons = []

    if steps == 0:
        # Holding the oblique is only right while it CLOSES. The old test ended
        # here, so once the enemy sat one sextant off the bow the order was
        # STRAIGHT forever - and with both fleets angled off, the bearing
        # rotates with them and the ships sail past each other without a single
        # turn being ordered. Doctrine wants an oblique APPROACH (pp41-42), not
        # a parallel course: so check whether straight actually gains ground,
        # and turn in when it does not.
        ahead = H.forward_hex(pos, facing)
        d_now = H.hex_distance(pos, target_xy)
        d_next = H.hex_distance(ahead, target_xy)
        if d_next < d_now or not may_turn:
            note = (f"holding the oblique - he bears {FACING_NAME[direct]} but we run "
                    f"{FACING_NAME[facing]}, keeping him off our #1 (pp41-42)"
                    if oblique and desired != direct
                    else f"already heading {FACING_NAME[facing]}, straight at the target")
            if d_next >= d_now and not may_turn:
                note += (f"; WARN this heading is no longer closing (range {d_now} -> "
                         f"{d_next}) but the turn mode is not yet satisfied")
            return ("STRAIGHT", [note])
        in_steps, in_dir = turn_delta(facing, direct)
        in_side = "RIGHT" if in_dir > 0 else "LEFT"
        nf = (facing + in_dir) % 6
        return (f"TURN {in_side} to {FACING_NAME[nf]}",
                [f"the oblique has stopped closing (straight ahead leaves range at "
                 f"{d_next} vs {d_now}) - he is slipping past our {in_side.lower()} side",
                 f"turn mode satisfied; pointing at his bearing {FACING_NAME[direct]} "
                 f"resumes the approach - the oblique can be re-established once "
                 f"the range is coming down again"])

    # A 180 is the one case worth considering an HET for; otherwise turn or slip.
    if steps == 3:
        reasons.append("target is directly astern - a 180 needs three turns, "
                       "two warp TACs + a sublight TAC (p45), or an HET")

    if may_turn:
        nf = (facing + direction) % 6
        head = f"TURN {side} to {FACING_NAME[nf]}"
        reasons.append(f"turn mode {turn_mode} satisfied"
                       + (f" ({since_turn} straight hexes since last turn)"
                          if since_turn is not None else " (no turn yet this battle)"))
        if steps > 1:
            reasons.append(f"{steps} turns needed in total to face {FACING_NAME[desired]}; "
                           f"this is the first - then {turn_mode} straight hexes before the next")
        return (head, reasons)

    # Cannot turn yet. A sideslip shifts us laterally without changing facing and
    # still counts toward the turn mode (C4.32), so it is the right filler.
    if may_slip:
        head = f"SIDESLIP {side}"
        reasons.append(f"cannot turn yet - need {turn_mode} straight hexes, have "
                       f"{since_turn if since_turn is not None else 0} (C3.31)")
        reasons.append("a sideslip closes the offset without changing facing, and still "
                       "counts as straight movement toward the turn mode (C4.32)")
        return (head, reasons)

    reasons.append(f"cannot turn (need {turn_mode} straight, have "
                   f"{since_turn if since_turn is not None else 0}) and cannot slip again yet "
                   f"(slip mode 1, C4.1)")
    return ("STRAIGHT", reasons)


def station_keeping_order(ship, consort, log, turn_mode, moves_now, want_range=2):
    """Escort variant: hold a position off the consort's flank/rear rather than
    charging the enemy."""
    if not consort:
        return None
    pos = (ship["x"], ship["y"])
    cpos = (consort["x"], consort["y"])
    d = H.hex_distance(pos, cpos)
    if d > want_range:
        head, reasons = move_order(ship, cpos, log, turn_mode, moves_now)
        return (head, [f"station: {d} hexes from {consort['label']}, want {want_range} - "
                       f"close up"] + reasons)
    if not moves_now:
        return ("HOLD - no movement this impulse",
                [f"on station ({d} hexes off {consort['label']})"])
    # On station: match his heading so the screen stays between him and the enemy.
    steps, direction = turn_delta(ship.get("facing", 0), consort.get("facing", 0))
    if steps == 0:
        return ("STRAIGHT", [f"on station {d} hexes off {consort['label']}, matching his heading"])
    since_turn, _ = maneuver_state(ship, log)
    if since_turn is None or since_turn >= turn_mode:
        nf = (ship.get("facing", 0) + direction) % 6
        return (f"TURN {'RIGHT' if direction > 0 else 'LEFT'} to {FACING_NAME[nf]}",
                [f"match {consort['label']}'s heading to hold the screen"])
    return ("STRAIGHT", [f"on station; cannot turn to match heading yet (turn mode {turn_mode})"])
