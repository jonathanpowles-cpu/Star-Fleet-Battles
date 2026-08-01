"""
SFB Fleet Bridge - the tactical display layer.

A tabbed window over the live battle, replacing the single scrolling text console:

    BOARD   hex map: ships, facings, shield states, range rings, firing arcs,
            seeker tracks. Spatial truth shown spatially - this is also an
            error-catching surface (a wrong bearing or turn mode is obvious here
            in a way it never is in a sentence).
    BRIDGE  the commander's view: orders for the AI side, advice for yours,
            voiced in character by the LLM (sfb_voice), grounded in engine facts.
    SHIPS   per-ship detail: tactical advice, EAF, and SSD state.
    COMMS   inter-ship traffic - captains coordinating, chivvying, taunting.

Read-only by construction. Every number comes from the client's own autosave and
combat log; this window never writes to the game and is never the referee.

    python sfb_bridge.py --ai Kzinti --advise Lyran [--no-voice]
"""
from __future__ import annotations
import os, sys, math, glob, time, argparse, traceback, importlib
import tkinter as tk
from tkinter import ttk, scrolledtext, font as tkfont
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import sfb_command as cmd
import sfb_hex as H
import sfb_voice as V
import sfb_silhouettes as SIL
import sfb_shadow as SHADOW

RESTORE = os.path.dirname(cmd.AUTOSAVE)

# ---------------------------------------------------------------- appearance
BG      = "#0d1117"
PANEL   = "#161b22"
FG      = "#c9d1d9"
MUTED   = "#8b949e"
GRID    = "#303a46"     # visible but not competing with the ships
ACCENT  = "#f0f6fc"
SIDE_A  = "#ff7b72"     # the AI's side (orders)
SIDE_B  = "#79c0ff"     # your side (advice)
GOLD    = "#e3b341"
GREEN   = "#56d364"
RED     = "#f85149"
ORANGE  = "#ffa657"
TEAL    = "#79d9c0"
PURPLE  = "#c9a0ff"

HEX_SIZE = 26           # circumradius in pixels at zoom 1.0


# ---------------------------------------------------------------- hex → pixel
def hex_center(col, row, size, ox, oy):
    """Flat-top, EVEN-Q offset layout, matching sfb_hex.to_cube.

    Even-q pushes odd columns UP (-0.5), where odd-q pushed them down. This must
    track the coordinate convention or ships render a half-hex off on alternating
    columns; the convention was corrected to even-q after the combat log proved
    the client uses it (single-hex moves are distance 1 only under even-q).
    facing 0 (A) = north = row-1, so 'up' on screen is up on the board.
    """
    x = size * 1.5 * col
    y = size * math.sqrt(3) * (row - 0.5 * (col & 1))
    return x + ox, y + oy


def hex_points(cx, cy, size):
    """Flat-top hex vertices (angles 0, 60, ... 300)."""
    return [(cx + size * math.cos(math.radians(a)),
             cy + size * math.sin(math.radians(a))) for a in range(0, 360, 60)]


def facing_angle(facing):
    """Screen angle in degrees for a facing. Facing 0 = north = -90 degrees."""
    return -90 + 60 * (facing % 6)


# A save with no ships on the board is only a few KB (lobby/empty game); a real
# battle is tens of KB. Parsing is expensive (it spawns a JVM), so use size as a
# cheap prefilter before trying to read one.
MIN_LIVE_SAVE_BYTES = 20_000


def tactical_save_candidates():
    """All plausible tactical saves, newest first, INCLUDING .bak rolls.

    The client writes an almost-empty file when a game is closed or still in the
    lobby, so the newest file by mtime is often not the live battle. Backups hold
    the real state in that case.
    """
    cands = []
    for pat in ("game#SFB_Game*", "gameSFB_Game*", "game#SFBCadet_Game*",
                "gameSFBCadet_Game*"):
        for p in glob.glob(os.path.join(RESTORE, pat)):
            if "Campaign" in p or "FNE" in p:
                continue
            try:
                cands.append((os.path.getmtime(p), os.path.getsize(p), p))
            except OSError:
                pass
    cands.sort(reverse=True)
    return cands


def newest_tactical_save():
    """Newest save that plausibly contains a battle (by size)."""
    for _mt, size, p in tactical_save_candidates():
        if size >= MIN_LIVE_SAVE_BYTES:
            return p
    c = tactical_save_candidates()
    return c[0][2] if c else cmd.AUTOSAVE


# Parsing a save spawns a JVM (StateDump) and costs 1.5-4.5 SECONDS. The refresh
# tick runs every 1.5s, so re-parsing an unchanged file would keep a JVM starting
# permanently and make the whole window feel sluggish. Cache on (path, mtime,
# size): the save only changes when you act in the client.
_STATE_CACHE = {}
_STATE_CACHE_MAX = 8


def _parse_cached(path):
    try:
        key = (path, os.path.getmtime(path), os.path.getsize(path))
    except OSError:
        return None
    hit = _STATE_CACHE.get(key)
    if hit is not None:
        return hit
    try:
        st = cmd.dump_state(path)
    except Exception:
        return None
    if st and st.get("ships"):
        if len(_STATE_CACHE) >= _STATE_CACHE_MAX:
            _STATE_CACHE.pop(next(iter(_STATE_CACHE)))
        _STATE_CACHE[key] = st
    return st


# The save this session is PINNED to. Once chosen it never changes on its own.
#
# The old behaviour was "newest file above 20KB wins", re-evaluated every refresh
# tick, so the console silently followed whatever the client last wrote: creating
# a one-ship scratch scenario to check a rules value displaced a live six-ship
# battle, and the console began advising on the test ship without saying so.
# A save is now chosen ONCE, explicitly, and the console stays on it.
_PINNED_SAVE = None
_PINNED_GAME = None      # frozenset of ship labels - the pinned game's IDENTITY


def _fingerprint(st):
    return frozenset(sh.get("label") for sh in (st or {}).get("ships") or [])


def pin_save(path):
    """Pin the console to the GAME in this file, not to the filename.

    The client's backups ROTATE: gameSFB_Game1.bak.2 is a slot, not a game, and
    within hours of the first pin it held a different scenario entirely - the
    console sat on a stale snapshot while the real battle rolled forward under
    another name. So remember the game's identity (its ship set) and follow
    THAT: if the pinned file no longer contains our game, look for the file
    that does.
    """
    global _PINNED_SAVE, _PINNED_GAME
    _PINNED_SAVE = path
    st = _parse_cached(path)
    _PINNED_GAME = _fingerprint(st) if st else None
    # The named file may already be a stub - the client rotates the plain
    # gameSFB_Game1 to a .bak the moment it writes, so a --save pointing at it
    # (or a pin captured before a crash) can resolve to nothing. If so, DON'T
    # give up with a null fingerprint (which disables identity-following and
    # leaves an empty board). Adopt the newest real game on disk instead, and
    # follow THAT by identity.
    if not _PINNED_GAME:
        for _mt, size, p in tactical_save_candidates():
            if size < MIN_LIVE_SAVE_BYTES:
                continue
            st2 = _parse_cached(p)
            if st2 and st2.get("ships"):
                _PINNED_SAVE = p
                _PINNED_GAME = _fingerprint(st2)
                break
    return path


def pinned_save():
    return _PINNED_SAVE


def _repin_by_identity():
    """Find the newest candidate holding the pinned game; None if it is gone."""
    global _PINNED_SAVE
    for _mt, size, p in tactical_save_candidates():
        if size < MIN_LIVE_SAVE_BYTES:
            continue
        st = _parse_cached(p)
        if st and _fingerprint(st) == _PINNED_GAME:
            _PINNED_SAVE = p
            return st, p
    return None, None


def describe_saves(max_tries=8):
    """Candidate saves with enough detail to choose between them.

    [(path, size, mtime, nships, turn, impulse, races)] - each is parsed, because
    file size alone does not distinguish a real battle from a stub.
    """
    out = []
    for mt, size, p in tactical_save_candidates()[:max_tries]:
        if size < MIN_LIVE_SAVE_BYTES:
            continue
        st = _parse_cached(p)
        ships = (st or {}).get("ships") or []
        if not ships:
            continue
        races = sorted({s.get("race") or "?" for s in ships})
        out.append((p, size, mt, len(ships), (st or {}).get("turn"),
                    (st or {}).get("impulse"), races))
    return out


def load_live_state(max_tries=4):
    """Read the PINNED save; auto-select only if nothing has been pinned.

    When pinned, only that file is read. It is still re-parsed as it changes -
    that is how the console follows the game - but the console will never switch
    to a DIFFERENT file by itself.
    """
    if _PINNED_SAVE:
        st = _parse_cached(_PINNED_SAVE)
        if st and st.get("ships"):
            if _PINNED_GAME is None or _fingerprint(st) == _PINNED_GAME:
                return st, _PINNED_SAVE
            # The FILE is readable but no longer holds OUR game - the backup
            # slots rotated under us. Follow the game, not the filename.
            st2, p2 = _repin_by_identity()
            if st2:
                return st2, p2
            return None, _PINNED_SAVE
        # Unreadable or empty: the game may have rolled to another slot.
        st2, p2 = _repin_by_identity()
        if st2:
            return st2, p2
        return None, _PINNED_SAVE
    tried = 0
    for _mt, size, p in tactical_save_candidates():
        if size < MIN_LIVE_SAVE_BYTES or tried >= max_tries:
            continue
        tried += 1
        st = _parse_cached(p)
        if st and st.get("ships"):
            return st, p
    return None, None


# ------------------------------------------------------------------ hot reload
_RELOADABLE = ["sfb_rules", "sfb_hex", "sfb_log", "sfb_situations", "sfb_maneuver",
               "sfb_doctrine", "sfb_charts", "sfb_resolve", "sfb_shadow", "sfb_replay",
               "sfb_command"]
_mtimes = {}


def _src(name):
    m = sys.modules.get(name)
    return getattr(m, "__file__", None) if m else None


def reload_if_changed():
    changed = []
    for n in _RELOADABLE:
        p = _src(n)
        if not p or not os.path.exists(p):
            continue
        try:
            mt = os.path.getmtime(p)
        except OSError:
            continue
        if _mtimes.get(n) is None:
            _mtimes[n] = mt
        elif mt > _mtimes[n]:
            _mtimes[n] = mt
            changed.append(n)
    if not changed:
        return []
    first = min(_RELOADABLE.index(n) for n in changed)
    out = []
    for n in _RELOADABLE[first:]:
        if n in sys.modules:
            try:
                importlib.reload(sys.modules[n])
                out.append(n)
            except Exception:
                pass
    globals()["cmd"] = sys.modules.get("sfb_command", cmd)
    return out


