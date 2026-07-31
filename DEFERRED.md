# Deferred work & standing caveats

The single register of things known to be incomplete, provisional, or
guessed. When something here is settled, fix it in code **and** strike it here.
The rule of the project: never let a flag live only in a code comment — it
belongs here too, or it will be rediscovered the hard way.

Last swept: turn 1 of the live Kzinti-vs-Lyran carrier game (impulse ~21).

---

## A. Numbers that are provisional (would change advice if wrong)

| id | where | what's uncertain | how to settle |
|----|-------|------------------|---------------|
| A2 | `sfb_seekers.PLASMA_REDUCTION_BANDS` | Plasma decay bands are read from `weapons.chart` but the **indexing is unverified** — distance-flown vs range-to-target. Launch strengths (R=50 etc.) are solid. | A game with a plasma race, or a clean read of the FP1.53 table image. No plasma in the current battle, so low urgency. |
| A3 | `sfb_carrier.FIGHTER_SPEED_ASSUMED = 8` | Fallback fighter speed when the airframe isn't in the save. Now largely mooted by reading `max_speed` per craft from the board, but still the fallback. | Annex #4 (G3 pp.121-144) via `tools/pdf_columns.py`, or just trust the board reading. |
| A4 | `sfb_deckcrew` — Gorn | Vudar now recovered (6 hulls, validated). GORN still unextracted: its page-153 block tears hull labels from data rows, so the splitter concatenates them. Absence must NOT be read as "2 crews". Low priority - Gorn not in current play; safe fallback flags UNKNOWN. | Hand-read the Gorn block from the G3 SSDs, or a bespoke splitter for that layout. |
| A7 | `_is_crippled` warp basis | S2.41 needs 10% of ORIGINAL warp; the save exposes current undestroyed warp (`warp_max`), used as proxy. Exact only for undamaged-warp ships. |
| A5 | Annex #7G — 5 flagged rows | Romulan SNV, Orion CSV/LVS and two others: printed DC disagrees with fighter count. May be real annex exceptions, not extraction errors. | Kept in `annex7g_deck_crews.json` `flagged`. Check against printed SSDs. |

## B. Open questions needing a live game or external data (from the audit)

| id | question | blocked on |
|----|----------|-----------|
| B2 | GR vs hellbores crossover — at what enemy hellbore count does general reinforcement beat specific. | A Hydran game. |
| B3 | Weasel interaction budget — should the engine *suppress* a conflicting drone order or merely *flag* it. | Playtest preference. |
| B4 | Plasma bolt-vs-launch posture (Glory Zone) — default `preferred_range` when the mode is unknown. | Playstyle call. |
| B5 | DAC-weighted `commitment()` — does the real 2d6 DAC improve the engage/avoid verdict or just add variance. | A/B comparison in play. |
| B6 | Sensor track length per ship (EW4 / audit Q9) — client ships no per-ship sensor-box table. | SSDs. |
| B7 | Shuttle boxes differentiated per ship (audit Q10). | SSDs. |

## C. Citations to firm up (tactic sound, rule number loose)

| id | where | issue |
|----|-------|-------|
| C2 | `sfb_maneuver` | Tactics-Manual extraction reads ambiguously on which ships may HET/erratic; AMBIGUITY FLAGGED in place. |

## D. Code-shape debt (works, but not how it should end up)

