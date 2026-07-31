"""
SFB hex geometry for the online-driver environment.

The online game addresses hexes as offset coordinates BoardLocation{x=col, y=row}
(hex label "COLROW", e.g. 1012). The home-grown engine worked in cube coords.
This module bridges them and provides distance / bearing / arc in a single
consistent facing convention.

Facing convention (A-F = 0-5, clockwise), CALIBRATED against the captured live
move: Bismarck {x:10,y:12} -> {x:10,y:11} at facing 0, i.e. facing 0 (A) = "up"
= row-1. In cube space "up" is parity-independent, so the whole 6-direction basis
is exact. (Verify B/C/E/F against a couple more live diagonal moves when handy.)
"""

from __future__ import annotations
from typing import Tuple

from sfb_rules import in_arc  # re-exported for callers

XY = Tuple[int, int]


# offset (odd-q, flat-top columns) -> cube
def to_cube(x: int, y: int) -> Tuple[int, int, int]:
    # EVEN-Q offset. This was odd-q (y - (x-(x&1))//2), which under-counts range
    # by one whenever the two hexes' column parity interacts, and could not even
    # reproduce a logged single-hex move as an adjacency. The client uses even-q:
    # every consecutive "has moved to" pair in the combat log is distance 1 under
    # even-q and 2 under odd-q. Verified against every ship in the live log.
    q = x
    r = y - (x + (x & 1)) // 2
    return (q, -q - r, r)   # (cx, cy, cz)


def _cube_sub(a, b):
    return (a[0] - b[0], a[1] - b[1], a[2] - b[2])


def hex_distance(a: XY, b: XY) -> int:
    d = _cube_sub(to_cube(*a), to_cube(*b))
    return (abs(d[0]) + abs(d[1]) + abs(d[2])) // 2


# facing 0-5 -> cube direction (clockwise, 60° rot = (x,y,z)->(-z,-x,-y))
# calibrated so facing 0 (A) = "up" = cube (0,+1,-1).
FACING_CUBE = [
    (0, 1, -1),   # 0 A  up
    (1, 0, -1),   # 1 B  upper-right
    (1, -1, 0),   # 2 C  lower-right
    (0, -1, 1),   # 3 D  down
    (-1, 0, 1),   # 4 E  lower-left
    (-1, 1, 0),   # 5 F  upper-left
]


def cube_to_offset(c: Tuple[int, int, int]) -> XY:
    q, _, r = c
    x = q
    y = r + (q + (q & 1)) // 2      # even-q inverse of to_cube
    return (x, y)


def neighbor(pos: XY, facing: int) -> XY:
    c = to_cube(*pos)
    d = FACING_CUBE[facing % 6]
    return cube_to_offset((c[0] + d[0], c[1] + d[1], c[2] + d[2]))


def forward_hex(pos: XY, facing: int) -> XY:
    return neighbor(pos, facing)


def absolute_bearing(src: XY, dst: XY) -> int:
    """Which of the 6 facings points most directly from src toward dst.
    Uses directional alignment of the cube-delta against the 6 hex directions
    (a one-hex probe is unstable at long range and mis-picks the bearing)."""
    if src == dst:
        return 0
    a, b = to_cube(*src), to_cube(*dst)
    d = (b[0] - a[0], b[1] - a[1], b[2] - a[2])
    best, best_dot = 0, -10 ** 9
    for f, fc in enumerate(FACING_CUBE):
        dot = d[0] * fc[0] + d[1] * fc[1] + d[2] * fc[2]
        if dot > best_dot:
            best_dot, best = dot, f
    return best


def relative_sextant(src: XY, dst: XY, facing: int) -> int:
    """Bearing to dst expressed relative to the ship's facing (0 = dead ahead)."""
    return (absolute_bearing(src, dst) - facing) % 6


def target_in_arc(src: XY, facing: int, arc_mask: int, dst: XY) -> bool:
    if src == dst:
        return False
    return in_arc(arc_mask, relative_sextant(src, dst, facing))


def shield_hit(target_pos: XY, target_facing: int, attacker_pos: XY) -> int:
    """Which of the target's 6 shields (0=front, relative to its facing) faces
    the attacker. Mirrors engine hit_facing()."""
    return (absolute_bearing(target_pos, attacker_pos) - target_facing) % 6