# =============================================================== BOARD CANVAS
class BoardView(tk.Frame):
    """The hex map. Draws what the engine believes, so mistakes are visible."""

    def __init__(self, master, app):
        super().__init__(master, bg=BG)
        self.app = app
        self.zoom = 1.0
        self.sel = None
        # Pan offset in pixels, added on top of the auto-fit origin. Zero means
        # "auto-fit centred"; the moment the user zooms or pans we start honouring
        # this instead of yanking the view back to fit.
        self.pan = [0.0, 0.0]
        self._layout = None            # (size, ox, oy) from the last redraw
        self._mouse = None             # last cursor pos inside the canvas
        self._drag = None              # anchor for drag-panning
        self.show = {"arcs": tk.BooleanVar(value=True),
                     "rings": tk.BooleanVar(value=True),
                     "seekers": tk.BooleanVar(value=True),
                     "craft": tk.BooleanVar(value=True),
                     "tethers": tk.BooleanVar(value=False),
                     "labels": tk.BooleanVar(value=True)}
        self.edge_scroll = tk.BooleanVar(value=True)

        bar = tk.Frame(self, bg=BG)
        bar.pack(fill="x", padx=6, pady=(6, 0))
        for key, txt in (("arcs", "firing arcs"), ("rings", "range rings"),
                         ("seekers", "seekers"), ("craft", "fighters/shuttles"),
                         ("tethers", "tethers"), ("labels", "labels")):
            tk.Checkbutton(bar, text=txt, variable=self.show[key], command=self.redraw,
                           bg=BG, fg=MUTED, selectcolor=BG, activebackground=BG,
                           activeforeground=FG, font=("Segoe UI", 9)).pack(side="left")
        tk.Checkbutton(bar, text="edge scroll", variable=self.edge_scroll,
                       bg=BG, fg=MUTED, selectcolor=BG, activebackground=BG,
                       activeforeground=FG, font=("Segoe UI", 9)).pack(side="left")
        tk.Button(bar, text="-", width=2, command=lambda: self._zoom(1 / 1.25), bg=PANEL,
                  fg=FG, relief="flat").pack(side="right", padx=2)
        tk.Button(bar, text="+", width=2, command=lambda: self._zoom(1.25), bg=PANEL,
                  fg=FG, relief="flat").pack(side="right")
        tk.Button(bar, text="Fit", command=self.fit, bg=PANEL, fg=FG,
                  relief="flat", padx=8).pack(side="right", padx=4)
        self.hint = tk.Label(bar, text="wheel = zoom   drag = pan   edge = scroll",
                             bg=BG, fg=MUTED, font=("Segoe UI", 9))
        self.hint.pack(side="right", padx=10)

        self.canvas = tk.Canvas(self, bg=BG, highlightthickness=0)
        self.canvas.pack(fill="both", expand=True, padx=6, pady=6)
        self.canvas.bind("<Button-1>", self._click)
        self.canvas.bind("<Configure>", lambda e: self.redraw())
        # --- zoom: mouse wheel, anchored on the cursor
        self.canvas.bind("<MouseWheel>", self._wheel)              # Windows / macOS
        self.canvas.bind("<Button-4>", lambda e: self._wheel(e, 1))   # X11 up
        self.canvas.bind("<Button-5>", lambda e: self._wheel(e, -1))  # X11 down
        # --- pan: drag with right or middle button (left is ship selection)
        for btn in ("3", "2"):
            self.canvas.bind(f"<Button-{btn}>", self._drag_start)
            self.canvas.bind(f"<B{btn}-Motion>", self._drag_move)
            self.canvas.bind(f"<ButtonRelease-{btn}>", self._drag_end)
        # --- edge scroll: track the cursor, pan on a timer while near an edge
        self.canvas.bind("<Motion>", self._track)
        self.canvas.bind("<Leave>", lambda e: setattr(self, "_mouse", None))
        self._edge_tick()

    # ------------------------------------------------------------ view control
    def fit(self):
        """Back to auto-fit: whole engagement centred."""
        self.zoom = 1.0
        self.pan = [0.0, 0.0]
        self.redraw()

    def _zoom(self, f, at=None):
        """Scale by f, keeping the board point under `at` (or the centre) fixed."""
        old = self.zoom
        self.zoom = max(0.25, min(6.0, self.zoom * f))
        if self.zoom == old or not self._layout:
            self.redraw()
            return
        size0, ox0, oy0 = self._layout
        cw, ch = self.canvas.winfo_width(), self.canvas.winfo_height()
        mx, my = at or (cw / 2, ch / 2)
        # Scale-invariant board coords under the cursor, before the zoom
        u, v = (mx - ox0) / size0, (my - oy0) / size0
        # Redraw to learn the new auto-fit origin at the new scale, then correct
        # the pan so that same board point lands back under the cursor.
        self.redraw()
        size1, ox1, oy1 = self._layout
        self.pan[0] += mx - (u * size1 + ox1)
        self.pan[1] += my - (v * size1 + oy1)
        self.redraw()

    def _wheel(self, ev, direction=None):
        d = direction if direction is not None else (1 if ev.delta > 0 else -1)
        self._zoom(1.15 if d > 0 else 1 / 1.15, at=(ev.x, ev.y))
        return "break"

    def _drag_start(self, ev):
        self._drag = (ev.x, ev.y, self.pan[0], self.pan[1])
        self.canvas.configure(cursor="fleur")

    def _drag_move(self, ev):
        if not self._drag:
            return
        x0, y0, px, py = self._drag
        self.pan[0] = px + (ev.x - x0)
        self.pan[1] = py + (ev.y - y0)
        self.redraw()

    def _drag_end(self, _ev):
        self._drag = None
        self.canvas.configure(cursor="")

    def _track(self, ev):
        self._mouse = (ev.x, ev.y)

    def _edge_tick(self):
        """Pan while the cursor rests near an edge. Speed ramps with proximity."""
        try:
            if (self.edge_scroll.get() and self._mouse and not self._drag
                    and self.winfo_ismapped()):
                mx, my = self._mouse
                cw, ch = self.canvas.winfo_width(), self.canvas.winfo_height()
                # Wide enough to hit without hunting for it - roughly a
                # thumb's width in from each edge.
                # Smaller zone all round: a wide band meant the board panned
                # during ordinary cursor transit. The top edge scrolls again -
                # the canvas sits BELOW the tab strip, so a modest margin there
                # is safe and top-edge panning is genuinely useful.
                MARGIN, MAX_SPEED = 55, 22
                dx = dy = 0.0
                if mx < MARGIN:
                    dx = (MARGIN - mx) / MARGIN * MAX_SPEED
                elif mx > cw - MARGIN:
                    dx = -(mx - (cw - MARGIN)) / MARGIN * MAX_SPEED
                if my < MARGIN:
                    dy = (MARGIN - my) / MARGIN * MAX_SPEED
                elif my > ch - MARGIN:
                    dy = -(my - (ch - MARGIN)) / MARGIN * MAX_SPEED
                if dx or dy:
                    self.pan[0] += dx
                    self.pan[1] += dy
                    self.redraw()
        except Exception:
            pass
        self.after(33, self._edge_tick)

    def _click(self, ev):
        best, bd = None, 1e9
        for s, (px, py) in self._positions.items():
            d = (px - ev.x) ** 2 + (py - ev.y) ** 2
            if d < bd:
                bd, best = d, s
        if best and bd < (HEX_SIZE * self.zoom * 1.4) ** 2:
            self.sel = best
            self.app.select_ship(best)
            self.redraw()

    def redraw(self):
        c = self.canvas
        c.delete("all")
        self._positions = {}
        st = self.app.state
        if not st:
            c.create_text(20, 20, anchor="nw", fill=MUTED, text="waiting for a battle...",
                          font=("Consolas", 11))
            return
        ships = st["ships"]
        if not ships:
            return
        cols = [s["x"] for s in ships]
        rows = [s["y"] for s in ships]
        pad = 3
        c0, r0 = min(cols) - pad, min(rows) - pad
        c1, r1 = max(cols) + pad, max(rows) + pad

        # Auto-fit: scale so the whole engagement is on screen, then centre it by
        # its ACTUAL pixel bounding box. Deriving the box from the grid span alone
        # gets the offsets wrong and pushes distant ships off-canvas.
        cw = max(200, self.canvas.winfo_width())
        ch = max(200, self.canvas.winfo_height())
        span_c, span_r = (c1 - c0 + 1), (r1 - r0 + 1)
        fit = min(cw / (span_c * 1.5 + 2), ch / (span_r * math.sqrt(3) + 2))
        fit = max(7.0, min(70.0, fit))
        # Once the user has zoomed or panned, FREEZE the auto-fit basis. Otherwise
        # ships moving apart would shrink the fit under them and their view would
        # drift mid-battle. "Fit" restores automatic behaviour.
        touched = self.zoom != 1.0 or self.pan != [0.0, 0.0]
        if not touched:
            self._base_fit = fit
        base = getattr(self, "_base_fit", None) or fit
        size = (base if touched else fit) * self.zoom

        # true extent at this size, including the hex bodies at the edges
        # The centring box is pinned to the ships' extent at the FROZEN scale, so
        # a panned view stays put as the engagement develops.
        xs, ys = [], []
        for cc in (c0, c1):
            for rr in (r0, r1):
                px, py = hex_center(cc, rr, size, 0, 0)
                xs.append(px)
                ys.append(py)
        minx, maxx = min(xs) - size, max(xs) + size
        miny, maxy = min(ys) - size, max(ys) + size
        ox = (cw - (maxx - minx)) / 2 - minx + self.pan[0]
        oy = (ch - (maxy - miny)) / 2 - miny + self.pan[1]
        # Remembered so _zoom() can anchor on the cursor and so panning has a
        # frame of reference. Includes the pan, which is what the maths expects.
        self._layout = (size, ox, oy)

        # ---- grid: cover what is actually VISIBLE, not just the ships' bounding
        # box, so panning and zooming don't reveal a void beyond the hexes.
        gc0 = int(math.floor((0 - ox) / (1.5 * size))) - 1
        gc1 = int(math.ceil((cw - ox) / (1.5 * size))) + 1
        gr0 = int(math.floor((0 - oy) / (math.sqrt(3) * size))) - 1
        gr1 = int(math.ceil((ch - oy) / (math.sqrt(3) * size))) + 1
        if (gc1 - gc0 + 1) * (gr1 - gr0 + 1) <= 6000:      # skip if absurdly zoomed out
            for col in range(gc0, gc1 + 1):
                for row in range(gr0, gr1 + 1):
                    cx, cy = hex_center(col, row, size, ox, oy)
                    c.create_polygon(hex_points(cx, cy, size), outline=GRID,
                                     fill="", width=1)
                    if size >= 22:
                        c.create_text(cx, cy - size * 0.62, text=f"{col:02d}{row:02d}",
                                      fill="#2c333c", font=("Consolas", 6))

        ai_side = self.app.ai.upper()
        # ---- range rings from the selected (or first friendly) ship
        anchor = None
        for s in ships:
            if s["label"] == self.sel:
                anchor = s
        if anchor is None:
            for s in ships:
                if s["race"].upper() == self.app.advise.upper():
                    anchor = s
                    break
        if anchor is not None and self.show["rings"].get():
            ax, ay = hex_center(anchor["x"], anchor["y"], size, ox, oy)
            # Range bands that actually change decisions, from the verified models:
            #   3  best ADD envelope (E5.61)     5  Ph-1 cliff (-38% at 6)
            #   8  overload / disruptor band     15 long-range duel
            for rng, colr, lbl in ((3, "#7a4a4a", "3 ADD"), (5, "#7a7040", "5 ph-1 cliff"),
                                   (8, "#3f7a5c", "8 overload"), (15, "#3c5a7a", "15")):
                rpx = size * math.sqrt(3) * rng
                c.create_oval(ax - rpx, ay - rpx, ax + rpx, ay + rpx,
                              outline=colr, dash=(4, 4))
                c.create_text(ax, ay - rpx - 7, text=lbl, fill=colr,
                              font=("Consolas", 8))

        # ---- seekers, grouped by hex. Drones launch in salvos and the client
        # stacks them - six drones in two hexes drew as two anonymous dots, so
        # the board under-reported the threat by a factor of three. One marker
        # per hex with a count badge tells the truth at a glance.
        if self.show["seekers"].get():
            by_hex = {}
            for sk in st.get("seeking", []) or []:
                by_hex.setdefault((sk.get("x"), sk.get("y")), []).append(sk)
            for (hx, hy), group in by_hex.items():
                try:
                    sx, sy = hex_center(hx, hy, size, ox, oy)
                except Exception:
                    continue
                r = 4 + min(3, len(group) - 1)
                c.create_oval(sx - r, sy - r, sx + r, sy + r, fill=RED, outline="")
                for tgt_lbl in {g.get("target") for g in group}:
                    tgt = next((t for t in ships if t["label"] == tgt_lbl), None)
                    if tgt:
                        tx, ty = hex_center(tgt["x"], tgt["y"], size, ox, oy)
                        c.create_line(sx, sy, tx, ty, fill="#5a2222", dash=(2, 3))
                kinds = {g.get("kind", "seek") for g in group}
                spd = max(int(g.get("speed") or 0) for g in group)
                txt = f'{len(group)}x {"/".join(sorted(kinds))} {spd}'
                c.create_text(sx, sy - r - 6, text=txt, fill=RED,
                              font=("Consolas", 7))

        # ---- fighters and shuttles, grouped by hex. These were parsed into the
        # state (they are real board pieces) but never drawn - the "craft"
        # toggle existed with no layer behind it, so a 14-fighter furball was
        # simply invisible on the tactical map.
        if self.show["craft"].get():
            by_hex = {}
            for cr in st.get("shuttles", []) or []:
                by_hex.setdefault((cr.get("x"), cr.get("y")), []).append(cr)
            for (hx, hy), group in by_hex.items():
                try:
                    sx, sy = hex_center(hx, hy, size, ox, oy)
                except Exception:
                    continue
                mine = (group[0].get("race") or "").upper() == ai_side
                colr = SIDE_A if mine else SIDE_B
                ftr = [g for g in group if (g.get("mission") or "").lower() == "manned"]
                oth = [g for g in group if (g.get("mission") or "").lower() != "manned"]
                r = 5 + min(3, len(group) - 1)
                # fighters: filled triangle; shuttle missions (WW/SS/SP): hollow
                if ftr:
                    c.create_polygon(sx, sy - r, sx - r, sy + r, sx + r, sy + r,
                                     fill=colr, outline="")
                if oth:
                    c.create_polygon(sx, sy - r - 2, sx - r - 2, sy + r,
                                     sx + r + 2, sy + r,
                                     fill="", outline=GOLD, width=1)
                types = {}
                for g in group:
                    t = g.get("shuttle_type") or g.get("mission") or "?"
                    types[t] = types.get(t, 0) + 1
                txt = " ".join(f"{n}x{t}" for t, n in sorted(types.items()))
                dmged = sum(1 for g in group if g.get("damage_taken"))
                if dmged:
                    txt += f" ({dmged} dmg)"
                c.create_text(sx, sy + r + 7, text=txt, fill=colr,
                              font=("Consolas", 7))

        # ---- ships
        for s in ships:
            cx, cy = hex_center(s["x"], s["y"], size, ox, oy)
            self._positions[s["label"]] = (cx, cy)
            mine = s["race"].upper() == ai_side
            colr = SIDE_A if mine else SIDE_B
            selected = (s["label"] == self.sel)

            # shield ring: one arc per facing, coloured by remaining strength
            sh = s.get("shields") or [0] * 6
            shm = s.get("shields_max") or [1] * 6
            for i in range(6):
                frac = (sh[i] / shm[i]) if shm[i] else 0
                sc = RED if frac <= 0 else (ORANGE if frac < 0.34 else
                                            (GOLD if frac < 0.7 else GREEN))
                # shield i is relative to the ship's facing; facing 0 shield = its bow
                a0 = facing_angle(s.get("facing", 0)) + 60 * i - 30
                c.create_arc(cx - size * 0.82, cy - size * 0.82,
                             cx + size * 0.82, cy + size * 0.82,
                             start=-a0 - 60, extent=56, style="arc", outline=sc, width=3)

            # Faction silhouette, oriented to the ship's facing. Nose direction is
            # the same datum the engine uses for arcs and shields, so a bearing
            # error shows up here as a visibly wrong-pointing ship.
            SIL.draw_ship(c, cx, cy, size * 0.72, s.get("race", ""),
                          s.get("type", ""), s.get("facing", 0), selected=selected)
            # a short heading tick beyond the nose, for unambiguous facing at low zoom
            ang = math.radians(facing_angle(s.get("facing", 0)))
            c.create_line(cx + math.cos(ang) * size * 0.80,
                          cy + math.sin(ang) * size * 0.80,
                          cx + math.cos(ang) * size * 1.05,
                          cy + math.sin(ang) * size * 1.05,
                          fill=colr, width=2)

            # ADVISED MOVE as an arrow: the engine's order, drawn where it acts.
            # Destination comes from the same kinematic model the referee uses
            # (SHADOW.apply_move), so the arrow IS the order, not a repaint.
            mv = s.get("advised_move")
            if mv and mv != "HOLD":
                try:
                    probe = {"x": s["x"], "y": s["y"],
                             "facing": int(s.get("facing", 0) or 0)}
                    SHADOW.apply_move(probe, mv)
                    dx, dy = hex_center(probe["x"], probe["y"], size, ox, oy)
                    c.create_line(cx, cy, dx, dy, fill=GOLD, width=3,
                                  arrow="last", arrowshape=(12, 14, 5),
                                  dash=(6, 3))
                    c.create_text((cx + dx) / 2 + 8, (cy + dy) / 2 - 8,
                                  text=mv.replace("_", " ").lower(),
                                  fill=GOLD, font=("Consolas", 7, "italic"),
                                  anchor="w")
                except Exception:
                    pass
            elif mv == "HOLD":
                c.create_oval(cx - size * 0.95, cy - size * 0.95,
                              cx + size * 0.95, cy + size * 0.95,
                              outline=GOLD, dash=(3, 4), width=2)

            # HUNT order: a thin red dashed line to the target whose down shield
            # this ship is manoeuvring to open (the hole itself is the red arc
            # already drawn on the target's shield ring).
            hunt = s.get("hunt")
            if hunt:
                tgt2 = next((z for z in ships if z["label"] == hunt[0]), None)
                if tgt2:
                    tx2, ty2 = hex_center(tgt2["x"], tgt2["y"], size, ox, oy)
                    c.create_line(cx, cy, tx2, ty2, fill=RED, width=1,
                                  dash=(2, 5))
                    c.create_text((cx + tx2) / 2, (cy + ty2) / 2 + 10,
                                  text=f"hunt #{hunt[1] + 1}", fill=RED,
                                  font=("Consolas", 7, "italic"))

            # forward firing arc wedge
            if self.show["arcs"].get() and selected:
                fa = facing_angle(s.get("facing", 0))
                c.create_arc(cx - size * 3.4, cy - size * 3.4, cx + size * 3.4, cy + size * 3.4,
                             start=-fa - 90, extent=180, style="pieslice",
                             outline="", fill="#12202c", stipple="gray12")

            if self.show["labels"].get():
                c.create_text(cx, cy + size * 0.95, text=s["label"], fill=colr,
                              font=("Consolas", 8, "bold" if selected else "normal"))
                c.create_text(cx, cy + size * 1.28, text=f'spd {s.get("speed",0)}',
                              fill=MUTED, font=("Consolas", 7))

        # Do NOT set scrollregion from bbox("all"): the range rings extend well
        # above the anchor ship into negative canvas coordinates, and Tk shifts the
        # view to bring the scrollregion into frame - silently pushing ships at the
        # bottom of the board out of sight. There are no scrollbars here; the view
        # must stay pinned at (0,0) and the auto-fit is what keeps everything in.
        c.configure(scrollregion=(0, 0, c.winfo_width(), c.winfo_height()))
        c.xview_moveto(0)
        c.yview_moveto(0)
        if os.environ.get("SFB_DEBUG"):
            print(f"[board] canvas={c.winfo_width()}x{c.winfo_height()} size={size:.1f} "
                  f"c0..c1={c0}..{c1} r0..r1={r0}..{r1} ox={ox:.0f} oy={oy:.0f} "
                  f"ships={len(ships)}", flush=True)
            for lbl, (px, py) in self._positions.items():
                onc = 0 <= px <= c.winfo_width() and 0 <= py <= c.winfo_height()
                print(f"[board]   {lbl:<22}({px:6.0f},{py:6.0f}) {'on' if onc else 'OFF'}",
                      flush=True)


