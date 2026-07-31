"""
In-character voice layer — the bridge crew, the advisor, and inter-ship comms.

The console's officer lines were pure string templates (sfb_command.bridge_brief),
which is why they repeated. This module actually calls the LLM, but does it in a
way a live tactical UI can survive:

  * a BACKGROUND THREAD does the calling, so the UI never blocks on the network
  * results are CACHED against a digest of the tactical situation, so we only
    spend a call when something materially changed (not every 1.5s refresh tick)
  * every request carries the ENGINE'S OWN computed facts, and the model is told
    to voice them, never to invent tactics or damage
  * without an API key it degrades to the templates rather than failing

Voices are drawn from sfb_crew.FACTION_CREW and the persistent named-officer
roster in crew_roster.json (sfb_names), so the same officers recur game to game.

    from sfb_voice import VoiceEngine
    v = VoiceEngine()
    v.request(scene)             # non-blocking; returns immediately
    lines = v.get(scene.key)     # None until ready, then a list of lines
"""
from __future__ import annotations
import os, json, time, hashlib, threading, queue, pathlib
from dataclasses import dataclass, field

MODEL = os.environ.get("SFB_VOICE_MODEL", "claude-sonnet-5")
MAX_TOKENS = 700
MIN_SECONDS_BETWEEN_CALLS = 4.0     # never hammer the API from a 1.5s UI tick
MAX_CALLS_PER_MINUTE = 6            # backstop: even if the situation genuinely
                                    # keeps changing, do not run away with cost
ROSTER_FILE = pathlib.Path(__file__).parent / "crew_roster.json"
USAGE_LOG = pathlib.Path(__file__).parent / "voice_usage.json"

# --------------------------------------------------------------------------
# Pricing, US dollars per MILLION tokens (input, output).
# Claude Sonnet 5 lists at $3/$15; an introductory $2/$10 runs through
# 2026-08-31, so that is what today's calls actually cost. Cache reads bill at
# ~0.1x input and cache writes at ~1.25x (5-minute TTL).
# --------------------------------------------------------------------------
PRICING = {
    "claude-sonnet-5":  (3.00, 15.00),
    "claude-opus-4-8":  (5.00, 25.00),
    "claude-opus-4-7":  (5.00, 25.00),
    "claude-haiku-4-5": (1.00,  5.00),
    "claude-fable-5":  (10.00, 50.00),
}
INTRO_PRICING = {"claude-sonnet-5": (2.00, 10.00)}   # through 2026-08-31
INTRO_ENDS = "2026-08-31"
CACHE_READ_MULT, CACHE_WRITE_MULT = 0.1, 1.25


def rates_for(model, today=None):
    """(input, output) $/Mtok, honouring the introductory window if live."""
    import datetime
    today = today or datetime.date.today().isoformat()
    if model in INTRO_PRICING and today <= INTRO_ENDS:
        return INTRO_PRICING[model]
    return PRICING.get(model, PRICING["claude-sonnet-5"])


def cost_of(usage, model):
    """Dollar cost of one response's usage object."""
    rin, rout = rates_for(model)
    g = (lambda k: getattr(usage, k, 0) or 0)
    plain = g("input_tokens")
    cache_read = g("cache_read_input_tokens")
    cache_write = g("cache_creation_input_tokens")
    out = g("output_tokens")
    return ((plain
             + cache_read * CACHE_READ_MULT
             + cache_write * CACHE_WRITE_MULT) * rin
            + out * rout) / 1_000_000


def load_api_key():
    """Same convention as sfb_brain: env var, else the Windows user environment."""
    if os.environ.get("ANTHROPIC_API_KEY"):
        return os.environ["ANTHROPIC_API_KEY"]
    try:
        import winreg
        k = winreg.OpenKey(winreg.HKEY_CURRENT_USER, r"Environment")
        val, _ = winreg.QueryValueEx(k, "ANTHROPIC_API_KEY")
        return val
    except Exception:
        return None


# --------------------------------------------------------------------------
# Faction voice direction. Kept short and concrete - long style notes make the
# model purple. Race flavour comes from sfb_crew.FACTION_CREW where available.
# --------------------------------------------------------------------------
FACTION_VOICE = {
    "FEDERATION": "Starfleet professionals. Measured, procedural, a little dry humour under "
                  "pressure. Address the captain as 'Captain'.",
    "KLINGON": "Klingon warriors. Blunt, aggressive, contemptuous of caution, hungry for the "
               "kill. Address the commander as 'Captain' or 'My Lord'.",
    "KZINTI": "Kzinti Hegemony officers - feline, disciplined, proud, faintly predatory. They "
              "speak of prey and of the hunt. Drone doctrine is their pride. Address the "
              "commander as 'Admiral'.",
    "LYRAN": "Lyran count's-fleet officers - aristocratic, aggressive, jealous of honour, "
             "fond of the ESG pounce. Address the commander as 'My Lord'.",
    "ROMULAN": "Romulan officers. Cold, patient, indirect, fond of the cloak and the ambush. "
               "Address the commander as 'Commander'.",
    "GORN": "Gorn officers. Slow, deliberate, immovable, plainspoken. Address the commander "
            "as 'Captain'.",
    "HYDRAN": "Hydran officers. Precise, methodical, fighter-minded. Address the commander "
              "as 'Captain'.",
    "THOLIAN": "Tholian officers. Alien, clipped, crystalline formality. Address the "
               "commander as 'Commander'.",
    "ORION": "Orion pirates. Opportunistic, mercenary, cheerfully amoral. Address the "
             "commander as 'Boss'.",
}
DEFAULT_VOICE = "Professional starship officers. Address the commander as 'Captain'."


