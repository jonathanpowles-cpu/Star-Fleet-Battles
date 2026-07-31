# SFB Online AI Environment

Drives the **existing SFU Online Client** with an AI, instead of maintaining a
bespoke UI. The home-grown C++ interface (`src/`, `include/`) is **paused, not
discontinued** — its tactical/rules knowledge has been ported here.

## Pieces

| File | Role |
|---|---|
| `sfb_relay.py` | Offline pastiche server. SFU client + bot both connect to `127.0.0.1:6668`; brokers messages, keeps each room's object store, logs traffic. |
| `sfb_client.py` | Headless driver. Login/join, tracks decoded game state (`Piece` with `xy`/`facing`/`speed`/`Label`/`race`/`SSD`), issues moves. Codec: values are `base64(gzip(typed-JSON))`. |
| `sfb_rules.py` | Combat engine — weapon damage tables (incl. dice-resolved), arcs, turn-mode chart, power/EAF model, DAC. Ported from `include/weapons.h`, `include/ship.h`. |
| `sfb_hex.py` | Hex geometry — online offset `{x,y}` ⇄ cube; distance/bearing/arc. Facing A–F calibrated from live capture. |
| `sfb_ssd.py` | Parses the SSD blob → per-facing shields, hull, weapon list. Shields fully reliable (kind 26); weapons best-effort by kind. |
| `sfb_tactics.py` | Deterministic AI ported from `src/main.cpp` — racial profiles, target selection, posture→heading/speed, EAF pipeline. |
| `sfb_brain.py` | Claude bridge (faction personalities) + `make_brain()` / `advisor_brain()` plugging into the driver. Claude optional; deterministic fallback. |
| `docs/sfb_mechanics_reference.md` | 553-line SFB rules reference (the AI's knowledge base). |

### Sophisticated tactical layer (from the Captain's Tactics Manual, ADB5703)

| File | Role |
|---|---|
| `sfb_doctrine.py` | Structured expert doctrine — energy management, the **Mizia Concept** (damage concentration), arc tactics, reserve-power efficiency, the combat checklist, per-race fingerprints, key timing (Impulse of Decision #25 / Truth #1). |
| `sfb_ew.py` | Electronic-warfare model — square-law ECM/ECCM shifts (1/4/9 pts → +1/+2/+3), degradation by weapon family, WW/EM/ECM-drone. |
| `sfb_advanced.py` | The sophisticated engine — alpha-strike estimation, Mizia/shield targeting, EW decisions, overload & HET discipline, power-curve awareness, seeking-weapon defense, doctrine-ordered energy plan, and `assess()` → a full tactical brief (advice + fire plan). |

`sfb_brain.advisor_brain(nick)` plugs the whole thing into `SFBGameClient.run()` and
prints per-ship expert advice; `assess()` is equally the basis for AI control.

## Running it

1. **Redirect the client** to the relay: add `127.0.0.1  server.sfbonline.com`
   to the Windows hosts file (admin). Log in via **online mode** (not single
   player — that's local and unreachable).
2. **Start the relay**: `python sfb_relay.py`
3. **Run the bot** against the same game room:
   ```python
   from sfb_client import SFBGameClient
   from sfb_brain import make_brain
   c = SFBGameClient("127.0.0.1", 6668, "KlingonAI", "#SFB_Game1")
   c.start("<any-password>")               # relay accepts anything
   c.run(make_brain("KlingonAI", dry_run=True))   # reports plans; no moves
   ```
   Set `dry_run=False` to actually drive ships; `use_claude=True` for posture +
   bridge chatter (needs `ANTHROPIC_API_KEY`).

## Confirmed vs pending

**Confirmed & tested:** relay↔real-client handshake; state read/write; the full
tactical stack (geometry, profiles, target selection, posture, EAF, SSD shields
[36/30/24/24/24/30 off the real Bismarck], fire planning, retreat).

**Pending (needs live input):**
1. Wire actual **weapon fire + EAF submission** through the game's activity/fire
   protocol (movement speed/turn already wired; `plan.fire` lists intent).
2. Verify **B/C/E/F diagonal facings** with a live turning move (facing 0
   confirmed; diagonals derived by exact cube rotation).
3. Point the bot at a live human game room and let it own the enemy fleet.

Note: SFB Online is a virtual tabletop — it does **not** enforce rules — so the
AI must know SFB itself (which is why the rules engine is ported, not assumed).