# ================================================================ TEXT PANE
class TextPane(scrolledtext.ScrolledText):
    """Shared colour-coded text widget."""

    TAGS = {
        "head":    dict(foreground=ACCENT, font=("Consolas", 12, "bold"), spacing1=8, spacing3=4),
        "sideA":   dict(foreground=SIDE_A, font=("Consolas", 10, "bold"), spacing1=6),
        "sideB":   dict(foreground=SIDE_B, font=("Consolas", 10, "bold"), spacing1=6),
        "rp":      dict(foreground=GOLD, font=("Consolas", 10, "italic")),
        "combat":  dict(foreground=ORANGE),
        "fire":    dict(foreground=ORANGE),
        "move":    dict(foreground=GREEN),
        "eaf":     dict(foreground="#a5d6ff"),
        "warn":    dict(foreground=RED, font=("Consolas", 10, "bold")),
        "posture": dict(foreground=GREEN, font=("Consolas", 10, "bold")),
        "man":     dict(foreground="#8ddb8c"),
        "seek":    dict(foreground=RED, font=("Consolas", 10, "bold")),
        "mission": dict(foreground=PURPLE),
        "trade":   dict(foreground=GOLD),
        "cyc":     dict(foreground=PURPLE, font=("Consolas", 10, "bold")),
        "doctrine": dict(foreground=TEAL, font=("Consolas", 9, "italic")),
        # The one line you actually execute this impulse - deliberately the
        # loudest thing on the screen, since everything else is context for it.
        "impulse": dict(foreground="#ffffff", background="#1f6f43",
                        font=("Consolas", 13, "bold"), spacing1=2, spacing3=6,
                        lmargin1=6, lmargin2=6),
        "impulse_hold": dict(foreground="#c9d1d9", background="#2a2f36",
                             font=("Consolas", 12, "bold"), spacing1=2, spacing3=4,
                             lmargin1=6, lmargin2=6),
        # Ship name: same weight as the order it introduces, and sitting
        # directly on top of it so the pair reads as one block.
        "shipname": dict(foreground="#ffffff", background="#243447",
                         font=("Consolas", 13, "bold"), spacing1=12, spacing3=0,
                         lmargin1=6, lmargin2=6),
        "impulse_why": dict(foreground=MUTED, font=("Consolas", 9, "italic"),
                            lmargin1=24, lmargin2=24),
        "sub":     dict(foreground=MUTED),
        "body":    dict(foreground=FG),
    }

    def __init__(self, master, **kw):
        super().__init__(master, wrap="word", bg=BG, fg=FG, insertbackground=FG,
                         font=("Consolas", 10), padx=12, pady=10, borderwidth=0, **kw)
        for name, cfg in self.TAGS.items():
            self.tag_configure(name, **cfg)
        self.configure(state="disabled")

    def set_lines(self, lines, ai_side=""):
        """Render lines, but ONLY if the content actually changed.

        The tick loop repaints every 1.5s so async LLM results can land. Blindly
        clearing and re-inserting resets the scroll position, which yanks the
        view back to the top while you are reading. Two guards: skip the repaint
        entirely when nothing changed, and preserve the scroll offset when it did.
        """
        sig = (tuple(lines), ai_side)
        if sig == getattr(self, "_last_sig", None):
            return
        self._last_sig = sig
        yview = self.yview()          # (first, last) fractions
        at_top = yview[0] <= 0.0001
        self.configure(state="normal")
        self.delete("1.0", "end")
        for ln in lines:
            self._emit(ln, ai_side)
        self.configure(state="disabled")
        # Restore the reader's position. If they were at the very top, leave it
        # there so new content is visible rather than scrolled past.
        if not at_top:
            self.yview_moveto(yview[0])

    def _emit(self, ln, ai_side):
        s = ln.lstrip()
        if ln.startswith("==="):
            self.insert("end", ln.strip("= ") + "\n", "head")
        elif ln.startswith("---"):
            tag = "sideA" if ai_side and ai_side.upper() in ln.upper() else "sideB"
            self.insert("end", ln.strip("- ") + "\n", tag)
        elif ln.startswith("~"):
            self.insert("end", ln[1:].strip() + "\n", "rp")
        elif ln.startswith("*"):
            self.insert("end", ln + "\n", "combat")
        elif ln.startswith("@@"):
            self.insert("end", "  " + ln[2:].strip() + "  " + chr(10), "shipname")
        elif s.startswith(">>>"):
            body = s[3:].strip()
            up = body.upper()
            # Grey band for anything that is deliberately NOT acting this
            # impulse, green for a positive instruction. Scanning down the
            # greens tells you everything you have to actually do.
            passive = any(k in up for k in ("HOLD", "DO NOT", "NO LAUNCH",
                                            "RELOADING", "CANNOT"))
            self.insert("end", "  " + body + "  " + chr(10),
                        "impulse_hold" if passive else "impulse")
        elif ln.startswith("          "):          # justification under an order
            self.insert("end", "    " + s + chr(10), "impulse_why")
        elif s.startswith("DOCTRINE"):
            self._warnsplit(ln, "doctrine")
        elif s.startswith(("SEEKERS", "SCREEN", "DISENGAGE")):
            self._warnsplit(ln, "seek")
        elif s.startswith("POSTURE"):
            self.insert("end", ln + "\n", "posture")
        elif s.startswith("MANEUVER"):
            self._warnsplit(ln, "man")
        elif s.startswith("MISSION"):
            self.insert("end", ln + "\n", "mission")
        elif s.startswith("TRADE"):
            self.insert("end", ln + "\n", "trade")
        elif s.startswith("DISRUPTORS"):
            self.insert("end", ln + "\n", "cyc")
        elif s.startswith("EAF"):
            self.insert("end", ln + "\n", "eaf")
        elif s.startswith("FIRE"):
            self._warnsplit(ln, "fire")
        elif s.startswith("MOVE"):
            self._warnsplit(ln, "move")
        elif ln.startswith("[ORDER]"):
            self._warnsplit("• " + ln.split("]", 1)[1].strip(), "sideA")
        elif ln.startswith("[advise]"):
            self._warnsplit("• " + ln.split("]", 1)[1].strip(), "sideB")
        else:
            self.insert("end", ln + "\n", "sub")

    def _warnsplit(self, ln, base):
        if "WARN" in ln:
            pre, w = ln.split("WARN", 1)
            self.insert("end", pre, base)
            self.insert("end", "WARN" + w + "\n", "warn")
        else:
            self.insert("end", ln + "\n", base)


