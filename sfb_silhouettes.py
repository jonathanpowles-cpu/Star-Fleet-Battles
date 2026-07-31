"""
Top-down ship silhouettes for the tactical board, per faction.

Shapes are polygons in SHIP-LOCAL coordinates: the nose points along -y ("up"),
the hull spans roughly -1..+1 in both axes, and the origin is the ship's centre.
`draw_ship()` rotates them by facing (0 = north, +60 degrees per step, clockwise,
matching sfb_hex.FACING_CUBE) and scales to the hex size.

PROVENANCE - which shapes are actually sourced, and which are approximations:

  KZINTI  drawn from the official ADB counter sheet (Kzinti Pack Counters.png),
          inspected directly: pointed nose, broad angular shoulders, narrow
          central spine, small swept mid-wings, engine cluster aft. RED hull.
  LYRAN   drawn from the official Lyran Pack #1 counters, inspected directly:
          twin-boom catamaran - two parallel hulls with pointed prows joined by
          a central cross-piece, bridge block amidships, rear legs. ORANGE/TAN.
  FED     from the counter-gallery description on file: saucer forward, secondary
          hull trailing, twin nacelles aft. Silver-grey on blue.
  KLINGON from the same source: forward command pod on a neck, wide swept-back
          wings, two rear nacelles. Steel blue-grey, gold trim.
  ROMULAN from the same source: wide rounded forward section, narrow aft twin
          booms. Silver-grey on red.
  OTHERS  GENERIC hulls, clearly marked below. Not authentic - replace by
          inspecting that faction's counter sheet when it matters.

Counter sheets: https://www.starfleetgames.com/ArtGallery/Counter%20Gallery.shtml
"""
from __future__ import annotations
import math

# --------------------------------------------------------------------------
# Faction palettes. hull / trim / accent, plus the counter background colour
# (used for the selection halo so a ship reads as "its" faction at a glance).
# --------------------------------------------------------------------------
PALETTE = {
    "FEDERATION": dict(hull="#afb9c8", dark="#6e788c", trim="#1964c8", accent="#d21e1e",
                       counter="#1a72cc"),
    "KLINGON":    dict(hull="#505f73", dark="#374b5f", trim="#dcb414", accent="#d21e1e",
                       counter="#101010"),
    "ROMULAN":    dict(hull="#afb4be", dark="#7d838f", trim="#c81414", accent="#e0a020",
                       counter="#cc1111"),
    "KZINTI":     dict(hull="#c62222", dark="#8e1414", trim="#e8c020", accent="#4fb6d8",
                       counter="#c62222"),
    "LYRAN":      dict(hull="#e08a30", dark="#a25c1c", trim="#c8b400", accent="#3a86c8",
                       counter="#e8d820"),
    "GORN":       dict(hull="#d2822c", dark="#8f5518", trim="#e8c020", accent="#7ab648",
                       counter="#e07818"),
    "HYDRAN":     dict(hull="#5aa86e", dark="#37704a", trim="#e8e0d0", accent="#e08a30",
                       counter="#3f8f5a"),
    "THOLIAN":    dict(hull="#d8d0c8", dark="#9a8f88", trim="#c81414", accent="#c81414",
                       counter="#d02020"),
    "ORION":      dict(hull="#4fb6d8", dark="#2f7d96", trim="#e8e8e8", accent="#20c860",
                       counter="#1f9ec0"),
}
DEFAULT_PALETTE = dict(hull="#9aa4b2", dark="#68717f", trim="#c9d1d9", accent="#e3b341",
                       counter="#4a5260")


def palette(race):
    return PALETTE.get((race or "").upper(), DEFAULT_PALETTE)


def _mirror(poly):
    """Mirror a polygon across the centreline (x -> -x), reversed for winding."""
    return [(-x, y) for x, y in reversed(poly)]


# --------------------------------------------------------------------------
# SILHOUETTES. Each entry is a list of (polygon, colour-key) pairs, drawn in
# order (back to front). Colour keys index PALETTE: hull / dark / trim / accent.
# --------------------------------------------------------------------------

# --- KZINTI: dagger. Pointed nose, broad shoulders, spine, mid-wings, engines.
_KZ_WING_L = [(-0.14, -0.06), (-0.62, 0.12), (-0.60, 0.24), (-0.14, 0.12)]
_KZ_ENGINE_L = [(-0.34, 0.40), (-0.14, 0.40), (-0.14, 0.98), (-0.34, 0.92)]
KZINTI = [
    (_KZ_WING_L, "dark"), (_mirror(_KZ_WING_L), "dark"),
    (_KZ_ENGINE_L, "dark"), (_mirror(_KZ_ENGINE_L), "dark"),
    ([(-0.10, 0.40), (0.10, 0.40), (0.10, 0.90), (-0.10, 0.90)], "dark"),
    # nose + shoulders as one tapering body
    ([(0.0, -1.00), (0.26, -0.74), (0.36, -0.60), (0.36, -0.36), (0.16, -0.28),
      (0.16, 0.46), (-0.16, 0.46), (-0.16, -0.28), (-0.36, -0.36), (-0.36, -0.60),
      (-0.26, -0.74)], "hull"),
    # shoulder detail flashes
    ([(-0.30, -0.56), (-0.20, -0.56), (-0.20, -0.40), (-0.30, -0.40)], "trim"),
    ([(0.20, -0.56), (0.30, -0.56), (0.30, -0.40), (0.20, -0.40)], "trim"),
    ([(-0.05, -0.86), (0.05, -0.86), (0.05, -0.70), (-0.05, -0.70)], "trim"),
]

