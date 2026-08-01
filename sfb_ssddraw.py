"""
Draw the authentic SSD from the client's own layout data.

StateDump now emits ship['ssd_boxes']: every box group with the client's real
position (x,y,w,h), kind, count and per-box status. This module renders that on
a Tk canvas - the record sheet the client itself uses, with EXACT box-level
damage, for every hull in the save. No scans, no indexing.

Status semantics (observed): 1 = intact. Anything else is drawn as damage -
filled red - so an unknown status value can never hide a hit.
"""
from __future__ import annotations

# kind -> display colour family. Kinds are the client's boxtype ids
# (client_data/boxtypes.names); groups we don't specially colour get neutral.
_SHIELD_KIND = 26
_COLOURS = {
    # power
    6: "#e3b341", 12: "#e3b341", 17: "#e3b341",      # warp engines
    16: "#e3b341", 40: "#e3b341",                    # impulse
    20: "#d4a017", 32: "#d4a017", 18: "#b8860b",     # APR / AWR / battery
    # weapons
    29: "#ffa657", 28: "#ffa657", 15: "#ffa657",     # disruptor / photon / phaser
    33: "#ffa657", 34: "#ffa657", 35: "#ffa657", 36: "#ffa657", 37: "#ffa657",
    # drone racks / ADD: PURPLE - the old light-red was one shade off the
    # destroyed fill, so a healthy rack read as damage at a glance.
    14: "#c9a0ff", 39: "#c9a0ff", 62: "#c9a0ff", 63: "#c9a0ff",
    38: "#c9a0ff",
    # hull / structure
    5: "#8b949e", 11: "#8b949e", 13: "#6e7681",      # hull / excess
    # shields
    _SHIELD_KIND: "#79c0ff",
}
_INTACT_BG = "#161b22"
_DESTROYED = "#f85149"
_OUTLINE = "#30363d"
_TEXT = "#c9d1d9"

_NAMES = None
_ABBREV = None


def _kind_name(kind):
    global _NAMES
    if _NAMES is None:
        try:
            import sfb_resolve as RES
            _NAMES = {int(n.rsplit("=", 1)[1]): n.rsplit("=", 1)[0]
                      for n in RES._load_boxtypes() if "=" in n}
        except Exception:
            _NAMES = {}
    return _NAMES.get(kind, f"kind {kind}")


def _abbrev(kind):
    """The client's OWN short label for a box kind (abbrev_boxtypes.names) -
    'Disr', 'Ph-1', 'Batt', 'L Warp'. Falls back to the full name."""
    global _ABBREV
    if _ABBREV is None:
        import os
        path = os.path.join(os.path.dirname(os.path.abspath(__file__)),
                            "client_data", "abbrev_boxtypes.names")
        _ABBREV = {}
        try:
            with open(path, encoding="utf-8", errors="replace") as f:
                for ln in f:
                    if "=" in ln:
                        nm, _, num = ln.strip().rpartition("=")
                        try:
                            _ABBREV[int(num)] = nm
                        except ValueError:
                            pass
        except OSError:
            pass
    return _ABBREV.get(kind) or _kind_name(kind)


def draw(canvas, boxes, width, height, title=""):
    """Render ssd_boxes onto the canvas, scaled to (width, height).

    Each group's rect is split into its individual box cells along the major
    axis (the client lays strips that way - a 170x18 rect with 34 boxes is 34
    cells across). Intact cells are outlined; destroyed cells fill red.
    Returns the number of destroyed boxes drawn.
    """
    canvas.delete("all")
    if not boxes:
        canvas.create_text(width // 2, height // 2, fill=_TEXT,
                           text="no SSD layout in save", font=("Consolas", 10))
        return 0
    x0 = min(b["x"] for b in boxes)
    y0 = min(b["y"] for b in boxes)
    x1 = max(b["x"] + b["w"] for b in boxes)
    y1 = max(b["y"] + b["h"] for b in boxes)
    span_x, span_y = max(1, x1 - x0), max(1, y1 - y0)
    pad = 14
    sc = min((width - 2 * pad) / span_x, (height - 2 * pad - 16) / span_y)

    def X(v):
        return pad + (v - x0) * sc

    def Y(v):
        return pad + 14 + (v - y0) * sc

    if title:
        canvas.create_text(width // 2, 10, text=title, fill=_TEXT,
                           font=("Consolas", 10, "bold"))
    destroyed = 0
    for b in boxes:
        n = max(1, int(b.get("n") or b.get("max") or 1))
        status = b.get("status") or []
        colour = _COLOURS.get(b.get("kind"), _INTACT_BG)
        gx, gy = X(b["x"]), Y(b["y"])
        gw, gh = b["w"] * sc, b["h"] * sc
        horizontal = b["w"] >= b["h"]
        for i in range(n):
            if horizontal:
                cx0 = gx + gw * i / n
                cx1 = gx + gw * (i + 1) / n
                cy0, cy1 = gy, gy + gh
            else:
                cy0 = gy + gh * i / n
                cy1 = gy + gh * (i + 1) / n
                cx0, cx1 = gx, gx + gw
            st = status[i] if i < len(status) else 1
            dead = st != 1
            if dead:
                destroyed += 1
            canvas.create_rectangle(cx0, cy0, cx1, cy1,
                                    fill=_DESTROYED if dead else colour,
                                    outline=_OUTLINE)
        # EVERY group gets a label (the client's own abbreviation - 'Disr',
        # 'Ph-1', 'Batt' - plus the mount designation where one exists).
        # Inside the strip when it fits, otherwise just outside it: above a
        # horizontal strip, to the left of a vertical one - so small mounts
        # stay identifiable without cluttering the cells themselves.
        kind = b.get("kind")
        des = str(b.get("des") or "").strip()
        if kind == _SHIELD_KIND and des:
            lbl = f"#{des}"
        else:
            lbl = _abbrev(kind)
            # a single mount letter/number is a designation worth showing;
            # multi-value strings (damage-control repair tracks etc.) are data,
            # not a name - they'd bloat the label into noise.
            if des and des != lbl and len(des) <= 3:
                lbl = f"{lbl} {des}"
        fits_inside = gw > 8 * len(lbl) and gh > 11
        if fits_inside:
            canvas.create_text(gx + gw / 2, gy + gh / 2, text=lbl,
                               fill="#0d1117" if kind in _COLOURS else _TEXT,
                               font=("Consolas", 7, "bold"))
        elif horizontal:
            canvas.create_text(gx, gy - 6, text=lbl, fill=_TEXT,
                               anchor="w", font=("Consolas", 6))
        else:
            canvas.create_text(gx - 3, gy + gh / 2, text=lbl, fill=_TEXT,
                               anchor="e", font=("Consolas", 6))
    return destroyed