def load_roster():
    try:
        return json.loads(ROSTER_FILE.read_text())
    except Exception:
        return {}


def officers_for(ship_label, roster=None):
    """Named officers for a ship, if the roster has them."""
    roster = roster if roster is not None else load_roster()
    return roster.get(ship_label) or {}


@dataclass
class Scene:
    """Everything the voice layer needs to speak one beat of the battle.

    `facts` are the ENGINE'S computed truths - ranges, shields, orders, doctrine.
    The model voices these; it must not invent new ones.
    """
    side: str                      # whose bridge we are on
    role: str                      # 'orders' (AI plays this side) or 'advice'
    turn: int
    impulse: int
    ships: list = field(default_factory=list)      # [{label, type, ...}]
    facts: list = field(default_factory=list)      # engine lines to voice
    events: list = field(default_factory=list)     # what just happened (combat log)
    enemy_side: str = ""
    enemy_ships: list = field(default_factory=list)
    mode: str = "bridge"           # 'bridge' | 'comms' | 'ship'
    focus: str = ""                # ship label when mode == 'ship'

    # Coarse range bands. Drifting 27 -> 26 is not a new tactical situation and
    # must not cost a call; crossing 16 -> 15 is.
    BANDS = ((3, "knife"), (5, "close"), (8, "overload"), (15, "medium"),
             (25, "long"), (99, "distant"))

    @staticmethod
    def _band(rng):
        for hi, name in Scene.BANDS:
            if rng <= hi:
                return name
        return "distant"

    @property
    def key(self):
        """Cache key = the SITUATION, deliberately coarse.

        Keying on exact positions meant every hex of movement was a fresh call -
        218 of them in one session, mostly to say the same thing about the same
        standoff. The bridge should speak when something happens that needs
        judgement, not on a timer. So the key changes only when:

          * the turn changes            (Energy Allocation - the real decision)
          * the range BAND changes      (not the exact range)
          * combat occurs               (count of fire/damage/kill events)
          * seekers appear or clear
          * a shield goes down, or a ship is lost

        Everything else reuses the cached voicing for free.
        """
        rng_band, downs, losses = "?", 0, 0
        try:
            if self.ships and self.enemy_ships:
                import sfb_hex as _H
                rng = min(_H.hex_distance((a.get("x", 0), a.get("y", 0)),
                                          (b.get("x", 0), b.get("y", 0)))
                          for a in self.ships for b in self.enemy_ships)
                rng_band = self._band(rng)
            downs = sum(1 for s in self.ships
                        for v in (s.get("shields") or []) if v <= 0)
            losses = sum(1 for s in self.ships if not (s.get("hull") or [1])[0])
        except Exception:
            pass
        combat = len([e for e in self.events
                      if any(k in e.lower() for k in ("fired", "damage", "destroyed"))])
        blob = json.dumps({
            "s": self.side, "r": self.role, "m": self.mode, "f": self.focus,
            "t": self.turn,                 # new turn = new allocation decision
            "band": rng_band,               # band, NOT exact range
            "combat": combat,               # any shooting since last time
            "seek": len([e for e in self.events if "seeker" in e.lower()]),
            "downs": downs, "losses": losses,
        }, sort_keys=True, default=str)
        return hashlib.sha1(blob.encode()).hexdigest()[:16]


SYSTEM = (
    "You voice the bridge crew of a starship in a Star Fleet Battles tactical battle. "
    "You are given the TACTICAL FACTS computed by a rules engine. Your job is to SPEAK them "
    "in character - never to invent tactics, damage, ranges, or systems that are not in the "
    "facts. If a fact says range 8 and a down #3 shield, you may dramatise that; you may not "
    "add a hull breach or a casualty that was not reported.\n\n"
    "Write SHORT exchanges - crew talking to each other and to the commander. Each line is "
    "'Station: text' or 'Name: text' when names are supplied. Keep each line under 25 words. "
    "No narration, no stage directions, no markdown. Officers may banter, disagree, or show "
    "strain, but the tactical content must match the facts exactly."
)


