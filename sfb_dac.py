"""
The Damage Allocation Chart, decoded from the client and CONFIRMED IN PLAY.

Source: client_data/ship_dac.table. Structure:

    2                                   <- dice: a 2d6 allocation roll
    1,1, 2,1, 24,1, 4,1, ...            <- roll 2  : 13 (kind, flag) pairs
    ...
    25,1, 26,1, 3,1, ...                <- roll 12

  * 11 rows, one per 2d6 result (2..12); row index = roll - 2
  * 13 columns, walked LEFT TO RIGHT as damage is scored
  * "Excess Damage" (kind 13) is always the last column
  * flag 1 = BOLD. D4.31: "A given BOLD result can only be scored ONE time in
    each volley." Non-bold entries repeat indefinitely.

VALIDATION - this is a measurement, not a reading. Eight internals were applied
to an LDR DN in the client and every roll recorded:

    roll 7 -> Cargo              roll 2 -> Bridge
    roll 5 -> Right Warp Engine  roll 6 -> Forward Hull
    roll 5 -> Rear Hull  (x3)    roll 9 -> Left Warp Engine

Seven of eight matched this table exactly, including the bold rule: roll 5's
column A (Right Warp Engine, BOLD) was consumed once and the next three hits on
roll 5 all fell through to column B (Rear Hull, not bold). The save diff
reconciled as well - hull 24->20 for the four hull hits, warp 45->43 for the two
engine hits.

The eighth is the interesting one, and is why `_has` exists: row 7 column A is
ARMOR, the LDR DN has no armor boxes, and the client scored that hit on CARGO.
So an entry naming a system the ship lacks is NOT simply skipped to the next
column - the client substitutes. Only the armor->cargo case has been observed,
so this module SKIPS missing systems and says so, rather than inventing a
substitution table for the rest.

Note also: D4.31's printed example says a roll of 12 scores on "auxiliary
control, emergency bridge, and scanners", where this table's roll-12 row reads
Cargo, Shield, Scanner - only the third agrees. The printed example appears to
describe a different edition of the chart. The client is authoritative for what
the client does, and the measurement above backs it.
"""
from __future__ import annotations

import os

DATA_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), "client_data")
EXCESS_DAMAGE = 13

# Chart kind -> the key StateDump reports it under, where one exists. Kinds with
# no entry are systems the state dump does not break out; they count as present
# so the model never silently reroutes damage away from them.
KIND_TO_SYSTEM = {
    1: "bridge", 2: "flag_bridge", 3: "scanner", 4: "damage_control",
    5: "hull", 7: "transporter", 8: "tractor", 9: "shuttle", 10: "lab",
    11: "hull", 18: "battery", 21: "probe", 22: "sensor",
    23: "aux_control", 24: "emergency_bridge", 25: "cargo", 27: "armor",
    6: "warp", 12: "warp", 17: "warp", 16: "impulse", 20: "apr", 32: "awr",
    14: "drone", 15: "phaser", 28: "photon", 29: "disruptor",
    33: "phaser-1", 34: "phaser-2", 35: "phaser-3", 36: "phaser-4",
    49: "esg", 46: "hellbore", 47: "fusion", 48: "ppd",
}

_NAMES = None
_ROWS = None


def names():
    """kind -> human name, from the client's own boxtypes.names."""
    global _NAMES
    if _NAMES is None:
        _NAMES = {}
        try:
            with open(os.path.join(DATA_DIR, "boxtypes.names"),
                      encoding="utf-8", errors="replace") as fh:
                for ln in fh:
                    if "=" in ln:
                        n, v = ln.rsplit("=", 1)
                        try:
                            _NAMES[int(v.strip())] = n.strip()
                        except ValueError:
                            pass
        except OSError:
            pass
    return _NAMES


def rows(table="ship_dac.table"):
    """[[(kind, flag) x 13] x 11], indexed by roll - 2."""
    global _ROWS
    if _ROWS is None:
        _ROWS = []
        try:
            with open(os.path.join(DATA_DIR, table), encoding="utf-8",
                      errors="replace") as fh:
                lines = [l.strip() for l in fh if l.strip()]
            for ln in lines[1:]:              # line 0 is the dice count
                c = [int(x) for x in ln.split(",")]
                _ROWS.append([(c[i], c[i + 1]) for i in range(0, len(c) - 1, 2)])
        except (OSError, ValueError):
            _ROWS = []
    return _ROWS


def roll_distribution():
    """{roll: probability} for 2d6."""
    d = {}
    for a in range(1, 7):
        for b in range(1, 7):
            d[a + b] = d.get(a + b, 0) + 1 / 36
    return d


# MEASURED substitutions: what the client scores a hit on when the ship has no
# boxes of the charted type. Only one has been observed - an LDR DN with no armor
# took its "Armor" result on Cargo - so only that one is encoded. This is data,
# not a guess; do not extend it without measuring the case in the client the same
# way (apply internals, record the rolls, diff the save).
MISSING_BOX_SUBSTITUTE = {27: 25}        # Armor -> Cargo


def _has(ship, kind):
    """Does this ship have any undestroyed boxes of this chart kind?

    An absent key means ZERO boxes, not "unknown": StateDump emits every kind in
    KIND_TO_SYSTEM, so if the ship's systems dict has no `armor` entry, the ship
    genuinely has no armor. Treating absence as "assume present" made the model
    predict armour hits on ships that have none.
    """
    key = KIND_TO_SYSTEM.get(kind)
    if key is None:
        return True                          # kind we do not track - assume present
    if key == "hull":
        return (ship.get("hull") or [0])[0] > 0
    if key in ("warp", "impulse", "apr", "awr", "battery"):
        return (ship.get("power") or {}).get(key, 0) > 0
    for src in ("systems", "weapons"):
        v = (ship.get(src) or {}).get(key)
        if v is not None:
            return v[0] > 0
    return False


def allocate(ship, roll, consumed):
    """The kind one damage point scores on, given the roll and what is spent.

    `consumed` is the set of BOLD kinds already scored this volley (D4.31); it
    is mutated in place, so call this once per point, in order.
    """
    r = rows()
    if not r or not (2 <= roll <= 12):
        return None
    for kind, flag in r[roll - 2]:
        if flag == 1 and kind in consumed:
            continue                         # bold scores once per volley
        if kind != EXCESS_DAMAGE and not _has(ship, kind):
            sub = MISSING_BOX_SUBSTITUTE.get(kind)
            if sub is not None and _has(ship, sub):
                if flag == 1:
                    consumed.add(kind)
                return sub
            continue
        if flag == 1:
            consumed.add(kind)
        return kind
    return EXCESS_DAMAGE


def expected_hits(ship, internals):
    """{kind: expected hits} over a volley of `internals` points.

    Exact within a row - the bold rule is order-dependent, so each row's own
    sequence is walked - and averaged across rows by their 2d6 probability.
    """
    out = {}
    for roll, p in roll_distribution().items():
        consumed = set()
        for _ in range(max(0, int(internals))):
            k = allocate(ship, roll, consumed)
            if k is None:
                break
            out[k] = out.get(k, 0) + p
    return out


def summarise(ship, internals, top=6):
    """What a volley of this size is likely to kill, most likely first."""
    nm = names()
    ranked = sorted(expected_hits(ship, internals).items(), key=lambda kv: -kv[1])
    return [(nm.get(k, f"kind {k}"), round(v, 2)) for k, v in ranked[:top]]