# ==================================================================== THE APP
class Bridge(tk.Tk):
    def __init__(self, ai, advise, use_voice=True):
        super().__init__()
        self.ai, self.advise = ai, advise
        self.state_obj = None
        self.lines = []
        self.sel_ship = None
        self._eaf_history = {}          # {ship label: {turn: [eaf lines]}} for turn-by-turn
        self.shadow = None              # Phase-1 shadow world {label: kinematic rec}
        self.shadow_base = None         # (turn, impulse) the shadow was last seeded at
        self.shadow_moves = {}          # {label: advised MOVE} captured at the base
        self._last_mtime = 0
        self._voice_keys = {}
        # 1 = terse, 3 = normal, 0 = full (mapped to None: keep every reason).
        self.detail = tk.IntVar(value=3)

        self.title(f"SFB Fleet Bridge   -   AI: {ai}   You: {advise}")
        self.configure(bg=BG)
        self.geometry("1180x820")

        self.voice = V.VoiceEngine(enabled=use_voice)

        style = ttk.Style(self)
        try:
            style.theme_use("clam")
        except Exception:
            pass
        style.configure("TNotebook", background=BG, borderwidth=0)
        style.configure("TNotebook.Tab", background=PANEL, foreground=MUTED,
                        padding=(16, 7), font=("Segoe UI", 10))
        style.map("TNotebook.Tab", background=[("selected", BG)],
                  foreground=[("selected", ACCENT)])

        top = tk.Frame(self, bg=BG)
        top.pack(fill="x", padx=10, pady=(8, 2))
        self.status = tk.Label(top, text="waiting for battle...", bg=BG, fg=MUTED,
                               font=("Segoe UI", 10))
        self.status.pack(side="left")
        self.vstatus = tk.Label(top, text=self.voice.status, bg=BG, fg=TEAL,
                                font=("Segoe UI", 9))
        self.vstatus.pack(side="right", padx=8)
        self.usage = tk.Label(self, text="", bg=BG, fg=MUTED,
                              font=("Consolas", 8), anchor="e")
        self.usage.pack(fill="x", padx=12, pady=(0, 4), side="bottom")
        tk.Button(top, text="Refresh", command=self.refresh, bg=PANEL, fg=FG,
                  relief="flat", padx=12).pack(side="right")

        # Detail level for the order blocks. Deduplication always runs; this
        # only controls how much rationale is kept under each order, so the
        # citations stay available without the same call being restated.
        det = tk.Frame(top, bg=BG)
        det.pack(side="right", padx=10)
        tk.Label(det, text="detail", bg=BG, fg=MUTED,
                 font=("Segoe UI", 9)).pack(side="left", padx=(0, 4))
        for txt, val in (("terse", 1), ("normal", 3), ("full", 0)):
            tk.Radiobutton(det, text=txt, variable=self.detail, value=val,
                           command=lambda: self.refresh(force=True),
                           bg=BG, fg=MUTED, selectcolor=BG, activebackground=BG,
                           activeforeground=FG,
                           font=("Segoe UI", 9)).pack(side="left")

        self.nb = ttk.Notebook(self)
        self.nb.pack(fill="both", expand=True, padx=10, pady=(4, 10))

        # --- BOARD
        self.board = BoardView(self.nb, self)
        # --- FLAGSHIP: the glance view - the ~7 decisions that matter now.
        ff = tk.Frame(self.nb, bg=BG)
        self.flag_txt = TextPane(ff)
        self.flag_txt.pack(fill="both", expand=True)
        self.nb.add(ff, text="Flagship")

        self.nb.add(self.board, text="Board")

        # --- BRIDGE (with a side toggle: whose bridge are we standing on?)
        f = tk.Frame(self.nb, bg=BG)
        bbar = tk.Frame(f, bg=BG)
        bbar.pack(fill="x", padx=10, pady=(8, 2))
        tk.Label(bbar, text="BRIDGE OF:", bg=BG, fg=MUTED,
                 font=("Segoe UI", 9, "bold")).pack(side="left", padx=(0, 8))
        self.bridge_side = tk.StringVar(value=self.advise)
        for s, colr in ((self.advise, SIDE_B), (self.ai, SIDE_A)):
            tk.Radiobutton(bbar, text=s, value=s, variable=self.bridge_side,
                           command=self._render_bridge, bg=BG, fg=colr,
                           selectcolor=BG, activebackground=BG, activeforeground=colr,
                           font=("Segoe UI", 10, "bold")).pack(side="left", padx=4)
        self.flag_lbl = tk.Label(bbar, text="", bg=BG, fg=GOLD, font=("Segoe UI", 9))
        self.flag_lbl.pack(side="right")
        self.bridge_txt = TextPane(f)
        self.bridge_txt.pack(fill="both", expand=True)
        self.nb.add(f, text="Bridge")

        # --- SHIPS
        sf = tk.Frame(self.nb, bg=BG)
        left = tk.Frame(sf, bg=PANEL, width=190)
        left.pack(side="left", fill="y")
        left.pack_propagate(False)
        tk.Label(left, text="SHIPS", bg=PANEL, fg=MUTED,
                 font=("Segoe UI", 9, "bold")).pack(anchor="w", padx=10, pady=(10, 4))
        self.ship_list = tk.Listbox(left, bg=PANEL, fg=FG, selectbackground="#30363d",
                                    highlightthickness=0, borderwidth=0,
                                    font=("Consolas", 10), activestyle="none")
        self.ship_list.pack(fill="both", expand=True, padx=6, pady=6)
        self.ship_list.bind("<<ListboxSelect>>", self._on_pick)
        self.ship_nb = ttk.Notebook(sf)
        self.ship_nb.pack(side="left", fill="both", expand=True)
        self.tab_adv, self.adv_txt = self._sub_tab("Tactical")
        # EAF as a real TABLE: rows = allocation categories, columns = turns,
        # with the engine's advice for the next turn as the last column. The
        # advised free-text (reasoning) sits in a small pane below the grid.
        ef = tk.Frame(self.ship_nb, bg=BG)
        style = ttk.Style(self)
        style.configure("EAF.Treeview", background=PANEL, fieldbackground=PANEL,
                        foreground=FG, rowheight=22, font=("Consolas", 10))
        style.configure("EAF.Treeview.Heading", background=BG, foreground=ACCENT,
                        font=("Consolas", 10, "bold"))
        self.eaf_tree = ttk.Treeview(ef, show="headings", style="EAF.Treeview")
        self.eaf_tree.pack(fill="both", expand=True, padx=6, pady=(6, 0))
        self.eaf_tree.tag_configure("total", background="#243447",
                                    font=("Consolas", 10, "bold"))
        self.eaf_txt = TextPane(ef, height=9)
        self.eaf_txt.pack(fill="x", padx=0, pady=(4, 0))
        self.ship_nb.add(ef, text="EAF")
        self.tab_eaf = ef
        self.tab_ssd, self.ssd_txt = self._sub_tab("SSD")
        self.tab_crew, self.crew_txt = self._sub_tab("Bridge crew")
        self.nb.add(sf, text="Ships")

        # --- COMMS
        cf = tk.Frame(self.nb, bg=BG)
        self.comms_txt = TextPane(cf)
        self.comms_txt.pack(fill="both", expand=True)
        self.nb.add(cf, text="Comms")

        # --- REFEREE (Phase-1 shadow-state kinematics check)
        rf = tk.Frame(self.nb, bg=BG)
        self.ref_txt = TextPane(rf)
        self.ref_txt.pack(fill="both", expand=True)
        self.nb.add(rf, text="Referee")

        # Show the window IMMEDIATELY, then load. Parsing the save spawns a JVM
        # (1.5-4.5s); doing that before the first draw meant several seconds of
        # nothing on screen, which reads as "it didn't launch".
        self.status.configure(text="reading the battle...")
        self.update_idletasks()
        self.after(60, self.refresh)
        # Come to the front on launch. Without this the window can open behind
        # the game client and read as "did not start" - the app was running fine,
        # it was just buried.
        try:
            self.lift()
            self.attributes("-topmost", True)
            self.after(1200, lambda: self.attributes("-topmost", False))
            self.focus_force()
        except Exception:
            pass
        # Lay the board again once the window has actually been mapped and the
        # canvas reports its real size.
        self.after(400, self.board.redraw)
        self._tick()

    def _sub_tab(self, name):
        fr = tk.Frame(self.ship_nb, bg=BG)
        t = TextPane(fr)
        t.pack(fill="both", expand=True)
        self.ship_nb.add(fr, text=name)
        return fr, t

    # ------------------------------------------------------------- state
    @property
    def state(self):
        return self.state_obj

    def _read_state(self):
        st, path = load_live_state()
        self._cur_save = path
        return st

    def select_ship(self, label):
        """Select from a board click. Matches on the roster index, not on the
        display text - prefix matching would confuse 'CA 702' with
        'CA 702 Sorceror'."""
        self.sel_ship = label
        try:
            labels = getattr(self, "_list_labels", [])
            if label in labels:
                i = labels.index(label)
                self.ship_list.selection_clear(0, "end")
                self.ship_list.selection_set(i)
                self.ship_list.see(i)
        except Exception:
            pass
        self._render_ship()

    def _on_pick(self, _ev):
        sel = self.ship_list.curselection()
        if not sel:
            return
        labels = getattr(self, "_list_labels", [])
        if sel[0] >= len(labels):
            return
        self.sel_ship = labels[sel[0]]      # index the roster, never parse the text
        self.board.sel = self.sel_ship
        self.board.redraw()
        self._render_ship()

    def refresh(self, force=False):
        rl = reload_if_changed()
        st = self._read_state()
        if st is None or not st.get("ships"):
            # The client holds the save open mid-game, so the autosave reads as
            # a stub. The combat LOG still tracks every position and facing
            # (validated 81/81 by the replay harness) - show those rather than
            # going blind.
            self.status.configure(text="save locked by client - showing "
                                       "log-tracked positions")
            try:
                import sfb_log
                import sfb_replay as REPLAY
                clog = sfb_log.parse()
                res = REPLAY.replay(clog)
                out = [f"=== LOG-TRACKED PICTURE (turn {clog['turn']}, "
                       f"impulse {clog['impulse']}) ===",
                       "save is client-locked; positions below come from the "
                       "combat log via the replay harness (exact).", ""]
                for lab in sorted(res["ships"]):
                    r = res["ships"][lab]
                    if r["x"] is None:
                        continue
                    f = r["facing"]
                    out.append(f"  {lab}: hex ({r['x']},{r['y']})  facing "
                               f"{'ABCDEF'[f] if f is not None else '?'}")
                sh = clog.get("shields") or {}
                if sh:
                    out.append("")
                    out.append("last logged shields:")
                    for lab, v in sh.items():
                        holes = [f"#{i+1}" for i, x in enumerate(v) if x == 0]
                        out.append(f"  {lab}: {v}"
                                   + (f"   DOWN: {', '.join(holes)}" if holes else ""))
                self.bridge_txt.set_lines(out, self.ai)
            except Exception:
                pass
            return
        self.state_obj = st
        try:
            self.lines = cmd.build_commands(st, self.ai, self.advise)
            # Trim restatements before anything renders. Deduplication is
            # unconditional - the same call announced by three subsystems is
            # never wanted - while the detail level controls how much of the
            # rationale survives under each order.
            try:
                import sfb_condense as CD
                self.lines = CD.condense(self.lines, self.detail.get() or None)
            except Exception:
                pass
        except Exception:
            self.status.configure(text="error building orders")
            self.bridge_txt.set_lines(traceback.format_exc().splitlines())
            return
        note = f"  [reloaded {', '.join(rl)}]" if rl else ""
        self.status.configure(
            text=f'Turn {st["turn"]}, Impulse {st["impulse"]}   ({len(st["ships"])} ships){note}')
        self.vstatus.configure(text=self.voice.status)
        self.usage.configure(text=self.voice.usage_line)

        self._fill_ship_list()
        self.board.redraw()
        self._render_flagship()
        self._render_bridge()
        self._render_ship()
        self._render_comms()
        try:
            self._render_referee()
        except Exception:
            self.ref_txt.set_lines(["referee error:", *traceback.format_exc().splitlines()])

    def _fill_ship_list(self):
        """Rebuild the ship list ONLY when the roster actually changes.

        Two things went wrong here before: the click handler recovered the label
        by splitting the display text on whitespace (every real ship name has
        spaces, so it only ever saw the first word), and this method cleared the
        listbox on every 1.5s refresh, throwing away the selection highlight.
        Labels are now kept in a parallel list and never parsed back out of the
        display text.
        """
        ships = self.state["ships"]
        labels = [s["label"] for s in ships]
        if labels != getattr(self, "_list_labels", None):
            self._list_labels = labels
            self.ship_list.delete(0, "end")
            for s in ships:
                mark = "»" if s["race"].upper() == self.ai.upper() else " "
                self.ship_list.insert("end", f'{s["label"]} {mark}')
        if self.sel_ship not in labels:
            mine = [s for s in ships if s["race"].upper() == self.advise.upper()]
            self.sel_ship = (mine or ships)[0]["label"] if ships else None
        # Keep the highlight on whatever is actually selected.
        if self.sel_ship in labels:
            i = labels.index(self.sel_ship)
            if self.ship_list.curselection() != (i,):
                self.ship_list.selection_clear(0, "end")
                self.ship_list.selection_set(i)
                self.ship_list.see(i)

    def _ship(self, label):
        return next((s for s in self.state["ships"] if s["label"] == label), None)

    def _ship_lines(self, label):
        """Engine lines belonging to one ship, sliced out of the full order set.

        Match the ship named right after the [ORDER]/[advise] tag - NOT any name
        that merely appears in the line. The header reads
        '[advise] CW 705 Marauder ... vs KHS FF 9 @ rng 7', so the ENEMY's name
        is in it too; a plain `label in ln` grabbed every block whose target was
        this ship, which is why viewing one hull showed several.
        """
        out, grab = [], False
        for ln in self.lines:
            if ln.startswith(("[ORDER]", "[advise]")):
                after = ln.split("]", 1)[1].strip()      # text after the tag
                grab = after.startswith(label)
            elif ln.startswith("[FLIGHT]"):
                # '[FLIGHT] <carrier>: ...' - file under the home carrier, so a
                # carrier's fighters appear with it and never orphan under an
                # unrelated ship block.
                carrier = ln[len("[FLIGHT]"):].split(":", 1)[0].strip()
                grab = (carrier == label)
            elif ln.startswith(("---", "===")):
                grab = False
            if grab:
                out.append(ln)
        return out

    # ------------------------------------------------------------ renderers
    def _voiced(self, scene, fallback):
        """Cached LLM lines if ready, else the engine's own text meanwhile."""
        got = self.voice.get_or_request(scene)
        return got if got else fallback

    @staticmethod
    def _ship_blocks(lines):
        """Split engine output into {ship label: [detail lines]}.

        The previous version guessed which ship a detail line belonged to from a
        global condition, which meant the FLAGSHIP - whose block is emitted first
        - ended up with no detail at all. Detail lines are indented under their
        header, so track the current header and attach to it.
        """
        blocks, order, cur = {}, [], None
        for ln in lines:
            if ln.startswith(("[ORDER]", "[advise]")):
                body = ln.split("]", 1)[1].strip()
                cur = body.split(" (")[0].strip()      # label is before " (TYPE,"
                blocks.setdefault(cur, {"head": body, "detail": []})
                if cur not in order:
                    order.append(cur)
            elif ln.startswith(("---", "===")):
                cur = None
            elif cur and ln.startswith("    "):
                blocks[cur]["detail"].append(ln.strip())
        return blocks, order

    def _render_bridge(self):
        st = self.state
        if not st:
            return
        side = self.bridge_side.get()
        is_ai = side.upper() == self.ai.upper()
        enemy = self.ai if not is_ai else self.advise
        mine = [s for s in st["ships"] if s["race"].upper() == side.upper()]
        # Everything not on the selected side is a contact. Derived from `side`,
        # not from self.ai/self.advise, so the whole view flips with the toggle.
        foes = [s for s in st["ships"] if s["race"].upper() != side.upper()]
        if not mine:
            self.bridge_txt.set_lines([f"=== {side} ===", f"No {side} ships on the board."])
            self.flag_lbl.configure(text="")
            return

        # The brief is given FROM the flagship - it is that bridge we stand on.
        flag = cmd.flagship(mine)
        self.flag_lbl.configure(
            text=f'flagship: {flag["label"]} ({flag.get("type","?")})' if flag else "")

        blocks, _order = self._ship_blocks(self.lines)
        events = [l[2:] for l in self.lines if l.startswith("* ")]
        turn, imp = st["turn"], st["impulse"]
        log = getattr(cmd, "_last_log", None)
        try:
            import sfb_log
            log = sfb_log.restrict_to_ships(sfb_log.parse(),
                                            [s["label"] for s in st["ships"]])
        except Exception:
            log = None

        # Energy Allocation is the moment the turn is actually decided, so brief
        # differently there: plan and power, not per-impulse fiddling. It happens
        # BETWEEN turns, so "impulse 0" only catches the opening turn - after that
        # the signal is the client's own log ("X has started/finished Energy
        # Allocation"). We are in EA while a start is outstanding.
        ea_phase = imp <= 0
        if log:
            pending = set()
            for e in (log.get("events") or []):
                if e.get("kind") == "ea":
                    (pending.add if e.get("what") == "started" else pending.discard)(e["ship"])
            if pending:
                ea_phase = True
                self._ea_pending = sorted(pending)
            else:
                self._ea_pending = []

        hdr = (f'=== {side.upper()} FLAG BRIDGE'
               + (f' - {flag["label"]}' if flag else "") + " ===")
        role = ("ORDERS - the AI plays this side; execute them faithfully."
                if is_ai else "ADVICE - your call.")
        waiting = getattr(self, "_ea_pending", [])
        phase = ("ENERGY ALLOCATION - set power for the whole turn now"
                 + (f" (allocating: {', '.join(waiting)})" if waiting else "")
                 if ea_phase else
                 f"IMPULSE {imp} of 32 - act as the impulse unfolds")
        out = [hdr, role, f"PHASE: {phase}", ""]

        # ---- voiced bridge chatter, grounded in this side's own facts
        own_facts = []
        for lbl in [s["label"] for s in mine]:
            b = blocks.get(lbl)
            if b:
                own_facts.append(b["head"])
                own_facts += b["detail"][:4]
        sc = V.Scene(side=side, role="orders" if is_ai else "advice",
                     turn=turn, impulse=imp, ships=mine, facts=own_facts,
                     events=events, enemy_side=enemy, enemy_ships=foes, mode="bridge",
                     focus=flag["label"] if flag else "")
        voiced = self._voiced(sc, [])
        out += ["~ " + v for v in voiced] if voiced else ["~ (voicing the bridge...)"]
        out.append("")

        # ---- FLEET PLAN: one intent for the whole squadron
        try:
            plan = cmd.fleet_plan(side, mine, foes, st, log, turn, imp)
        except Exception:
            plan = []
        if plan:
            out.append(f"--- {side} FLEET PLAN, TURN {turn} ---")
            out += ["  PLAN " + p for p in plan]
            out.append("")

        # ---- OUR SHIPS: everything. Flagship first, and it now gets its detail.
        out.append(f"--- {side} SHIPS ({'energy allocation' if ea_phase else 'impulse orders'}) ---")
        ordered = ([flag] if flag else []) + [s for s in mine
                                              if not flag or s["label"] != flag["label"]]
        for s in ordered:
            lbl = s["label"]
            lead = "  (FLAG)" if flag and lbl == flag["label"] else ""
            b = blocks.get(lbl)
            # Ship name goes on its own bold line IMMEDIATELY above its impulse
            # order, so the two read as a single "who / what to do" block; the
            # supporting detail follows underneath.
            out.append(f'@@{lbl}{lead}' + (f'   {b["head"].split("(", 1)[1]}'
                                           if b and "(" in b["head"] else ""))
            if not b:
                out.append("    (no orders generated for this ship)")
                continue
            det = b["detail"]
            order_lines = [d for d in det if d.startswith(">>>")]
            why_lines = [d for d in det if d.startswith("      ") or
                         (det.index(d) > 0 and det[det.index(d) - 1].startswith(">>>")
                          and not d.startswith((">>>", "MISSION", "POSTURE", "TRADE", "EAF",
                                                "MOVE", "FIRE", "SHIELD", "WEASEL",
                                                "MANEUVER", "DISRUPTORS", "FIGHTERS", "HET")))]
            # 1. the executable order first
            out += order_lines
            out += ["          " + d for d in why_lines]
            # 2. then the reasoning
            rest = [d for d in det if d not in order_lines and d not in why_lines]
            if ea_phase:
                pri = [d for d in rest if d.startswith(("EAF", "MISSION", "POSTURE", "TRADE"))]
                out += ["    " + d for d in pri]
                out += ["    " + d for d in rest
                        if d not in pri and d.startswith(("DISRUPTORS", "SEEKERS", "SCREEN",
                                                          "DOCTRINE", "SHIELD", "WEASEL",
                                                          "FIGHTERS"))]
            else:
                out += ["    " + d for d in rest]
            out.append("")

        # ---- ENEMY: no orders. What he can do to us, what he has spent.
        out.append(f"--- {enemy} CONTACTS (assessment only) ---")
        for f in sorted(foes, key=lambda z: min(
                H.hex_distance((z["x"], z["y"]), (m["x"], m["y"])) for m in mine) if mine else 0):
            out.append(f'* {f["label"]} ({f.get("type","?")}, spd {f.get("speed",0)}, '
                       f'hex {f["x"]:02d}{f["y"]:02d})')
            try:
                for t in cmd.threat_assessment(f, mine, st, log, turn, imp):
                    out.append("    " + t)
            except Exception as e:
                out.append(f"    (assessment unavailable: {e})")
            out.append("")

        self.bridge_txt.set_lines(out, self.ai)

    def _render_ship(self):
        if not self.state or not self.sel_ship:
            return
        s = self._ship(self.sel_ship)
        if not s:
            return
        lines = self._ship_lines(self.sel_ship)

        # --- Tactical
        head = [f'=== {s["label"]} ({s.get("type","?")}, {s["race"]}) ===',
                f'hex {s["x"]:02d}{s["y"]:02d}   facing {cmd.FACING_VEC.get(s.get("facing",0),"?")}'
                f'   speed {s.get("speed",0)}   size class {s.get("size_class","?")}']
        tm, letter, ok = cmd.turn_mode_of(s)
        head.append(f'MANEUVER turn mode {tm} at speed {s.get("speed",0)} (category {letter})'
                    + ("" if ok else "  WARN category unreadable in save"))
        # Standing hull facts belong here, not in the per-impulse order stream -
        # an ESG ship can never use a wild weasel, which is true for the whole
        # game and was being re-announced every impulse.
        try:
            import sfb_shuttles as _SH
            _wn = _SH.weasel_standing_note(s)
            if _wn:
                head.append(_wn)
        except Exception:
            pass
        rng = cmd.disruptor_max_range(s)
        if rng:
            head.append(f'disruptor max range {rng} (Annex #8A)')
        self.adv_txt.set_lines(head + [""] + (lines or ["(no orders for this ship)"]), self.ai)

        # --- EAF for the SELECTED ship (either side). Two parts:
        #   (1) the ACTUAL per-turn allocation read straight from the client's EAF
        #       object (what the player really entered - ground truth), and
        #   (2) the engine's advice for the upcoming allocation.
        p = s.get("power") or {}
        role = "ORDER" if s["race"].upper() == self.ai.upper() else "ADVICE"

        # (1) ACTUAL allocation as a GRID: one row per category, one column per
        # turn, straight from the client EAF.
        actual = [t for t in (s.get("eaf") or [])]
        cats, seen = [], set()
        for row in actual:
            for k in row:
                if k.startswith(("Total Power", "Reserve Power Avail",
                                 "End ", "Speed Plot", "Notes")):
                    continue
                if k not in seen:
                    seen.add(k)
                    cats.append(k)
        cols = ["cat"] + [f"t{i + 1}" for i in range(len(actual))]
        tree = self.eaf_tree
        tree.delete(*tree.get_children())
        tree.configure(columns=cols)
        tree.heading("cat", text=f"{s['label']}  (avail {p.get('total', 0)})")
        tree.column("cat", width=210, anchor="w")
        for i in range(len(actual)):
            tree.heading(f"t{i + 1}", text=f"Turn {i + 1}")
            tree.column(f"t{i + 1}", width=70, anchor="e")
        for k in cats:
            vals = [k]
            for row in actual:
                v = row.get(k, "")
                vals.append(f"{v:g}" if isinstance(v, (int, float)) else str(v))
            tag = ("total",) if "Power" in k or k.startswith("Warp") else ()
            tree.insert("", "end", values=vals, tags=tag)
        if not cats:
            tree.configure(columns=("cat",))
            tree.heading("cat", text=f"{s['label']} - no EAF entered yet")
            tree.column("cat", width=400, anchor="w")

        # (2) The engine's advice for the next allocation - reasoning text.
        try:
            _foes = [z for z in (self.state or {}).get("ships", [])
                     if (z.get("race") or "").upper() != (s.get("race") or "").upper()]
            hk = cmd.compute_eaf(s, s.get("speed", 0), True, False, enemies=_foes)
            cur_eaf = hk if isinstance(hk, list) else [str(hk)]
        except Exception as e:
            cur_eaf = [f'(EAF calc unavailable: {e})']
        turn = (self.state or {}).get("turn", 1)
        impulse = (self.state or {}).get("impulse", 1)
        alloc_turn = turn + 1 if impulse >= 32 else turn
        self.eaf_txt.set_lines([f'ADVISED for turn {alloc_turn} ({role}):']
                               + cur_eaf, self.ai)

        # --- SSD. Now flags DAMAGE prominently: a summary of what is destroyed
        # up top, a marker on every damaged line, and a POWER section (engine
        # boxes were not shown at all before, so a burned-out warp engine was
        # invisible). 'apart from shields, nothing was showing' - because the only
        # non-shield damage line (HULL x/y) was unflagged and buried, and power
        # damage had no line at all.
        sh, shm = s.get("shields") or [0]*6, s.get("shields_max") or [0]*6
        hull = s.get("hull") or [0, 0]
        p = s.get("power") or {}

        damaged = []          # (name, cur, max) for everything below full
        if hull[1] and hull[0] < hull[1]:
            damaged.append(("hull", hull[0], hull[1]))
        for nm, key in (("warp", "warp"), ("impulse", "impulse"),
                        ("APR", "apr"), ("AWR", "awr")):
            cur, mx = p.get(key, 0), p.get(key + "_max", 0)
            if mx and cur < mx:
                damaged.append((nm, cur, mx))
        for fam, (cur, mx) in (s.get("weapons") or {}).items():
            if mx and cur < mx:
                damaged.append((fam, cur, mx))
        for sysname, (cur, mx) in (s.get("systems") or {}).items():
            if mx and cur < mx:
                damaged.append((sysname, cur, mx))
        down_sh = [cmd.SHIELD.get(i, i) for i in range(6) if sh[i] <= 0 and shm[i]]

        ssd = [f'=== {s["label"]} SSD ===']
        if damaged or down_sh:
            ssd.append("DAMAGE: " + "; ".join(
                [f"shield {x} DOWN" for x in down_sh]
                + [f"{nm} {cur}/{mx}" for nm, cur, mx in damaged]))
        else:
            ssd.append("DAMAGE: none - all systems intact")
        ssd.append("")

        ssd.append("SHIELDS")
        for i in range(6):
            bar = ""
            if shm[i]:
                filled = int(round(12 * max(0, sh[i]) / shm[i]))
                bar = "█" * filled + "·" * (12 - filled)
            flag = "  <== DOWN" if sh[i] <= 0 else ("  <== hit" if shm[i] and sh[i] < shm[i] else "")
            ssd.append(f'  {cmd.SHIELD.get(i,i):<10} {sh[i]:>3}/{shm[i]:<3} {bar}{flag}')

        def _line(name, cur, mx):
            return f'  {name:<16} {cur}/{mx}' + ("  <== DAMAGED" if mx and cur < mx else "")

        ssd += ["", "HULL / POWER", _line("hull", hull[0], hull[1])]
        for nm, key in (("warp", "warp"), ("impulse", "impulse"),
                        ("APR", "apr"), ("AWR", "awr")):
            mx = p.get(key + "_max", 0)
            if mx or p.get(key, 0):
                ssd.append(_line(nm, p.get(key, 0), mx or p.get(key, 0)))
        ssd += ["", "WEAPONS"]
        for fam, (cur, mx) in sorted((s.get("weapons") or {}).items()):
            ssd.append(_line(fam, cur, mx))
        ssd += ["", "SYSTEMS"]
        for sysname, (cur, mx) in sorted((s.get("systems") or {}).items()):
            ssd.append(_line(sysname, cur, mx))
        self.ssd_txt.set_lines(ssd, self.ai)

        # --- Bridge crew (LLM, per ship)
        st = self.state
        sc = V.Scene(side=s["race"], role="orders" if s["race"].upper() == self.ai.upper()
                     else "advice", turn=st["turn"], impulse=st["impulse"], ships=[s],
                     facts=lines, events=[l[2:] for l in self.lines if l.startswith("* ")],
                     enemy_ships=[z for z in st["ships"]
                                  if z["race"].upper() != s["race"].upper()],
                     mode="ship", focus=s["label"])
        crew = self._voiced(sc, ["(voicing the bridge...)"])
        self.crew_txt.set_lines([f'=== {s["label"]} BRIDGE ==='] + ["~ " + c for c in crew],
                                self.ai)

    def _render_comms(self):
        st = self.state
        out = []
        for side, role in ((self.ai, "orders"), (self.advise, "advice")):
            ships = [s for s in st["ships"] if s["race"].upper() == side.upper()]
            if len(ships) < 1:
                continue
            facts = [l for l in self.lines if l.startswith(("[ORDER]", "[advise]"))]
            sc = V.Scene(side=side, role=role, turn=st["turn"], impulse=st["impulse"],
                         ships=ships, facts=facts,
                         events=[l[2:] for l in self.lines if l.startswith("* ")],
                         enemy_side=self.advise if side == self.ai else self.ai,
                         mode="comms")
            voiced = self._voiced(sc, [])
            out.append(f'--- {side} FLEET COMMS ---')
            out += ["~ " + v for v in voiced] if voiced else ["(opening channel...)"]
            out.append("")
        self.comms_txt.set_lines(out, self.ai)

    def _render_flagship(self):
        """The glance view: turn clock + the ranked handful of decisions that
        matter this impulse, one line each, drawn from the same order lines the
        ship tabs show in full."""
        st = self.state
        if not st:
            return
        out = []
        if self.lines and self.lines[0].startswith("==="):
            out.append(self.lines[0])
        out.append("")
        try:
            top = cmd.flagship_summary(self.lines)
        except Exception:
            top = []
        if not top:
            out.append("(no pressing decisions - closing/manoeuvre phase)")
        else:
            out.append("--- PRIORITY DECISIONS ---")
            for side, ship, txt in top:
                tag = "[ORDER]" if side.upper() == self.ai.upper() else "[advise]"
                out.append(f"{tag} {ship}: {txt}")
        out += ["", "(full rationale per ship on the Ships tab)"]
        self.flag_txt.set_lines(out, self.ai)

    def _render_referee(self):
        """Phase-1 shadow referee: advance the shadow by dead reckoning from the
        last snapshot to the current impulse, reconcile against the client's
        actual save, and report divergences.

        The shadow is advanced by the ADVISED move for each ship (the maneuver
        the bridge recommended, captured as ship['advised_move']), applied once
        on that ship's first moving impulse of the leg. So if the order was
        followed, the prediction reconciles CLEAN; a divergence now means the
        player/AI chose a DIFFERENT move (informative) OR the geometry/impulse
        chart is wrong. A ship with no advice falls back to STRAIGHT.
        """
        st = self.state
        if not st:
            return
        out = ["=== SHADOW REFEREE (Phase 1: kinematics) ===", ""]
        ti = (st.get("turn", 1), st.get("impulse", 1))
        roster = tuple(sorted(s.get("label") for s in st.get("ships", []) if s.get("label")))
        prev_roster = tuple(sorted(self.shadow.keys())) if self.shadow else ()

        def capture_moves():
            return {s["label"]: (s.get("advised_move") or "STRAIGHT")
                    for s in st.get("ships", []) if s.get("label")}

        # (Re)seed when there is no shadow, the turn changed, the roster changed,
        # or the impulse did not advance within the turn (new turn / went back).
        reseed = (self.shadow is None or self.shadow_base is None
                  or ti[0] != self.shadow_base[0] or ti[1] <= self.shadow_base[1]
                  or roster != prev_roster)

        if reseed:
            self.shadow = SHADOW.build(st)
            self.shadow_base = ti
            self.shadow_moves = capture_moves()
            out.append(f"re-synced to client at turn {ti[0]}, impulse {ti[1]} "
                       f"({len(self.shadow)} ship(s) tracked)")
            out.append("holding this snapshot as the base; the next in-turn "
                       "impulse is checked against the ADVISED move for each ship")
        else:
            base_imp = self.shadow_base[1]
            moves = self.shadow_moves or {}
            applied = set()          # each order is one maneuver: apply it once
            for i in range(base_imp + 1, ti[1] + 1):
                step = {lab: moves.get(lab, "STRAIGHT")
                        for lab in self.shadow if lab not in applied}
                moved = [lab for lab, r in self.shadow.items()
                         if SHADOW.moves_this_impulse(r["speed"], i)]
                SHADOW.advance_impulse(self.shadow, i, step)
                applied.update(moved)
            div = SHADOW.reconcile(self.shadow, st)
            shown = sorted(set(moves.get(lab, "STRAIGHT") for lab in self.shadow))
            out.append(f"predicted turn {ti[0]}, impulses {base_imp + 1}-{ti[1]} "
                       f"using the advised moves ({', '.join(shown)}):")
            out.append("")
            if not div:
                out.append("MATCH - every ship is exactly where the ADVISED move "
                           "puts it. Orders were followed and the geometry + "
                           "impulse chart are confirmed for this leg.")
            else:
                out.append(f"{len(div)} divergence(s) "
                           f"(clean = advice followed; a mismatch = a DIFFERENT "
                           f"move was made OR a rule bug):")
                for d in div:
                    out.append(f"  {d['label']}: {d['field']}  "
                               f"predicted {d['shadow']} vs client {d['observed']}")
            # Re-anchor to ground truth and recapture advice for the next leg.
            self.shadow = SHADOW.build(st)
            self.shadow_base = ti
            self.shadow_moves = capture_moves()

        out += ["", "--- shadow state (what the referee believes) ---"]
        for lab in sorted(self.shadow):
            r = self.shadow[lab]
            out.append(f"  {lab}: hex ({r['x']},{r['y']})  "
                       f"facing {r['facing']} ({'ABCDEF'[r['facing'] % 6]})  "
                       f"speed {r['speed']}")

        # --- Phase 2: ENERGY. The client stores no live charge, so this is a
        # self-consistency / legality check on its EAF, not a position-style diff.
        out += ["", "=== ENERGY (Phase 2: EAF legality) ===", ""]
        try:
            snaps, viol = SHADOW.energy_world(st)
        except Exception:
            out += ["energy layer error:", *traceback.format_exc().splitlines()]
            snaps, viol = {}, []
        if not any(r.get("has_eaf") for r in snaps.values()):
            out.append("no EAF data in the save yet (energy allocation appears "
                       "once a turn is entered)")
        else:
            if not viol:
                out.append("CONSISTENT - every ship's EAF balances, ESG within the "
                           "5-pt cap, capacitor within capacity.")
            else:
                out.append(f"{len(viol)} legality issue(s):")
                for v in viol:
                    out.append(f"  {v['label']}: {v['field']} - {v['detail']}")
            out.append("")
            for lab in sorted(snaps):
                r = snaps[lab]
                if not r.get("has_eaf"):
                    continue
                esg = r.get("esg")
                esg_txt = ("  ESG " + "/".join(f"{g:g}" for g in esg)
                           if esg else "")
                cap = (f"  cap {r['cap_current']:g}/{r['cap_max']:g}"
                       if r.get("cap_current") is not None else "")
                bat = f"  batt-used {r['battery_used']:g}" if r.get("battery_used") else ""
                out.append(f"  {lab}: used {r['used']:g}/{r['available']:g} "
                           f"(balance {r['balance']:g}){esg_txt}{cap}{bat}")

        # --- REPLAY VALIDATION (Phase 3+): the whole logged game re-run through
        # the engine. Movement: every logged move must be reproducible by a legal
        # engine move. Combat: range vs the client's OWN logged range, damage
        # magnitude vs chart bounds, shield facing vs geometry - all at
        # replay-tracked (exact-at-fire-time) positions - plus EXACT-outcome
        # reproduction wherever the client's dice are in the log. This supersedes
        # the old current-position volley check.
        out += ["", "=== REPLAY VALIDATION (full game vs engine) ===", ""]
        try:
            import sfb_log
            import sfb_replay as REPLAY
            clog = sfb_log.parse()
            mres = REPLAY.replay(clog)
            out.append(f"movement: {mres['matched']}/{mres['checked']} logged "
                       f"moves reproduced"
                       + ("" if mres["unmatched"] else "  [CLEAN]"))
            for u in mres["unmatched"]:
                out.append(f"  XXX T{u['t']}.{u['i']} {u['ship']} [{u['kind']}]: "
                           f"{u['detail']}")
            cres = REPLAY.replay_combat(clog)
            n = len(cres["checks"])
            bad = cres["violations"]
            out.append(f"combat:   {n - len(bad)}/{n} checks passed"
                       + ("" if bad else "  [CLEAN]"))
            for c in cres["checks"]:
                if not c["ok"]:
                    out.append(f"  XXX T{c['t']}.{c['i']} {c['kind']}: {c['detail']}")
            # The most recent few volley checks, so the current exchange is
            # visible without scrolling a full-game list.
            recent = cres["checks"][-8:]
            if recent:
                out.append("")
                out.append("--- latest combat checks ---")
                for c in recent:
                    mark = "ok " if c["ok"] else "XXX"
                    out.append(f"  [{mark}] T{c['t']}.{c['i']} {c['kind']}: "
                               f"{c['detail']}")
        except Exception:
            out += ["replay validation error:", *traceback.format_exc().splitlines()]
        self.ref_txt.set_lines(out, self.ai)

    # ------------------------------------------------------------------ loop
    def _tick(self):
        try:
            save = newest_tactical_save()
            m = max((os.path.getmtime(p) for p in (save, save + ".bak")
                     if os.path.exists(p)), default=0)
            engine_changed = any(
                (lambda p: p and os.path.exists(p) and os.path.getmtime(p) > _mtimes.get(n, 0))(_src(n))
                for n in _RELOADABLE)
            if m > self._last_mtime or engine_changed:
                self._last_mtime = m
                self.refresh()
            else:
                # voice results arrive asynchronously - repaint when they land
                self._render_bridge()
                self._render_ship()
                self._render_comms()
                self.vstatus.configure(text=self.voice.status)
                self.usage.configure(text=self.voice.usage_line)
                # Re-lay the board if the canvas has been resized since the last
                # draw. The first layout happens before the window is mapped, when
                # the canvas still reports a placeholder size, so the auto-fit would
                # otherwise stay wrong and push distant ships off-screen forever.
                cw, ch = self.board.canvas.winfo_width(), self.board.canvas.winfo_height()
                if (cw, ch) != getattr(self, "_board_dims", None):
                    self._board_dims = (cw, ch)
                    self.board.redraw()
        except Exception:
            pass
        self.after(1500, self._tick)


