# Star Fleet Battles — tactical AI

An advisor and read-only referee for SFB played through the **SFU Online Client**.
The client is the board, the dice and the SSDs; this project reads its saves and
logs, works out what should happen, and tells the human. It does not play the game
itself — see "The client is authoritative" below for why that isn't a design choice.

## Running it

```
Start SFB AI.bat                        # Kzinti (AI) vs Lyran (you)
Start SFB AI.bat Klingon Federation     # AI side first, your side second
Start SFB AI (debug).bat                # same, but the console stays open on error
```

Entry point is `sfb_bridge.py` (Tkinter). On start it asks which save to follow —
deliberately, so a scratch scenario can't displace a live battle mid-game.

Java state dumps run as single-file source launches from `java/`, with the client's
own jars on the classpath:

```
java -cp "<client>/app/core.jar;<client>/app/plugins/*;<client>/app/3rdparty/*" StateDump.java <save>
```

`sfb_command.dump_state()` builds that classpath and calls it; `CLIENT` at the top of
`sfb_command.py` is the one path to change if the client moves.

## This machine (verified 2026-08-06, after the laptop migration)

- Python **3.14.6** at `%LOCALAPPDATA%\Programs\Python\Python314` — *not* the
  `C:\Python314` the .bat files try first. They fall back to PATH, so it works;
  don't "fix" the hardcode without checking both paths still resolve.
- Java **Temurin 25.0.4** on PATH. `StateDump.java` etc. run as-is.
- `tkinter` present, `sfb_bridge` imports clean.
- **Not installed:** `anthropic` (LLM voice polish — engine falls back to templated
  bridge chatter), `javaobj` (only `sfb_fne.py`, imported lazily). Install them if a
  task needs those paths; nothing else does.

## How this project works

**Rules are authoritative — never guess.** Every rulebook is on disk at
`G:\My Drive\SFB\` (Master Rulebook 2012, F&E, the MSSBs, Modules C1/R1-3/J/G3,
Cadet, ADB5703). Read the PDF, cite the rule number in the code, and say so plainly
when a value is *not* verified. Two real defects — the D3.32 shield chart and the
Annex #7G crews — came from trusting column-drifted `pdftotext` output. Provisional
numbers get flagged and measured, not trusted.

**`DEFERRED.md` is the single register.** Anything incomplete, provisional or guessed
belongs there, not only in a code comment, or it gets rediscovered the hard way. When
something is settled, fix the code *and* strike it in the register. Sweep it when
starting a session and when finishing a chunk of work.

**Build from play, not from the audit list.** Every real bug this project has fixed
came from playing and reporting — hex convention, ESG trigger, generator pooling,
phaser count — not from `RULES_AUDIT.md`. The remaining audit items are marginal,
not-in-play, or blocked. Build them *when* a game fields the relevant system, and
test against it. Do not implement blind.

## Invariants that cost real time to establish

- **Hexes are even-q**, not odd-q. A logged single-hex move is distance 1 only under
  even-q; ranges were off by one until this was corrected. `sfb_hex` is the only
  place that should know this.
- **The log's mode tag lies.** A volley tagged "Standard mode" dealt 6/hit at range 8,
  which only exists on the overload row. Infer mode from the dice arithmetic (the
  actual rolls *are* logged), never from the tag. Legality bounds use
  `sfb_resolve.volley_absolute_max` across all DMG-* rows.
- **The client is authoritative and ignores live board writes.** Proven repeatedly:
  injected moves and injected pieces are stored and served back, and the client
  discards them even on its own "Re-sync board". Setting a foreign `owner` doesn't
  help — it triggers fog-of-war and the pieces vanish. The only channels that work
  are **save-edit + reload** and **chat**.
- **Edit saves with Java, not byte surgery.** They're serialized
  `gub.plugins.game.sfb_campaign.*` objects; `ObjectInputStream`/`ObjectOutputStream`
  do all the handle bookkeeping that made hand-editing miserable. Vector elements are
  `UnitGamePieceAttributes` (not `UnitGamePiece`, which is a Swing component).
- **Fully exit the client before writing a save** — both `SFU Online Client.exe`
  processes gone, or the write is lost.

## Layout

```
sfb_*.py        45 modules at the root; sfb_bridge is the UI, sfb_shadow/sfb_sop
                the referee spine, sfb_rules/sfb_hex/sfb_resolve the engine
java/           46 single-file tools — StateDump, ExportBattle, Rebuild, Inventory
docs/           sfb_mechanics_reference.md
client_data/    machine-readable rules data extracted from sfb.jar
rules_txt/      extracted rulebook text
save_backups/   dated save backups; keep the naming convention
DEFERRED.md     the register — read this first
SHADOW_STATE.md / PHASE5.md / RULES_AUDIT.md   design + audit notes
```