def _fmt_ships(ships):
    out = []
    for s in ships[:8]:
        sh = s.get("shields") or []
        down = [f"#{i+1}" for i, v in enumerate(sh) if v <= 0]
        out.append(f'  {s.get("label")} ({s.get("type","?")}) speed {s.get("speed",0)}'
                   + (f', shields DOWN: {",".join(down)}' if down else ''))
    return "\n".join(out)


def build_prompt(scene: Scene):
    roster = load_roster()
    voice = FACTION_VOICE.get(scene.side.upper(), DEFAULT_VOICE)
    named = []
    for s in scene.ships[:4]:
        o = officers_for(s.get("label", ""), roster)
        if o:
            named.append(f'  {s.get("label")}: ' +
                         ", ".join(f"{r}={n}" for r, n in o.items()))
    parts = [
        f"SIDE: {scene.side} (Turn {scene.turn}, Impulse {scene.impulse})",
        f"VOICE: {voice}",
    ]
    if named:
        parts.append("NAMED OFFICERS (use these names):\n" + "\n".join(named))
    if scene.ships:
        parts.append("OUR SHIPS:\n" + _fmt_ships(scene.ships))
    if scene.events:
        parts.append("WHAT JUST HAPPENED:\n" + "\n".join("  " + e for e in scene.events[-6:]))
    if scene.facts:
        parts.append("TACTICAL FACTS (voice these, invent nothing):\n" +
                     "\n".join("  " + f for f in scene.facts[:14]))

    if scene.mode == "comms":
        parts.append(
            "TASK: Write 3-5 lines of INTER-SHIP COMMS traffic for this side - captains "
            "talking ship-to-ship, coordinating, chivvying each other, brief banter. Format "
            "each line 'SHIPNAME: text'. If an enemy is close, you may include one line of "
            "cross-side hail or taunt, prefixed 'ENEMY SHIPNAME:'.")
    elif scene.mode == "ship" and scene.focus:
        parts.append(
            f"TASK: Write 3-5 lines from the bridge of {scene.focus} ONLY - its own officers "
            f"reacting to its own situation. Format 'Station: text' or 'Name: text'.")
    else:
        role = ("These are ORDERS the commander must execute faithfully."
                if scene.role == "orders" else
                "These are RECOMMENDATIONS; the commander decides.")
        parts.append(f"TASK: Write 4-6 lines of bridge dialogue briefing the commander. {role}")
    return "\n\n".join(parts)


def _template_fallback(scene: Scene):
    """No API key - keep the console useful rather than silent."""
    who = scene.focus or (scene.ships[0].get("label") if scene.ships else scene.side)
    out = [f"[no ANTHROPIC_API_KEY - templated voice]"]
    for f in scene.facts[:4]:
        out.append(f"{who}: {f}")
    return out