# --- LYRAN: twin-boom catamaran, two pointed prows + central cross-piece.
_LY_BOOM_L = [(-0.40, -0.94), (-0.24, -0.68), (-0.24, 0.58), (-0.56, 0.58), (-0.56, -0.68)]
_LY_LEG_L = [(-0.50, 0.58), (-0.30, 0.58), (-0.30, 0.98), (-0.46, 0.98)]
LYRAN = [
    ([(-0.30, -0.16), (0.30, -0.16), (0.30, 0.22), (-0.30, 0.22)], "dark"),   # cross-piece
    (_LY_LEG_L, "dark"), (_mirror(_LY_LEG_L), "dark"),
    (_LY_BOOM_L, "hull"), (_mirror(_LY_BOOM_L), "hull"),
    ([(-0.12, -0.10), (0.12, -0.10), (0.12, 0.12), (-0.12, 0.12)], "accent"),  # bridge
    ([(-0.44, -0.60), (-0.36, -0.60), (-0.36, -0.30), (-0.44, -0.30)], "trim"),
    ([(0.36, -0.60), (0.44, -0.60), (0.44, -0.30), (0.36, -0.30)], "trim"),
]

# --- FEDERATION: saucer forward, secondary hull, twin nacelles aft.
_FED_NAC_L = [(-0.58, -0.18), (-0.36, -0.18), (-0.36, 0.72), (-0.58, 0.72)]
FEDERATION = [
    (_FED_NAC_L, "dark"), (_mirror(_FED_NAC_L), "dark"),
    ([(-0.46, -0.10), (-0.14, 0.06), (-0.14, 0.20), (-0.46, 0.10)], "dark"),  # pylons
    ([(0.46, -0.10), (0.14, 0.06), (0.14, 0.20), (0.46, 0.10)], "dark"),
    ([(-0.16, -0.30), (0.16, -0.30), (0.20, 0.62), (-0.20, 0.62)], "hull"),   # secondary
    ("SAUCER", "hull"),                                                       # see draw_ship
    ([(-0.05, -0.62), (0.05, -0.62), (0.05, -0.48), (-0.05, -0.48)], "trim"),
    ([(-0.56, 0.60), (-0.38, 0.60), (-0.38, 0.72), (-0.56, 0.72)], "accent"),  # nacelle tips
    ([(0.38, 0.60), (0.56, 0.60), (0.56, 0.72), (0.38, 0.72)], "accent"),
]

# --- KLINGON: command pod on a neck, wide swept wings, rear nacelles.
_KL_NAC_L = [(-0.52, 0.30), (-0.32, 0.30), (-0.32, 0.92), (-0.52, 0.92)]
KLINGON = [
    (_KL_NAC_L, "dark"), (_mirror(_KL_NAC_L), "dark"),
    # swept delta wings
    ([(-0.10, -0.06), (-0.92, 0.46), (-0.62, 0.60), (-0.10, 0.34)], "dark"),
    ([(0.10, -0.06), (0.92, 0.46), (0.62, 0.60), (0.10, 0.34)], "dark"),
    ([(-0.18, -0.10), (0.18, -0.10), (0.22, 0.62), (-0.22, 0.62)], "hull"),   # body
    ([(-0.07, -0.74), (0.07, -0.74), (0.07, -0.06), (-0.07, -0.06)], "hull"), # neck
    ("POD", "hull"),                                                          # command pod
    ([(-0.50, 0.82), (-0.34, 0.82), (-0.34, 0.92), (-0.50, 0.92)], "accent"),
    ([(0.34, 0.82), (0.50, 0.82), (0.50, 0.92), (0.34, 0.92)], "accent"),
]

# --- ROMULAN: wide rounded forward section, narrow aft twin booms.
_RO_BOOM_L = [(-0.34, 0.24), (-0.18, 0.24), (-0.18, 0.88), (-0.34, 0.88)]
ROMULAN = [
    (_RO_BOOM_L, "dark"), (_mirror(_RO_BOOM_L), "dark"),
    ([(0.0, -0.86), (0.46, -0.56), (0.62, -0.10), (0.40, 0.28), (-0.40, 0.28),
      (-0.62, -0.10), (-0.46, -0.56)], "hull"),
    ([(-0.10, -0.62), (0.10, -0.62), (0.10, -0.40), (-0.10, -0.40)], "trim"),
    ([(-0.30, 0.06), (0.30, 0.06), (0.30, 0.20), (-0.30, 0.20)], "dark"),
]

