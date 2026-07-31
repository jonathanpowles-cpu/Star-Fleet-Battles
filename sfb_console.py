"""
SFB Fleet Command Console - a standalone window that shows the AI's orders and
advice for the current battle, read from the client's local tactical autosave.

Works whether you play OFFLINE (relay) or ONLINE (real sfbonline.com) - it reads the
local autosave file, not the network, so no relay or hosts redirect is needed.

    python sfb_console.py            # auto-detects the live autosave
    python sfb_console.py --ai Kzinti --advise Lyran

The window auto-refreshes whenever you act in the client (it watches the autosave's
modification time). Text wraps; orders and advice are colour-coded per ship.
"""
from __future__ import annotations
import os, sys, glob, argparse, traceback, importlib
import tkinter as tk
from tkinter import scrolledtext, font as tkfont
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import sfb_command as cmd

# Modules reloaded when their source changes, in dependency order (leaves first)
# so that sfb_command picks up a corrected rules table rather than the stale one
# bound at import. Without this, editing a chart mid-game has no effect until the
# window is restarted - which is exactly how a wrong turn-mode table survived a
# fix once already.
_RELOADABLE = ["sfb_rules", "sfb_hex", "sfb_log", "sfb_situations", "sfb_maneuver",
               "sfb_doctrine", "sfb_command"]
_mtimes = {}


def _source_path(name):
    m = sys.modules.get(name)
    return getattr(m, "__file__", None) if m else None


def reload_if_changed():
    """Reload any engine module whose file changed. Returns the names reloaded."""
    changed = []
    for name in _RELOADABLE:
        p = _source_path(name)
        if not p or not os.path.exists(p):
            continue
        try:
            mt = os.path.getmtime(p)
        except OSError:
            continue
        if _mtimes.get(name) is None:
            _mtimes[name] = mt
        elif mt > _mtimes[name]:
            _mtimes[name] = mt
            changed.append(name)
    if not changed:
        return []
    # Reload every module from the first changed one onward, so dependents rebind.
    first = min(_RELOADABLE.index(n) for n in changed)
    reloaded = []
    for name in _RELOADABLE[first:]:
        if name in sys.modules:
            try:
                importlib.reload(sys.modules[name])
                reloaded.append(name)
            except Exception:
                pass
    globals()["cmd"] = sys.modules.get("sfb_command", cmd)
    return reloaded

RESTORE = os.path.dirname(cmd.AUTOSAVE)


def newest_tactical_save():
    """Most recently modified SFB tactical autosave (any game name), so the console
    follows whatever battle you have open - offline or online."""
    cands = []
    for pat in ("game#SFB_Game*", "gameSFB_Game*", "game#SFBCadet_Game*", "gameSFBCadet_Game*"):
        for p in glob.glob(os.path.join(RESTORE, pat)):
            if p.endswith((".bak",)) or ".bak." in p:
                continue
            if "Campaign" in p or "FNE" in p:
                continue
            try:
                cands.append((os.path.getmtime(p), p))
            except OSError:
                pass
    return max(cands)[1] if cands else cmd.AUTOSAVE

BG = "#0d1117"; FG = "#c9d1d9"
COL = {
    "header":  "#f0f6fc",
    "kzinti":  "#ff7b72",   # orders - red
    "lyran":   "#79c0ff",   # advice - blue
    "sub":     "#8b949e",
    "fire":    "#ffa657",   # fire lines pop
    "warn":    "#f85149",
}