def choose_save_dialog():
    """Ask which save to follow. Returns a path, or None if cancelled.

    Deliberately blocking and up-front: the console commits to one game and stays
    on it. Auto-selecting silently followed the client's most recent write, which
    is how a scratch test scenario came to displace a live battle.
    """
    saves = describe_saves()
    if not saves:
        return None

    root = tk.Tk()
    root.title("SFB Bridge - choose the game to follow")
    root.configure(bg=BG)
    chosen = {"path": None}

    tk.Label(root, text="Which save should the console follow?", bg=BG, fg=FG,
             font=("Segoe UI", 12, "bold")).pack(padx=14, pady=(14, 2), anchor="w")
    tk.Label(root, text="It stays on this one until you restart - it will not "
                        "switch by itself.",
             bg=BG, fg=DIM, font=("Segoe UI", 9)).pack(padx=14, pady=(0, 10), anchor="w")

    box = tk.Frame(root, bg=BG)
    box.pack(padx=14, pady=(0, 6), fill="both", expand=True)
    var = tk.IntVar(value=0)
    for i, (p, size, mt, n, turn, imp, races) in enumerate(saves):
        when = time.strftime("%H:%M:%S", time.localtime(mt))
        txt = (f"{n} ships  ({'/'.join(races)})   turn {turn}.{imp}"
               f"      {size // 1024} KB   {when}")
        tk.Radiobutton(box, text=txt, variable=var, value=i, bg=BG, fg=FG,
                       selectcolor=BG, activebackground=BG, activeforeground=FG,
                       anchor="w", justify="left", font=("Consolas", 10)).pack(
            fill="x", anchor="w")
        tk.Label(box, text=f"        {os.path.basename(p)}", bg=BG, fg=DIM,
                 font=("Consolas", 8)).pack(fill="x", anchor="w", pady=(0, 4))

    def ok():
        chosen["path"] = saves[var.get()][0]
        root.destroy()

    btns = tk.Frame(root, bg=BG)
    btns.pack(padx=14, pady=(4, 14), anchor="e")
    tk.Button(btns, text="Cancel", command=root.destroy).pack(side="right", padx=4)
    tk.Button(btns, text="Follow this game", command=ok).pack(side="right")
    root.bind("<Return>", lambda _e: ok())
    root.mainloop()
    # Guarantee the chooser's Tcl interpreter is fully torn down before Bridge
    # creates its own tk.Tk(). Two live roots in one process is the latent
    # fragility (D5): closing via the window 'X' exits mainloop WITHOUT calling
    # destroy, leaving a half-alive root that can interfere with the next one.
    try:
        root.destroy()
    except tk.TclError:
        pass
    return chosen["path"]