# --- GENERIC hulls for factions whose counter art has not been inspected.
_GEN_WING_L = [(-0.16, -0.10), (-0.66, 0.18), (-0.62, 0.32), (-0.16, 0.16)]
GENERIC = [
    (_GEN_WING_L, "dark"), (_mirror(_GEN_WING_L), "dark"),
    ([(0.0, -0.90), (0.28, -0.52), (0.26, 0.60), (-0.26, 0.60), (-0.28, -0.52)], "hull"),
    ([(-0.24, 0.60), (-0.08, 0.60), (-0.08, 0.90), (-0.24, 0.90)], "dark"),
    ([(0.08, 0.60), (0.24, 0.60), (0.24, 0.90), (0.08, 0.90)], "dark"),
    ([(-0.06, -0.70), (0.06, -0.70), (0.06, -0.50), (-0.06, -0.50)], "trim"),
]

SILHOUETTE = {
    "KZINTI": KZINTI, "LYRAN": LYRAN, "FEDERATION": FEDERATION,
    "KLINGON": KLINGON, "ROMULAN": ROMULAN,
}
# Factions using GENERIC - listed explicitly so it is obvious these are not
# authentic silhouettes yet.
GENERIC_FACTIONS = {"GORN", "HYDRAN", "THOLIAN", "ORION", "ISC", "ANDROMEDAN",
                    "WYN", "LDR", "SELTORIAN"}

# Bases and small craft do not use a ship hull at all.
BASE_TYPES = {"BS", "BSX", "BATS", "BATSX", "SB", "SBX", "STB", "STX", "MB", "FRD", "MONITOR"}
SMALL_TYPES = {"PF", "FIGHTER", "INT", "GDN", "SHUTTLE", "ADMIN"}


def silhouette_for(race, ship_type=""):
    t = (ship_type or "").upper().replace("-", "").strip()
    if t in BASE_TYPES:
        return "BASE"
    if t in SMALL_TYPES:
        return "SMALL"
    return SILHOUETTE.get((race or "").upper(), GENERIC)


def _rot(pt, theta):
    """Rotate a ship-local point clockwise on screen (y down) by theta radians."""
    x, y = pt
    ct, st = math.cos(theta), math.sin(theta)
    return (x * ct - y * st, x * st + y * ct)


def draw_ship(canvas, cx, cy, size, race, ship_type="", facing=0, selected=False,
              outline=None, tags=()):
    """Draw a ship silhouette centred at (cx, cy), nose along `facing`.

    size is the hull half-length in pixels. Returns the list of canvas item ids.
    """
    pal = palette(race)
    shape = silhouette_for(race, ship_type)
    theta = math.radians(60 * (facing % 6))
    ids = []
    edge = outline or ("#ffffff" if selected else "#0b0e13")
    w = 2 if selected else 1

    def place(poly):
        pts = []
        for p in poly:
            rx, ry = _rot(p, theta)
            pts.extend((cx + rx * size, cy + ry * size))
        return pts

    if shape == "BASE":
        # Bases are fixed installations - an octagonal station, no facing.
        r = size * 0.85
        pts = []
        for a in range(0, 360, 45):
            pts.extend((cx + r * math.cos(math.radians(a + 22.5)),
                        cy + r * math.sin(math.radians(a + 22.5))))
        ids.append(canvas.create_polygon(pts, fill=pal["hull"], outline=edge, width=w,
                                         tags=tags))
        ids.append(canvas.create_oval(cx - r * 0.4, cy - r * 0.4, cx + r * 0.4, cy + r * 0.4,
                                      fill=pal["dark"], outline="", tags=tags))
        return ids

    if shape == "SMALL":
        # PFs / fighters - a small dart, still facing-aware.
        dart = [(0.0, -0.9), (0.5, 0.5), (0.0, 0.2), (-0.5, 0.5)]
        ids.append(canvas.create_polygon(place(dart), fill=pal["hull"], outline=edge,
                                         width=w, tags=tags))
        return ids

    for poly, key in shape:
        colr = pal[key]
        if poly == "SAUCER":
            # circular saucer, offset forward along the ship's axis
            ox, oy = _rot((0.0, -0.52), theta)
            r = size * 0.44
            ids.append(canvas.create_oval(cx + ox * size - r, cy + oy * size - r,
                                          cx + ox * size + r, cy + oy * size + r,
                                          fill=colr, outline=edge, width=w, tags=tags))
        elif poly == "POD":
            ox, oy = _rot((0.0, -0.82), theta)
            r = size * 0.19
            ids.append(canvas.create_oval(cx + ox * size - r, cy + oy * size - r,
                                          cx + ox * size + r, cy + oy * size + r,
                                          fill=colr, outline=edge, width=w, tags=tags))
        else:
            ids.append(canvas.create_polygon(place(poly), fill=colr, outline=edge,
                                             width=w, tags=tags))
    return ids


def is_authentic(race):
    """True when this faction's silhouette came from inspected counter art."""
    return (race or "").upper() in {"KZINTI", "LYRAN", "FEDERATION", "KLINGON", "ROMULAN"}