class Console(tk.Tk):
    def __init__(self, ai, advise):
        super().__init__()
        self.ai, self.advise = ai, advise
        self.title(f"SFB Fleet Command  -  AI: {ai}   You: {advise}")
        self.configure(bg=BG)
        self.geometry("760x680")
        self._last_mtime = 0
        self._auto = tk.BooleanVar(value=True)

        top = tk.Frame(self, bg=BG)
        top.pack(fill="x", padx=8, pady=(8, 4))
        self.status = tk.Label(top, text="waiting for battle...", bg=BG, fg=COL["sub"],
                               font=("Segoe UI", 10))
        self.status.pack(side="left")
        tk.Checkbutton(top, text="auto", variable=self._auto, bg=BG, fg=FG,
                       selectcolor=BG, activebackground=BG, activeforeground=FG).pack(side="right")
        tk.Button(top, text="Refresh", command=self.refresh, bg="#21262d", fg=FG,
                  activebackground="#30363d", relief="flat", padx=12).pack(side="right", padx=6)

        base = tkfont.Font(family="Consolas", size=11)
        self.txt = scrolledtext.ScrolledText(self, wrap="word", bg=BG, fg=FG,
                                             insertbackground=FG, font=base,
                                             padx=12, pady=10, borderwidth=0)
        self.txt.pack(fill="both", expand=True, padx=8, pady=(0, 8))
        for name, colr in COL.items():
            self.txt.tag_configure(name, foreground=colr)
        self.txt.tag_configure("eaf", foreground="#a5d6ff")
        self.txt.tag_configure("move", foreground="#7ee787")
        self.txt.tag_configure("rp", foreground="#e3b341", font=("Consolas", 11, "italic"))
        self.txt.tag_configure("combat", foreground="#ff9e64")
        self.txt.tag_configure("cyc", foreground="#d2a8ff", font=("Consolas", 11, "bold"))
        self.txt.tag_configure("posture", foreground="#56d364", font=("Consolas", 11, "bold"))
        self.txt.tag_configure("man", foreground="#8ddb8c")
        self.txt.tag_configure("seek", foreground="#ff6b6b", font=("Consolas", 11, "bold"))
        self.txt.tag_configure("mission", foreground="#c9a0ff")
        self.txt.tag_configure("trade", foreground="#ffd479")
        self.txt.tag_configure("doctrine", foreground="#79d9c0", font=("Consolas", 10, "italic"))
        self.txt.tag_configure("headerbold", foreground=COL["header"],
                               font=("Consolas", 13, "bold"), spacing1=6, spacing3=4)
        self.txt.tag_configure("kzhead", foreground=COL["kzinti"],
                               font=("Consolas", 11, "bold"), spacing1=8)
        self.txt.tag_configure("lyhead", foreground=COL["lyran"],
                               font=("Consolas", 11, "bold"), spacing1=8)
        self.txt.configure(state="disabled")

        self.refresh(force=True)
        self._tick()

    def _read_state(self):
        save = newest_tactical_save()
        for path in (save, save + ".bak"):
            try:
                if os.path.exists(path):
                    st = cmd.dump_state(path)
                    self._cur_save = path
                    return st
            except Exception:
                continue
        return None

    def refresh(self, force=False):
        rl = reload_if_changed()
        if rl:
            self.status.configure(text=f"engine updated ({', '.join(rl)}) - rebuilding orders")
        state = self._read_state()
        if state is None:
            self.status.configure(text="no readable battle autosave yet")
            return
        try:
            lines = cmd.build_commands(state, self.ai, self.advise)
        except Exception:
            self.status.configure(text="error building orders")
            self._render_error(traceback.format_exc())
            return
        self.status.configure(text=f"Turn {state['turn']}, Impulse {state['impulse']}   "
                                    f"({len(state['ships'])} ships)")
        self._render(lines)

    def _render(self, lines):
        self.txt.configure(state="normal")
        self.txt.delete("1.0", "end")
        for ln in lines:
            if ln.startswith("==="):
                self.txt.insert("end", ln.strip("= ") + "\n", "headerbold")
            elif ln.startswith("---") and self.ai.upper() in ln.upper():
                self.txt.insert("end", ln.strip("- ") + "\n", "kzhead")
            elif ln.startswith("---"):
                self.txt.insert("end", ln.strip("- ") + "\n", "lyhead")
            elif ln.startswith("~"):
                self.txt.insert("end", ln[1:].strip() + "\n", "rp")
            elif ln.startswith("*"):
                self.txt.insert("end", ln + "\n", "combat")
            elif ln.lstrip().startswith("MISSION"):
                self.txt.insert("end", ln + chr(10), "mission")
            elif ln.lstrip().startswith("TRADE"):
                self.txt.insert("end", ln + chr(10), "trade")
            elif ln.lstrip().startswith("DOCTRINE"):
                self._split_warn(ln, "doctrine")
            elif ln.lstrip().startswith("SCREEN"):
                self.txt.insert("end", ln + chr(10), "seek")
            elif ln.lstrip().startswith("SEEKERS"):
                self.txt.insert("end", ln + chr(10), "seek")
            elif ln.lstrip().startswith("POSTURE"):
                self.txt.insert("end", ln + "\n", "posture")
            elif ln.lstrip().startswith("MANEUVER"):
                self.txt.insert("end", ln + "\n", "man")
            elif ln.lstrip().startswith("DISRUPTORS"):
                self.txt.insert("end", ln + "\n", "cyc")
            elif ln.startswith("[ORDER]"):
                self._colorline(ln, "kzinti")
            elif ln.startswith("[advise]"):
                self._colorline(ln, "lyran")
            elif ln.lstrip().startswith("EAF"):
                self.txt.insert("end", ln + "\n", "eaf")
            elif ln.lstrip().startswith("FIRE"):
                self._split_warn(ln, "fire")
            elif ln.lstrip().startswith("MOVE"):
                self._split_warn(ln, "move")
            else:
                self.txt.insert("end", ln + "\n", "sub")
        self.txt.configure(state="disabled")

    def _colorline(self, ln, base_tag):
        # split so FIRE/WARN clauses pop
        self.txt.insert("end", "• ", base_tag)
        body = ln.split("]", 1)[1].strip()
        for chunk in body.replace("FIRE:", "\x00FIRE:").replace("WARN", "\x00WARN").split("\x00"):
            if chunk.startswith("FIRE:"):
                self.txt.insert("end", chunk, "fire")
            elif chunk.startswith("WARN"):
                self.txt.insert("end", chunk, "warn")
            else:
                self.txt.insert("end", chunk, base_tag)
        self.txt.insert("end", "\n")

    def _split_warn(self, ln, base):
        """Render a line, making any WARN clause pop red."""
        if "WARN" in ln:
            pre, warn = ln.split("WARN", 1)
            self.txt.insert("end", pre, base)
            self.txt.insert("end", "WARN" + warn + "\n", "warn")
        else:
            self.txt.insert("end", ln + "\n", base)

    def _render_error(self, msg):
        self.txt.configure(state="normal")
        self.txt.delete("1.0", "end")
        self.txt.insert("end", msg, "warn")
        self.txt.configure(state="disabled")

    def _tick(self):
        if self._auto.get():
            try:
                save = newest_tactical_save()
                m = max((os.path.getmtime(p) for p in (save, save + ".bak") if os.path.exists(p)), default=0)
                # Refresh on a new game state OR on an engine edit, so a rules fix
                # takes effect without restarting the window.
                engine_changed = any(
                    (lambda p: p and os.path.exists(p) and
                     os.path.getmtime(p) > _mtimes.get(n, 0))(_source_path(n))
                    for n in _RELOADABLE)
                if m > self._last_mtime or engine_changed:
                    self._last_mtime = m
                    self.refresh()
            except Exception:
                pass
        self.after(1500, self._tick)


def main():
    ap = argparse.ArgumentParser(description="SFB Fleet Command Console")
    ap.add_argument("--ai", default="Kzinti")
    ap.add_argument("--advise", default="Lyran")
    args = ap.parse_args()
    Console(args.ai, args.advise).mainloop()


if __name__ == "__main__":
    main()