# A single-instance guard. Two bridge windows were appearing because nothing
# stopped a second launch: the chooser dialog takes a moment to parse the saves,
# so a double-click on the launcher (or relaunching when the first seems slow)
# started a second process, and both ran to completion.
#
# The lock is a localhost socket bound to a fixed port. A bound socket is held
# by the OS for the life of the process and released the instant it dies - so
# unlike a PID file it cannot go stale after a crash, and unlike a mutex it needs
# no cleanup. If the bind fails, another bridge already owns the port, so this
# launch bows out before it can open a window (or a duplicate chooser).
_SINGLE_INSTANCE_PORT = 52731        # arbitrary, in the private range
_instance_lock = None


def _pid_file():
    return os.path.join(os.path.dirname(os.path.abspath(__file__)), "bridge.pid")


def acquire_single_instance():
    """Try to bind the lock port. True if we got it; False if it is held.

    Plain bind, NO SO_REUSEADDR: on Windows SO_REUSEADDR lets two processes bind
    the same port at once, defeating the lock. A listening socket releases its
    port the instant the process dies, so a cleanly-closed bridge frees it at
    once; only a LIVE (or hung) bridge still holding the socket blocks a second.
    """
    global _instance_lock
    import socket
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    try:
        s.bind(("127.0.0.1", _SINGLE_INSTANCE_PORT))
        s.listen(1)
    except OSError:
        s.close()
        return False
    _instance_lock = s          # keep it alive for the process lifetime
    try:
        with open(_pid_file(), "w") as f:
            f.write(str(os.getpid()))
    except Exception:
        pass
    return True


