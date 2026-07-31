# Phase 5 draft — the executable Sequence of Play

Phases 1–3 made the shadow a *checker*: it reads the client, predicts, and
reconciles (kinematics exactly, energy and combat by legality). Phase 5 is the
step that makes it an *engine*: run the 32-impulse turn ourselves, start to end,
so the client becomes optional rather than authoritative.

This is the largest single phase. It is drafted, not started.

## The gift: the SOP is already data

We do not have to transcribe rule 6. The client ships it:

- `client_data/imp.act` — the **full** Sequence of Play, 147 steps across 5
  segments (6A Movement, 6B Impulse Activity, 6C Dogfight, 6D Direct-Fire,
  6E Postcombat), each a row `id, parent, text` with the governing rule cited.
- `client_data/decision.act` — the **99** steps that require a player *decision*
  (the rest are mechanical "resolve X"). This is the exact list of points where
  the engine must ask someone what to do.
- `tourn_imp.act` / `tourn_decision.act` — the tournament (no-terrain) variants,
  a smaller and cleaner starting target.

So the driver is a **table-walk over the client's own SOP**, not a hand-built
state machine. Each step is either *mechanical* (call a resolver) or a *decision*
(ask the decision provider). That framing is the whole design.

## Architecture

```
   for impulse in 1..32:
       for step in SOP:                      # rows of imp.act, in order
           if step.is_decision:              # in decision.act
               order = decisions.ask(step, shadow)   # advisor / default / replay
           resolve(step, shadow, order)      # mutate the shadow world
   end-of-turn: EAF entry, reload, repair
```

Three new pieces on top of the Phase 1–3 primitives:

### 1. SOP driver (`sfb_sop.py` — new)
Loads `imp.act` + `decision.act`, exposes `steps()` (ordered) with an
`is_decision` flag, and runs the impulse loop. It owns the turn/impulse clock and
dispatches each step to a **handler** keyed by rule id (`6A2.04` → move,
`6D2.04` → direct fire, `6E…` → reload). Unhandled steps are *logged and
skipped*, never silently — the skip list is the honest measure of coverage.

### 2. Handlers — where Phases 1–3 plug in
Most of the engine already exists as advice/primitives; Phase 5 wires them to
the SOP steps that trigger them:

| SOP step | handler | status |
|---|---|---|
| 6A2.02 which units move (C1.4) | `sfb_rules.moves_this_impulse` | **have it** |
| 6A2.04 move pieces | `sfb_shadow.apply_move` / `advance_impulse` | **have it (Ph1)** |
| 6A3.01 resolve ESGs | ESG field/depletion model | have model, needs mutate |
| 6A3.03 seeking-weapon impact | `sfb_seekers` | have model, needs mutate |
| 6B3 EW / lending | `sfb_ew` channel ledger | have it |
| 6D1 fire declaration | `decisions.ask` (advisor) | new wiring |
| 6D2 direct-fire fire | `sfb_resolve.resolve_volley` (seeded) | **have it (Ph3)** |
| 6D4 damage resolution | `sfb_resolve.dac_hit` + shield/hull mutate | have decoder, needs mutate |
| 6E reload / repair / DC | reload clocks (E1.50), repair | **new** |
| end-of-turn EAF entry | `compute_eaf` → energy world | have advice, needs commit |

The recurring shape of the new work is **"mutate, don't just advise"**: the ESG,
seeker, DAC, and reload logic all exist as read-only assessments; Phase 5 makes
each one *write* the shadow (drop a shield, strike a box, decrement a rack).

### 3. Decision provider (`sfb_decisions.py` — new) — the crux
Every one of the 99 decision steps needs an answer. Three sources, in order of
trust:

- **Advisor** — for the orders the bridge already computes (move, EAF, fire,
  launch), the recommended action IS the decision. This is why Phases 1.5 and 2
  captured `advised_move` and the EAF: they are the engine's decision feed.
- **Default** — most decision steps have an obvious no-op in a given scenario
  ("activate cloak?" → no, on a ship with no cloak). A scenario-scoped default
  table collapses the 99 to the handful that actually vary.
- **Replay** — for *validation*, feed decisions from the recorded log/save so
  the engine reproduces a game that already happened; any divergence is then a
  pure rules bug, not a decision difference. This is the bridge from checker to
  engine — the engine first proves it can re-play the client before it is
  trusted to play solo.

## Scope discipline — the tournament-duel subset

147 steps sounds enormous, but for the live Kzinti-vs-Lyran duel the **large
majority never fire**: black holes, gravity/nebula, cloaks, webs, displacement
devices, Andromedan PA panels, planets/atmosphere, dogfight interface, PPD,
hellbore, mines, monsters. The MINIMAL executable turn that covers *this battle*
is roughly:

    6A movement · 6A3 ESG + seeker impact · 6B3 EW · 6D direct-fire ·
    6D4 DAC/shield/hull · 6E reload + EAF entry

— on the order of **20–30 live steps**, all but the 6E/6D4 mutators already
built. The driver still *walks* the full SOP (so nothing is quietly dropped) but
only a scenario-relevant subset has handlers; every skipped step is logged, and
the skip log is the coverage report. Build handlers when a scenario fields the
system, and test against it — the same rule that governed the whole project.

## Validation — the ultimate reconcile

Phase 5 has a cleaner test than any prior phase: **run a full turn forward from a
saved start and diff the shadow's end-state against the client's actual
end-of-turn save.**

1. *Replay mode, turn-level* — decisions from the log; compare end positions,
   speeds, shields, hull, power. Exact on kinematics/energy; legality on combat.
2. *Replay mode, impulse-level* — reconcile after each impulse to localise the
   first divergence (the Phase-1 referee, now driven forward instead of
   re-seeded each read).
3. *Solo mode* — decisions from the advisor; the client is no longer needed to
   advance the turn, only to spot-check. This is the point at which "dispense
   with the client" becomes literally true for the covered subset.

## Effort & risks

- **Effort** — the driver + decision provider are modest (the SOP is data). The
  weight is in the *mutators*: DAC→box→system-loss with the knock-on effects
  (D22 energy-balance-due-to-damage, breakdowns, crippling), and the 6E reload/
  repair clocks. Call it the biggest phase so far, dominated by damage-effect
  bookkeeping rather than by the loop itself.
- **Hidden information** — simultaneous/secret steps (6A4.02 movement changes,
  6D1.02 fire decision) are secret-and-simultaneous by rule. Solo play is fine;
  replay must take them from the log, which only records what became visible.
- **Divergence is diagnosis, not failure** — the first replay will diverge. The
  value is that impulse-level reconcile points at the exact step and rule, which
  is how every real bug this project fixed was found. Expect the SOP walk to
  *surface* rules gaps, not to run clean first time.
- **Scope creep is the real risk** — the temptation is to handle steps no live
  game triggers. The skip log exists precisely to make "not yet covered" honest
  and visible instead of pretending completeness.

## Suggested first slice

`sfb_sop.py` that loads `imp.act`, walks it for one impulse, dispatches only
`6A2.04` (move) to the existing Phase-1 mutator, logs every other step as
skipped, and reconciles position against the client. That is a one-impulse
executable turn on rails — it proves the table-walk + handler-dispatch + skip-log
spine end to end, reusing Phase 1 wholesale, before any new mutator is written.
Everything else is adding handlers to a spine that already turns.