class VoiceEngine:
    """Threaded, cached LLM voice. Safe to call from a Tk mainloop."""

    def __init__(self, model=MODEL, enabled=True):
        self.model = model
        self.enabled = enabled
        self._cache = {}
        self._pending = set()
        self._q = queue.Queue()
        self._lock = threading.Lock()
        self._last_call = 0.0
        self._key = load_api_key()
        self._client = None
        self._err = None
        # Running usage/cost for this session, plus a persisted lifetime total
        # so cost is visible across restarts rather than resetting each launch.
        self.calls = 0
        self.tok_in = self.tok_out = self.tok_cache_read = self.tok_cache_write = 0
        self.cost = 0.0
        self.lifetime = self._load_usage()
        if self.enabled and self._key:
            try:
                import anthropic
                self._client = anthropic.Anthropic(api_key=self._key)
            except Exception as e:
                self._err = f"anthropic SDK unavailable: {e}"
        elif not self._key:
            self._err = "no ANTHROPIC_API_KEY found"
        self._worker = threading.Thread(target=self._run, daemon=True)
        self._worker.start()

    # ------------------------------------------------------------ usage
    def _load_usage(self):
        try:
            return json.loads(USAGE_LOG.read_text())
        except Exception:
            return {"calls": 0, "input": 0, "output": 0,
                    "cache_read": 0, "cache_write": 0, "cost": 0.0}

    def _save_usage(self):
        try:
            USAGE_LOG.write_text(json.dumps(self.lifetime, indent=1))
        except Exception:
            pass

    def _record(self, usage):
        c = cost_of(usage, self.model)
        g = (lambda k: getattr(usage, k, 0) or 0)
        self.calls += 1
        self.tok_in += g("input_tokens")
        self.tok_out += g("output_tokens")
        self.tok_cache_read += g("cache_read_input_tokens")
        self.tok_cache_write += g("cache_creation_input_tokens")
        self.cost += c
        lt = self.lifetime
        lt["calls"] = lt.get("calls", 0) + 1
        lt["input"] = lt.get("input", 0) + g("input_tokens")
        lt["output"] = lt.get("output", 0) + g("output_tokens")
        lt["cache_read"] = lt.get("cache_read", 0) + g("cache_read_input_tokens")
        lt["cache_write"] = lt.get("cache_write", 0) + g("cache_creation_input_tokens")
        lt["cost"] = lt.get("cost", 0.0) + c
        self._save_usage()
        return c

    @property
    def usage_line(self):
        """One-line usage/cost summary for the UI."""
        if not self._client:
            return ""
        rin, rout = rates_for(self.model)
        intro = " intro" if self.model in INTRO_PRICING and rin < PRICING[self.model][0] else ""
        thr = "  THROTTLED" if getattr(self, "_throttled", False) else ""
        return (f"{self.calls} calls{thr}  {self.tok_in:,}in/{self.tok_out:,}out  "
                f"${self.cost:.4f} this session  "
                f"(${self.lifetime.get('cost', 0.0):.2f} lifetime)  "
                f"@ ${rin:g}/${rout:g} per Mtok{intro}")

    @property
    def status(self):
        if not self.enabled:
            return "voice off"
        if self._err:
            return self._err
        return f"voice on ({self.model})"

    def request(self, scene: Scene):
        """Queue a scene for voicing. Non-blocking; returns the cache key."""
        k = scene.key
        with self._lock:
            if k in self._cache or k in self._pending:
                return k
            # Backstop rate limit. The situation key should already keep calls
            # rare; this catches a pathological case (e.g. a running fight where
            # something changes every impulse) before it costs real money.
            now = time.time()
            self._recent = [t for t in getattr(self, "_recent", []) if now - t < 60]
            if len(self._recent) >= MAX_CALLS_PER_MINUTE:
                self._throttled = True
                return k
            self._recent.append(now)
            self._throttled = False
            self._pending.add(k)
        self._q.put(scene)
        return k

    def get(self, key):
        """Voiced lines for a key, or None if not ready yet."""
        with self._lock:
            return self._cache.get(key)

    def get_or_request(self, scene: Scene):
        """Convenience: return lines if cached, else queue and return None."""
        with self._lock:
            hit = self._cache.get(scene.key)
        if hit is not None:
            return hit
        self.request(scene)
        return None

    def _run(self):
        while True:
            scene = self._q.get()
            try:
                lines = self._voice(scene)
            except Exception as e:
                lines = [f"[voice error: {e}]"]
            with self._lock:
                self._cache[scene.key] = lines
                self._pending.discard(scene.key)
                # keep the cache from growing without bound over a long game
                if len(self._cache) > 400:
                    for k in list(self._cache)[:100]:
                        self._cache.pop(k, None)

    def _voice(self, scene: Scene):
        if not self._client:
            return _template_fallback(scene)
        # gentle rate limit so a fast UI tick cannot spam the API
        gap = time.time() - self._last_call
        if gap < MIN_SECONDS_BETWEEN_CALLS:
            time.sleep(MIN_SECONDS_BETWEEN_CALLS - gap)
        self._last_call = time.time()
        msg = self._client.messages.create(
            model=self.model, max_tokens=MAX_TOKENS, system=SYSTEM,
            messages=[{"role": "user", "content": build_prompt(scene)}])
        try:
            self._record(msg.usage)
        except Exception:
            pass
        # Responses may lead with a thinking block, so take the first TEXT block
        # rather than content[0].
        text = ""
        for block in msg.content:
            if getattr(block, "type", None) == "text" or hasattr(block, "text"):
                if getattr(block, "type", None) == "thinking":
                    continue
                text = getattr(block, "text", "") or ""
                if text.strip():
                    break
        text = text.strip()
        if not text:
            return ["[voice returned no text]"]
        return [ln.strip() for ln in text.splitlines() if ln.strip()]


if __name__ == "__main__":
    v = VoiceEngine()
    print("status:", v.status)
    sc = Scene(side="Lyran", role="advice", turn=2, impulse=12,
               ships=[{"label": "Kharg", "type": "CA", "speed": 16,
                       "shields": [20, 0, 18, 20, 20, 20]}],
               facts=["Kharg: range 8 to Mystic, his #2 shield faces us.",
                      "POSTURE CLOSE - optimal band 8 for disruptors.",
                      "MANEUVER: turn mode 4 at speed 16; 2 hexes since last turn.",
                      "WARN Lyran ESG has no effect on plasma or direct fire (G23.81)."],
               events=["T2.10: Mystic fired 4 phaser-1 at Kharg", "T2.10: Kharg took 11 damage (#2)"])
    k = v.request(sc)
    for _ in range(60):
        r = v.get(k)
        if r:
            print("\n".join(r)); break
        time.sleep(0.5)
    else:
        print("(timed out)")