def _terminate_prior_bridge():
    """Kill a prior bridge that still holds the lock, so THIS launch can take it.

    'Newest wins.' The old bow-out behaviour meant that after a crash or an
    unclean kill, a lingering/hung bridge process kept the lock and every new
    launch silently refused to open - the 'loads randomly' complaint. Reading
    the PID we wrote on acquire, we verify it is really a pythonw (never kill a
    reused PID that is now some other program) and terminate it.
    """
    import subprocess
    try:
        with open(_pid_file()) as f:
            pid = int(f.read().strip())
    except Exception:
        return False
    if pid == os.getpid():
        return False
    nowin = 0x08000000 if os.name == "nt" else 0
    try:
        # Verify the PID is actually a pythonw before killing it.
        out = subprocess.run(["tasklist", "/FI", f"PID eq {pid}", "/FO", "CSV", "/NH"],
                             capture_output=True, text=True, timeout=10,
                             creationflags=nowin).stdout.lower()
        if "pythonw" not in out and "python" not in out:
            return False            # PID reused by something else - do not touch
        subprocess.run(["taskkill", "/PID", str(pid), "/F"],
                       capture_output=True, timeout=10, creationflags=nowin)
        return True
    except Exception:
        return False


def ensure_single_instance():
    """Acquire the lock, terminating any prior bridge that holds it (newest wins).

    Returns True once this process owns the lock. Only returns False in the rare
    case it cannot take over even after killing the holder.
    """
    if acquire_single_instance():
        return True
    _terminate_prior_bridge()
    for _ in range(20):                 # wait for the OS to release the port
        time.sleep(0.15)
        if acquire_single_instance():
            return True
    return False