| id | where | note |
|----|-------|------|
| D2 | `sfb_actions.ESG_CHART` x6/x7 columns | On the client chart but exceed the G23.22 five-point cap. Recorded as printed; the rule for when they apply (stacked generators?) is not modelled. Low priority - no ship in play has >5-point ESGs. |
| D3 | `closing_rate` | Straight-line projection along current facings — honest current-heading truth, but ships turn, so it under/over-shoots on a turning engagement. Good enough for the deadline logic; not a trajectory solver. |
| D4 | `sfb_ew.guiding` attribution | RESOLVED the double-count: rack code and flight code now share `sfb_ew.guiding`. Residual: fighter-launched drones are attributed in the LOG to the launching fighter, not its mothership, so the mothership's `guiding` count omits them - its free channels can read high when its AAS have drones out. Deeper fix needs mapping fighter->mothership and crediting those drones to the ship (R5.F2: ship or another unit guides them; the engine can't know which, so it's an assumption either way). |
| D5 | `sfb_shadow.allocate_internals` DAC column | The DAC (`ship_dac.table`) is 11 rows (2d6) x 13 columns; the 13 COLUMN semantics are unverified. Current model: one 2d6 roll picks the row, successive internal points walk columns left-to-right, skipping destroyed box types (D4.3), fresh row on exhaustion. Destroys real boxes correctly, but which column a given hit *should* use is an assumption. Settle against a known DAC worked example. |
| D6 | `sfb_shadow.apply_energy_balance` (6A3.11) | DONE for the primary case: destroyed warp/impulse boxes now cap speed (D22 plant ceiling) and cross the S2.41 crippling thresholds (exact, via `boxes_max`), applied at 6A3.11 so prior-impulse fire bites on the next impulse (rules-correct timing). RESIDUAL: it is a PLANT ceiling, not a full energy-budget solve - other commitments (weapons, shields, EAF) are not subtracted, so it only reduces speed under real plant loss, not on a marginal energy deficit. WEAPON loss now fed back: `surviving_weapon_boxes` caps a declared volley by undestroyed mounts at 6D2.04 (reads prior-impulse damage only, so a weapon killed earlier this impulse still fires - the 6D4.02 rule). RESIDUAL: SYSTEM-box loss (sensor->EW, shuttle->launch, tractor) is tracked in the inventory but has no consumer yet - those effects attach when the EW / shuttle / seeker handlers are registered on the spine (no live handler reads systems today). Still needs the fuller energy-budget solve for the marginal-deficit speed case. |
| D8 | `sfb_shadow` seekers (6A2.04 / 6A3.01 / 6A3.03) | DONE for the drone case, correctly ordered: seekers MOVE in 6A2.04 (`advance_seekers`, C1.31 - like any piece, recording `prev` for approach bearing), the ESG engages them in 6A3.01 and impact detonates in 6A3.03, both on POST-move positions. Warhead -> facing shield -> internals immediately (6A3 step-by-step). Anti-drone in the engine: phaser (6D2.04 volley targeting a seeker accumulates toward kill value 4 std / 6 heavy, FD1.51 unpenalised) and ESG (6A3.01 `resolve_esg_vs_seekers`, a G23.511 depleting pool = combined field at radius, G23.512 same-ship pooling, spends each drone's kill value till exhausted). The earlier pre-move ESG ordering artifact is FIXED (movement lifted to 6A2). FLAGGED remaining: (b) no re-targeting when the target moves off its hex, no endurance expiry (sfb_seekers.expiry exists but is unconsulted) - a seeker that misses just keeps homing; (c) plasma warhead coarse and plasma is neither phaser- nor ESG-killed here (no plasma in play); (d) no drone-guidance / control-channel limits in solo play. |
| D9 | `sfb_shadow` reload/repair + drone launch | Reload/repair runs at 6E3 on impulse 32 (turn boundary; the impulse procedure has no reload step). DONE: phaser capacitor refill (H6.1, topped to max - the next EAF pays, flagged); drone-rack rate-of-fire reset. Drone LAUNCH now in the engine (6B06.05 `handle_drone_launch` / `launch_drone`): racks built from the save (`drone_racks`, ADD boxes excluded as anti-drone), each with ammo + rate-of-fire (type-A/B 1/turn gap 8, type-C 2/turn gap 12 - `rack_kind`); a launched drone becomes a seeker homing from 6A2 of the next impulse and runs the full fly->impact->DAC loop. FLAGGED: launched-drone speed fixed at 8 (speed modules S/M/F=12/20/32 not read from the round); the 8/12-impulse gap resets at turn boundary (cross-turn gap per FD3.0 not enforced); damage-control repair (D9) still unmodelled (most battle damage is not repairable in-scenario anyway). |
| D7 | `sfb_shadow._box_inventory` control spaces | Only systems the save enumerates as counts are trackable (hull, power, weapons, shuttle). Control spaces (bridge, sensor, lab, cargo, damage-control) are NOT counted, so a DAC hit on one is recorded as an "untracked" hit rather than depleting a box. Real boxes, just not depletable in the shadow - fine for damage totals, wrong for "is the bridge gone". Needs a full SSD box list per hull. |

## E. Remaining implementation schedule (from RULES_AUDIT.md)

- **Batch 5 — SWEPT and CLOSED.** All items resolved or dismissed with reasons:
  E1 retracted (engine was right), E2 dead code, E3 blocked on charge data, M3
  correct (citation fixed), SC1/SC4 fixed. Remaining M1/M2/M4/M5 VERIFIED
  marginal: M1 slip-mode is an OPTIONAL advanced rule (C4.0) standard play does
  not use; M2 EM already modelled in `em_assessment` (EM does not change the
  C3.31 turn mode); M4 speed-0 already handled; M5 reverse/decel parsing is
  low-value (rare, not in play). None is a standard-play bug - do NOT implement
  blind.
- **Batch 6/7 — remaining, but NOT relevant to the current battle:**
  - Chaff (audit's "sharpest gap") - VERIFIED no ship in the Kzinti/Lyran game
    carries it; build when a chaff ship is fielded, and test against it.
  - Enveloping hellbores - no hellbore ship in play.
  - Per-airframe rearming state machine - depends on F1 (done); real but complex,
    low current value.
  - Doctrine postures (saber-dance, Glory Zone) - refinements; Glory Zone is
    plasma (not in play).
  - Weapon Status as a scenario input - the mechanism now exists (`ESG_INITIAL_CHARGE`,
    `ESG_CHARGE_OVERRIDE`, `BATTERY_DISCHARGE` overrides); generalise it when a
    scenario actually sets a WS.

  THE PATTERN: every real bug this project fixed came from PLAYING and reporting
  (hex convention, ESG trigger, generator pooling, phaser count), not from the
  audit list. The remaining audit items are marginal, not-in-play, or blocked.
  Build them WHEN a game fields the relevant system, and test against it - not blind.

---

### Recently settled (compact roll-up)

All verified fixes this project, condensed. Detail is in git history and code comments.

**Rules/data corrected:** shield chart (measured on client, audit E1 retracted); turn-mode
chart (validated vs client); hex convention odd-q -> even-q (ranges were off by one);
Annex #7G deck crews (355 carriers + Vudar extracted); disruptor use-or-lose deadline;
E1.50 per-mount reload; SC1 disengagement min(50%,15); SC4 crippled = S2.41 (10% warp).

**ESG (from player EAF):** trigger on inbound seekers not ship range; G23.31 announce lead;
G23.511 depleting-pool radius model; per-generator charges + G23.512 pooling (was halving
CW field); initial charge 0 (scenario had no WS); client field-strength chart.

**Anti-drone:** fighter-drone launches (AAS controlled by mothership, R5.F2); rack-level
targeting; F3.21 control-channel gating; one shared channel ledger; drone-wave OUTCOME
synthesis (leakers/damage/internals) + fixed the phaser-count bug (split-phaser ships read 0).

**Infrastructure:** battery-discharge tracking; save-pin by game identity + stub recovery;
single-instance lock -> NEWEST-WINS takeover (fixed 'loads randomly'); `_safe_headless_output`
(pythonw stderr); scenario-start charge overrides; abbreviated-vs-full ship-name matching
now CENTRAL (`sfb_log.canonical_label` + `restrict_to_ships` canonicalises ALL event kinds
to the full board label - previously only the ESG path matched, so fire/damage/launch/maneuver
events for hull-numbered ships were silently dropped by exact matching).

**Data facts proven:** energy charge is NOT in the save (boxStatus is damage state) - needs a
client read; `deck_crews_reported` is a has-capability flag, not a count (D1 resolved).

**Shadow-state engine (SHADOW_STATE.md / PHASE5.md):** read-only referee built in phases -
Ph1 kinematics (`sfb_shadow`, even-q apply_move + reconcile, exact), Ph1.5 advised-move feed
(`ship['advised_move']`), Ph2 energy (EAF legality: balance/ESG-cap/capacitor), Ph3 combat
(`sfb_resolve` seeded Roller + chart bounds + DAC decoder; volley magnitude/facing legality).
Surfaced in the bridge Referee tab. Ph5 spine (`sfb_sop`) walks the client's OWN Sequence of
Play (`imp.act`, 121 leaf steps) dispatching handlers by rule id with an honest skip-log;
handlers registered so far: 6A2.04 move (ships + seekers, C1.31), 6B06.05 drone launch (rack
ammo + rate-of-fire), 6A3.01 ESG-vs-seeker (depleting-pool field), 6A3.03 seeker impact (drones
home + detonate), 6A3.11 energy-balance/D22 (speed cap + S2.41 crippling), 6D2.04 direct-fire
(capped by surviving mounts AND phaser capacitor; volley may target a SEEKER = phaser anti-drone),
6D4.02 DAC internals, 6E3 postcombat/turn-boundary reload (impulse 32). Phaser capacitor is a LIVE
resource (fire drains H6.21, end-of-turn refills H6.1); drone racks track ammo + ROF + turn reset.
Coverage 8/121. Open items D5-D9 above.
REPLAY VALIDATED (`sfb_replay.py`): every logged movement of the live Kzinti-vs-Lyran game
(81 events, both fleets, turns/slips/HET-free mix) is reproduced by exactly a legal engine move -
even-q geometry, A-F facing model and the STRAIGHT/SLIP/TURN move set confirmed against real play.
Key log facts learned: destination labels embed facing ('1402C'); 'has turned to 2408D' is a
POSITIONAL event (turn resolves entering the hex), 'has changed to facing F' is facing-only.
EXACT-OUTCOME REPLAY working: sfb_log now parses the dice ('roll' events, RE_ROLL); replay_combat
matches each volley's 'Rolls 1d6' line by value-count, applies the actual rolls to the chart
(sfb_resolve.heavy_mode_damage, STD + OVLD rows) and reproduces the client's damage numbers
EXACTLY - T1.31 Marauder [6,1,1] -> 2x6=12 (OVLD inferred, tag said Standard), T1.32 Sabre
[2,1] -> 2x3=6 (STD inferred). Mode is now INFERRED from dice arithmetic, per the tag-lies rule.
Unpaired case: a damage event whose attacker logged no aggregate 'fires N' line (Sorcerer T1.32
had only per-mount fire_detail lines) gets no magnitude/exact check - pairing from fire_detail
is the refinement if it recurs.
COMBAT REPLAY also CLEAN (`replay_combat`, 20/20 with exact checks): every logged volley's range (our hex_distance
vs the client's own logged range - exact at replay-tracked positions), damage magnitude, shield
facing and ESG field strength are consistent with the engine's charts + geometry. Two more log
facts: (1) THE DICE ARE LOGGED ('Skylark Rolls 1d6: 6, 1, 1' = the actual to-hit rolls) - an
exact-outcome replay (not just legality) is possible by feeding logged dice into sfb_resolve;
unparsed today. (2) The fire_detail MODE TAG IS UNRELIABLE: T1.31 Marauder logged 'Standard mode'
but dealt 6/hit at range 8, which is only on DMG-OVLD (rolls 6,1,1 = 2 hits x 6 = the observed 12)
- so legality bounds use `sfb_resolve.volley_absolute_max` (max across ALL DMG-* rows), and any
future mode-dependent logic must infer mode from EAF/damage arithmetic, not the log tag.
