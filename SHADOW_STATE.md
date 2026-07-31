# Shadow-state layer — design sketch

## What it is

Today the bridge is a **read-only advisor**: it reads the client's save
(`dump_state` → positions, power, EAF, shields, weapons, seeking, shuttles) and
combat log (fire, movement, ESG activations, launches), then advises. It never
resolves anything — the client rolls dice, allocates damage, enforces the
sequence of play, and moves the pieces.

The shadow-state layer gives the bridge **its own mutable copy of the game
state**, applies the actions it advises to that copy, and resolves them with the
rules the project already encodes. Two payoffs:

1. **Shadow referee (now).** Each refresh, diff the shadow against the client's
   actual save. Where they agree, the rules are right. Where they diverge, a rule
   is wrong — an automatic, continuous regression test against real play, which
   is exactly how the real bugs got found this project (hex convention, ESG
   pooling, phaser count).
2. **Standalone path (later).** Once the shadow tracks the client faithfully
   turn after turn, the client becomes optional — the shadow *is* the engine.

It is deliberately incremental: it never has to be complete to be useful. A
shadow that models only movement + energy + ESG still catches every divergence
in those systems.

## Architecture

```
   client save/log ──► dump_state ──► OBSERVED state  ┐
                                                       ├─► reconcile ─► divergence report
   SHADOW state ──► apply(action) ──► resolve ─────────┘
        ▲                │
        └── advisor orders (what we already compute)
```

Four pieces:

### 1. State model (`sfb_shadow.py` — new)
A plain dict mirroring the `dump_state` schema (so it diffs cleanly against
OBSERVED), plus the paper-tracked fields the client save omits and we already
derive: ESG charge per generator, battery discharge, deck-crew commitments,
seeker endurance clocks, phaser-capacitor charge. **We already compute all of
these** (`sfb_actions.esg_charges`, `battery_discharge_from_eaf`,
`phaser_capacitor_state`, `sfb_deckcrew`, `sfb_seekers`) — the shadow just makes
them *stored* rather than *recomputed*.

### 2. Mutation ops (`sfb_shadow.apply`)
One function per action, each already half-written as advice:
- `move(ship, kind)` — STRAIGHT/SLIP/TURN/HET. Geometry exists in `sfb_hex`
  (`forward_hex`, turn deltas); `sfb_move` already decides the order.
- `allocate(ship, eaf)` — apply an EAF row. The consumers exist (`compute_eaf`);
  this writes the result to power/ESG/shields/capacitor.
- `fire(weapon, target)` — the only genuinely new part: **dice**. See §3.
- `launch(rack|bay)`, `esg_release(radius)`, `reload`, `damage_control` — each a
  small state edit governed by rules we cite in the advice.

### 3. Resolution (`sfb_resolve.py` — new; the real new work)
The dice-and-tables layer the advisor never needed because the client did it:
- **to-hit** — phaser/disruptor/photon charts (we HAVE these, client-sourced) ×
  EW shift (`sfb_ew`) → hit/miss with a seeded RNG.
- **DAC** — the real 2d6 `ship_dac.table` (extracted, decoded) → which box.
- **seeking-weapon impact** — endurance + speed vs target (`sfb_seekers`).
- **ESG field** — depleting-pool model (already built, G23.511).
RNG must be **seedable** so a shadow run is reproducible and a divergence is
diagnosable, not a dice-luck artifact.

### 4. Reconciliation (`sfb_reconcile.py` — new)
Diff SHADOW vs OBSERVED after each client write:
- position, facing, speed — must match exactly (movement rules)
- shields, hull, power boxes — match (damage rules)
- flag any field that diverges, with the rule most likely responsible
Surfaces in a new **"Referee" tab** on the bridge: green when tracking, a diff
list when not. This is the daily driver of correctness.

## Phasing (each phase independently testable against the live client)

1. **Kinematics.** Shadow tracks position/facing/speed only. Apply the advised
   move each impulse; reconcile against the client. Proves the movement +
   even-q geometry end to end. *Almost free — the pieces exist.*
2. **Energy.** Shadow holds power/ESG/battery/capacitor; apply EAF; reconcile
   the derived fields we already read from the client EAF. *Mostly wiring the
   `*_from_eaf` functions into stored state.*
3. **Direct-fire combat.** Add `sfb_resolve` to-hit + DAC with a seeded RNG.
   Reconcile shields/hull after each logged volley. *First real new code; the
   charts are the hard part and we have them.*
4. **Seeking weapons + ESG.** Impact resolution, endurance, ESG field depletion.
   *Models exist as advice; make them mutate state.*
5. **Sequence of play.** The 32-impulse loop as an executable procedure
   (`imp.act`/`decision.act` from the client are a reference). *This is the step
   that makes it a standalone engine.*
6. **Input.** A way to ENTER actions (not just read them) — the last mile to
   dispensing with the client.

Phases 1–2 are largely assembly of existing parts and deliver the shadow-referee
value immediately. Phase 3 is the first substantial new build. Phases 5–6 are
what actually replace the client and are a distinct, larger effort.

## What is reused vs new

| reused (already in the codebase) | new |
|---|---|
| weapon charts, DAC table, turn modes | seeded RNG + resolution (`sfb_resolve`) |
| hex geometry, arcs, ranges (validated) | stored mutable state (`sfb_shadow`) |
| energy/ESG/battery/capacitor derivation | reconciliation + Referee tab |
| seeker/deck-crew/damage models | executable sequence-of-play loop |
| master ship chart, client data files | action-input UI (phase 6) |

## Risks / open questions
- **Dice divergence is expected** — the client's actual roll ≠ our seeded roll,
  so combat reconciliation must compare *distributions/legality*, not exact
  outcomes: did the right boxes-type get hit, was the volley legal, is the total
  in range. Kinematics and energy reconcile exactly; combat reconciles soundly.
- **Hidden information** — EW, cloak, plotted movement. The shadow can only model
  what the save/log expose; some divergences will be "we can't see it," not "we
  got it wrong," and must be labelled as such.
- **Scope discipline** — the shadow is a *checker* first. Resist turning it into
  a full engine before phases 1–3 have earned trust against real games.