def _safe_headless_output():
    """Under pythonw there is NO console: sys.stdout / sys.stderr are None. Any
    print() then raises AttributeError('NoneType' has no 'write'), which crashes
    the process - and because the offending print sits on a conditional path (an
    error handler, a voice log, a debug line), the app appears to 'load randomly':
    it starts and runs until that path fires, then dies with no visible error.

    Redirect both streams to a log file so a stray print can never kill the GUI,
    and so real tracebacks are captured instead of vanishing. Harmless under a
    normal console run (stdout is not None there, so this does nothing)."""
    import io
    for name in ("stdout", "stderr"):
        if getattr(sys, name, None) is None:
            try:
                path = os.path.join(os.path.dirname(os.path.abspath(__file__)),
                                    "bridge.log")
                setattr(sys, name, open(path, "a", buffering=1, encoding="utf-8",
                                        errors="replace"))
            except Exception:
                setattr(sys, name, io.StringIO())     # last resort: swallow writes


def main():
    _safe_headless_output()
    ap = argparse.ArgumentParser(description="SFB Fleet Bridge - tactical display")
    ap.add_argument("--ai", default="Kzinti")
    ap.add_argument("--advise", default="Lyran")
    ap.add_argument("--no-voice", action="store_true", help="disable LLM voice")
    ap.add_argument("--save", help="follow this save file (skips the chooser)")
    ap.add_argument("--auto-save-pick", action="store_true",
                    help="restore the old behaviour: follow the newest save, "
                         "switching automatically as the client writes")
    ap.add_argument("--allow-multiple", action="store_true",
                    help="skip the single-instance guard (for side-by-side testing)")
    a = ap.parse_args()

    # NEWEST WINS. A relaunch TAKES OVER from any prior bridge instead of bowing
    # out. The old bow-out meant that after a crash or unclean kill, a lingering
    # process kept the lock and new launches silently refused to open - which is
    # the 'loads randomly' behaviour. Now double-clicking the launcher always
    # opens a fresh bridge and closes the stale one. --allow-multiple skips the
    # guard entirely for deliberate side-by-side runs.
    if not a.allow_multiple and not ensure_single_instance():
        return          # could not take over even after killing the holder (rare)

    if a.save:
        pin_save(a.save)
    elif not a.auto_save_pick:
        p = choose_save_dialog()
        if not p:
            return            # cancelled, or nothing worth following
        pin_save(p)

    Bridge(a.ai, a.advise, use_voice=not a.no_voice).mainloop()


if __name__ == "__main__":
    main()
