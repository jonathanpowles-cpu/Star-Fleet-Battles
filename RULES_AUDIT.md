# SFB Advisor — Rules Audit

**Scope:** `sfb_*.py` (~10,700 lines) audited against `rules_txt/` (Master Rulebook 2012, Module J, ADB5703 Tactics Manual).
**Engine model:** the engine does not simulate the game. A human plays all ships in the SFU Online Client; the engine reads the save file and play-by-play log and issues ORDERS for one side and ADVICE for the other. A "gap" therefore means advice that is *illegal*, *materially wrong*, or *fed stale/constant input* — not "the engine doesn't resolve X".
**SSD caveat:** the SSD books extracted to empty image scans. Nothing per-ship (BPV, sensor track length, fighter speeds, crew/BP boxes, exact shield cost rows for SC2/SC4) can be verified from the corpus. Those points are flagged inline.

---

## 1. Summary

The engine is in good structural shape: the rules coverage is broad, the citations are mostly real, and the hard geometry (hex math, arcs, turn brackets, disengagement legality, seeker geometry) is present and largely correct. The failures cluster into **three themes**, and the third is by far the most dangerous.

**Theme A — wrong numbers in otherwise-correct machinery.** The shield operation chart (D3.32) has SC3 at 2 instead of the D3.32 Total column of 4, and `MANDATORY_POWER` is a size-blind constant of 6. The `sfb_rules` phaser/disruptor/photon damage tables are fabricated and contradict the rulebook's own worked examples *and* the correctly-sourced charts sitting in `sfb_command.py` — the codebase has two disagreeing sets of weapon charts. HET is refused on impulse 1 citing a Tactical Manoeuvre restriction. The E1.50 reload lockout is right at 8 impulses but keyed per weapon *family*, so one ranging disruptor mutes all six.

**Theme B — stale or constant inputs.** This is the same class as the two bugs already fixed (fighter launches never parsed, deck crews pinned to a threshold), and it is systemic. **Enemy ECM is never parsed from anywhere** — `recommend_ew` reads it via `getattr(enemy,"ecm",0)` and no code path in the tree ever populates that attribute, so ECCM is pinned to zero for the entire game and the EW shift is always reported as +0. Battery charge is a hardcoded `batt_room = 0`. The sensor EW pool is a constant 6 that never falls with sensor damage. Fighter *landings* are never parsed, so "fighters aboard" is monotonically decreasing and any shuttle-type launch (weasel, scatter-pack) is debited from the fighter pool. `carrier_speed_limit` is fed fighter *boxes* as `fighters_out`, inverting J1.61. `drone_profile()` is called with the launching **ship**, so drone launch range keys off the firing ship's own throttle. Seeker control channels (F3.21), drone endurance, and plasma warhead decay (FP1.53) are all defined-but-unread or absent.

**Theme C — modules that disagree with each other.** There are two energy planners, two weapon-chart sets, two EW paths, and two carrier panels, and in each pair the correct implementation is on the *advisory* side while the *order-emitting* side carries the defect. `sfb_command` never imports `sfb_ew`. `sfb_tactics.plan_eaf` uses the legacy `MANDATORY_POWER` constant while `sfb_command.compute_eaf` implements the correct size-keyed formula. `sfb_doctrine` prints "fight in the plasma glory zone (r9-10)" while `sfb_tactics` steers the same ship to range 7.

Nothing found is catastrophic in isolation. But in a carrier fight the stale-input cluster (fighter counts, carrier speed cap, drone launch range, ECM) compounds into an advisor that is confidently wrong about the state of its own air group.

---

## 2. Ranked gaps

Severities are the post-verification ratings. "Playtest?" = a live game is the only way to sense-check the fix.

| # | Sev | Rule | Description | Playtest? |
|---|-----|------|-------------|-----------|
| 1 | MAJOR | D3.32 | Shield operation cost chart wrong: SC3 = 2, should be Total column 4 (SC2 = 4, should be ~5) | no |
| 2 | MAJOR | B3.3 / D6.6 / D3.32 | `MANDATORY_POWER = 6` size-blind constant; drives `sfb_tactics`/`sfb_advanced` budgets | no |
| 3 | MAJOR | H7.0 / B3.4 | `batt_room = 0` hardcoded — surplus power can never recharge discharged batteries | no |
| 4 | MAJOR | D3.343 | `reinforce_plan` targets DOWN shields (and *prefers* the weaker neighbour) | no |
| 5 | MAJOR | D3.3412 | Blanket "never use general reinforcement" — wrong vs hellbores / enveloping plasma | yes |
| 6 | MAJOR | E2.411/E3.4/E4.12 | `sfb_rules` weapon damage tables fabricated; contradict rulebook examples and `sfb_command`'s own charts | no |
| 7 | MAJOR | E3.52 / E4.413 | Overload damage flat 16 / flat 12; zero at R0-1 where overloads are legal | no |
| 8 | MAJOR | E4.14 | Advice fires standard photons at true range 0-1 (illegal without overload) | no |
| 9 | MAJOR | E1.50 | Reload lockout keyed per weapon FAMILY, not per weapon — suppresses legal fire | no |
| 10 | MAJOR | E1.50 / E2.22 | `phaser_actions` applies no reload check at all — full broadside every impulse | no |
| 11 | MAJOR | FD1.23 | `drone_profile()` called with the SHIP — launch range gate keys off launcher's speed | no |
| 12 | MAJOR | F3.21 | Seeker control-channel ceiling defined but never enforced | no |
| 13 | MAJOR | FP1.51/FP1.53 | Plasma torpedoes modelled as drones: flat 12 warhead, "4 damage kills it" | no |
| 14 | MAJOR | FD2.1 / FP1.51 | Seeker endurance / plasma decay never tracked — threats never expire | no |
| 15 | MAJOR | FD1.21 | Drone launch ignores FA arc and one-per-rack-per-turn rate | no |
| 16 | MAJOR | J1.52 / J1.61 | Fighter landings never parsed — "fighters aboard" only ever falls | no |
| 17 | MAJOR | J1.50 / D12.0 | `carrier_advice` never reads the log: full launch schedule re-issued forever | no |
| 18 | MAJOR | J1.61 | Carrier speed cap fed fighter BOXES as `fighters_out` — test is inverted | no |
| 19 | MAJOR | J1.4 | Any shuttle launch (weasel, SP, suicide) debited from the fighter pool | no |
| 20 | MAJOR | J4.8172 | No rearming clock — recovered fighters treated as instantly relaunchable | no |
| 21 | MAJOR | J1.331 | J1.61 cap uses rated max speed; crippled fighters halve (round up) | no |
| 22 | MAJOR | J3.131/J3.132 | Weasel advice omits maneuver-rate cap, FC shutdown, release of seekers | yes |
| 23 | MAJOR | E10.412/.413 | Hellbore modelled only in direct-fire mode; no enveloping branch at all | no |
| 24 | MAJOR | D6.35 / E1.811 | Order engine never computes an EW die-roll shift (`sfb_command` never imports `sfb_ew`) | no |
| 25 | MAJOR | D6.310 / D6.34 | ECCM never plotted by the EA path; recommender pinned to 0 by dead input | no |
| 26 | MAJOR | D6.32 / D6.315 | **Enemy ECM never parsed from log or save — constant 0 all game** | no |
| 27 | MAJOR | D6.3141 | `SENSOR_EW_POOL = 6` constant; never falls with sensor damage | no |
| 28 | MAJOR | D6.371/.372 | Tractor / transporter / SFG lock-on roll vs net ECM shift unmodelled | no |
| 29 | MAJOR | C7.11 | Disengage-by-acceleration ignores the "or 15 points, whichever is lower" clause | no |
| 30 | MAJOR | C7.22 | Seeker block ignores the endurance escape clause | no |
| 31 | MAJOR | S4.0 | Weapons Status entirely unimplemented — turn-1 capacitor assumption hardcoded | no |
| 32 | MAJOR | ADB5703 p12 | Klingon saber-dance band typo'd 13-15 (should be 9-15) **and** `preferred_range = 2` makes it unreachable | no |
| 33 | MINOR | C4.33 | Turn zeroes `since_slip` instead of satisfying it — one legal sideslip lost per turn | no |
| 34 | MINOR | C10.55 | EM never adds +1 to Turn Mode (HET half is implemented) | no |
| 35 | MINOR | C3.43 | `turn_mode(0)` returns 0; "turn now OK" for a stopped ship | no |
| 36 | MINOR | C2.232/.233 | Acceleration base not zeroed after reverse / breakdown / emergency decel | no |
| 37 | MINOR | E3.24 | Disruptor hold-charge advice cites E4.24 (a photon rule) in 6 places | no |
| 38 | MINOR | S2.41 | `_is_crippled` uses 50% warp; rule is 10% of *original* warp boxes | no |
| 39 | MINOR | D6.392 | Scout EW lending is free text: no quantity, no six-point received cap, no channels | no |
| 40 | MINOR | D6.34 | ECM hard-wired to 2 pts, blind to enemy ECCM; square thresholds are 1/4/9 | no |
| 41 | MINOR | ADB5703 p8 | `plan_eaf` reinforces shields *before* filling phaser capacitors | no |
| 42 | MINOR | ADB5703 p11 | No target selector ranks by shield state; volleys never split | no |
| 43 | MINOR | ADB5703 p30 | No bolt-vs-launch state, so plasma ships can never be steered to the Glory Zone | yes |
| 44 | MINOR | D4.21 | Only the Cadet 1d6 DAC exists, and it is dead code | yes |
| 45 | MINOR | D7.0 | Boarding parties / hit-and-run / capture absent from the order vocabulary | yes |
| 46 | ABSENT | S2.2 | Victory points / BPV not modelled — no advice is scored against the win condition | yes |

---

## 3. Findings by domain

### 3.1 Movement

#### M1 — Turn wrongly resets sideslip mode to zero (MINOR, C4.33)

> "A sideslip counts as a 'straight' movement for purposes of satisfying Turn Mode, and a turn (actually, the movement immediately after the turn) counts as a 'straight' movement for purposes of fulfilling sideslip mode. The two are completely independent. … The ship in the diagram 'turned' in position #2, moving 'straight' to position #3 as the completion of that turn. It then 'sideslipped' into position #4, i.e., the straight movement part of the turn counted as satisfying its slip mode." — C4.33

**Now:** `sfb_log.py:84` sets `d["since_slip"] = 0` on any turn event; `sfb_move.py:71-73` computes `may_slip = since_slip >= 1` and so refuses a sideslip on the impulse after a turn, emitting "cannot slip again yet (slip mode 1, C4.1)".
**Correct:** the hex entered as completion of a 60° turn satisfies slip mode. Only turn-and-slip *on the same impulse* is forbidden.
**Impact on orders:** one legal lateral correction is declined after every turn — exactly the oblique-approach geometry `move_order` is trying to produce. Conservative, never illegal.
**Fix:** in `_maneuver`, for `kind=="turn"` set `since_slip = 1` (keep `since_turn = 0`); keep the same-impulse exclusion by never emitting both in one order. Separately, `sfb_move.py:73` hardcodes slip mode as 1 rather than deriving it from speed.

#### M2 — Erratic Manoeuvres never increase Turn Mode (MINOR, C10.55)

> "(C10.55) MANEUVER: A ship using EM has its Turn Mode increased by one (four hexes to five hexes); the Turn Category (e.g., C) is not increased. One is added to all HET die rolls by ships using EM. … Nimble units are exempt from the first two effects of this rule (Turn Mode; HET), but cannot use EM while in orbit."

**Now:** `sfb_rules.py:337 turn_mode(speed, category)` has no EM parameter; the fact survives only as a comment at `:470`. No state or log parser sets an EM flag anywhere in the tree.
**Correct:** `tm + 1` when under EM, unless nimble (C11.23).
**Impact:** a turn advised one hex early for any ship under EM. Limited in practice because `sfb_maneuver.py:219-269` advises strongly against EM. The HET half of C10.55 *is* implemented (`sfb_advanced.py:140`, "EM +1").
**Fix:** `turn_mode(speed, category, em=False, nimble=False)`; thread an `em` flag from save/log into `turn_mode_of` and `move_order`.

#### M3 — HET wrongly declared illegal on impulse #1 (MAJOR, C6.33)

> "(C6.33) IMPULSES: A ship may make a High Energy Turn during any impulse whether it is scheduled to move or not."

**Now:** `sfb_maneuver.py:76-79` hard-returns `"You cannot HET on impulse #1 of any turn. (p13)"` and evaluates nothing else — no cost, no breakdown risk, no TAC alternative.
**Correct:** no impulse restriction on HETs. The "except Impulse #1" restriction belongs to sublight Tactical Manoeuvres (C5.11), where the rulebook text actually places it.
**Impact:** the impulse where an emergency escape HET is most likely needed — the first after energy allocation, when the enemy's new speed and heading are revealed — is the one impulse the advisor refuses to consider it, citing a rule that does not exist.
**Fix:** delete the `impulse == 1` early return from `het_assessment`; move it into `tac_assessment`.

#### M4 — Turn Mode 0 at speed 0 (MINOR, C3.43)

> "(C3.43) STARTING FROM ZERO: A unit starting from Speed Zero cannot turn before moving out of the hex because it has no way to satisfy its Turn Mode. If the owning player wants to turn before movement, the unit could perform an HET (C6.0) before movement, or move at Speed 1 (C3.33), or perform a Tactical Maneuver at Speed Zero and then change speed (C12.0) and move normally."

**Now:** `sfb_rules.py:344-347` returns 0 for `speed <= 1`. **Note:** returning 0 at speed *1* is correct and required by C3.432 ("Units moving at a speed of one hex per turn move on Impulse #32 and can turn before moving") — only speed 0 is wrong. The originally-cited surfacing site (`sfb_command.py:255`) is unreachable at speed 0 because `impulse_note` early-returns at `:247`. The real display sites are `sfb_command.py:351` (`maneuver_note`) and `sfb_bridge.py:1060`, both of which print "turn mode 0 at speed 0", reading as "free to turn".
**Impact:** misleading display only — `sfb_move` returns HOLD at speed 0 so no illegal order is emitted. The substantive loss is that the three legal alternatives (HET / TAC / move at speed 1) are never offered.
**Fix:** special-case speed 0 (not `<=1`); have the display say "cannot turn from a standing start (C3.43) — HET, TAC, or move at speed 1".

#### M5 — Acceleration base ignores the speed-zero resets (MINOR, C2.232/.233)

> "(C2.232) If a ship reverses direction (C3.52), its speed is considered to be zero for purposes of acceleration on the next turn. (C2.233) After suffering a breakdown (C6.541) or performing emergency deceleration (C8.4), speed is at zero for a defined period."

**Now:** `sfb_rules.py:378-380 max_accel(prev) = max(prev, 10)` — arithmetic correct per C2.21. But `sfb_tactics.py:160,242` feed it `me.last_speed`, populated in `sfb_brain.py:265` straight from the save-file piece and never zeroed. Nothing parses reverse, breakdown, or emergency-decel events from the log.
**Correct:** base is zero after a reverse (capping next turn at 10) and for a defined period after breakdown/emergency decel. Also unmodelled: C2.25 (HET/TAC/EM/braking hexes excluded from prior speed) and C2.24 (Tholian web keeps recorded speed).
**Impact:** on the turn after a reverse or breakdown from above speed 10, the engine recommends an EA for a speed the client will reject. Rare but the whole turn's allocation must be redone.
**Fix:** add reverse/breakdown/decel patterns to `sfb_log.parse`; record a per-ship `accel_base`.

---

### 3.2 Energy

#### E1 — ~~Shield operation cost chart wrong~~ **RETRACTED — the engine was right**

> **This finding was WRONG and has been reverted.** It was derived from the D3.32 chart in a
> `pdftotext` extract whose FULL/TOTAL columns are displaced UP BY ONE ROW, so the line that
> reads "3 (Cruisers) =1 +3 =4" is actually SC2's total. Measured on the client's own energy
> allocation form: **SC2 (LDR DN Lion) = 4, SC3 (Lyran CA Tiger) = 2** — exactly the original
> `{1:7, 2:4, 3:2, 4:1, 5:1}`. Acting on this finding over-booked every cruiser by 2 points a
> turn until it was caught. Do not re-apply it from the text extract; measure on the EAF.

<details><summary>Original (incorrect) finding, kept for the record</summary>

#### E1 — Shield operation cost chart wrong (MAJOR, D3.32)

> "(D3.32) COST OF OPERATION: The cost to operate a ship's shields is based on its size. … SIZE CLASS MINIMUM FULL TOTAL / 1 (Starbases) =2 +5 =7 / 3 (Cruisers) =1 +3 =4 … Note that all costs are given as Minimum+Full, and full shields cannot be operated without also operating minimum shields. For a ship to have all of the boxes on its SSD active, it pays the number in the 'Total' column."

**Now:** `sfb_rules.py:452 SHIELD_COST = {1:7, 2:4, 3:2, 4:1, 5:1}`, consumed unadjusted at `sfb_command.py:89`; the comment at `:86` says "SC3 cruiser = 2". For a size-3 cruiser the engine books 2 — not even the legal *minimum* of 1+0.
**Correct:** SC3 = 4 (Total column). SC1 = 7 is the one value the table gets right. **SC4 = 1 and SC5 = 1 are defensible** under D3.321: "When not using fractional accounting (B3.2), the cost of operation for size class 4 or 5 is 1 for minimum and +0 for full." SC2 should be 5, but the SC2 row in the text extract is column-garbled — only its minimum "=1" is legible verbatim, so confirm SC2 against a clean scan before committing a number.
**Impact:** every cruiser EA under-books housekeeping by 2. `compute_eaf` hands those 2 phantom points to speed, ECM or reinforcement, so the emitted order is over-budget. ~7% of a 30-power CA, every turn.
**Fix:** set `SHIELD_COST` to the Total column; add a separate `SHIELD_MIN_COST` so minimum-shield operation (D3.33) can be offered as a deliberate economy. Fix the comment.

</details>

#### E2 — `MANDATORY_POWER` is a size-blind constant (MAJOR, B3.3 / D6.6 / D3.32)

> "7. LIFE SUPPORT: You MUST allocate energy to life support or your entire crew will perish immediately. The life support cost for a ship depends on its size class. … 8. ACTIVE FIRE CONTROL: One unit of power will operate the scanners and sensors for the current turn; see (D6.6)."

**Now:** `sfb_rules.py:415 MANDATORY_POWER = 6`, commented "A2.0 life support(2) + A5.0 fire control(2) + D3.32 shield maint(2)". Both cited rule numbers are wrong and two of three components are wrong: life support is size-keyed (B3.3, and `LIFE_SUPPORT_COST` already gets it right) and fire control is 1, not 2.
**Not dead code:** `sfb_tactics.py:225` (`power = me.power - MANDATORY_POWER`, inside `plan_eaf` → `make_plan` → `sfb_brain.py:333`) sets the movement/arming budget and therefore the recommended speed; also `sfb_advanced.py:69,185`, the latter printing "housekeeping 6 (LS+shields+FC) pre-plotted" as text.
**Mitigation:** `sfb_command.py:88-96` already implements the correct size-keyed formula and drives the bridge EAF panel, so the human-facing EAF numbers are right. The constant corrupts the tactics/brain internal budget only. That is the real defect: two disagreeing housekeeping figures.
**Impact:** for an SC3 cruiser the true figure is 2+1+1 = 4, so the tactics layer over-reserves 2 and under-recommends speed/arming. For large units it under-reserves badly. (The starbase 3+1+7 = 11 figure depends on the D3.32 chart being fixed first — see E1.)
**Fix:** delete the constant; have `sfb_tactics`/`sfb_advanced` call the same size-keyed logic `compute_eaf` uses.

#### E3 — Battery recharge capacity hardcoded to zero (MAJOR, H7.0 / B3.4)

> "If a unit leaves part of its power output unallocated, it is simply assumed that the engine/reactor was operated at a lower power output and the unallocated energy was never created. This unallocated energy cannot be used for reserve power. A player would be better off to allocate it under (H7.4)." — B3.4

**Now:** `sfb_command.py:169` is literally `batt_room = 0  # nothing to recharge unless batteries are down`, so `to_batt` is always 0 and the recharge branch at `:176` is dead. `:173-175` unconditionally prints "batteries {batt} (already charged … nothing to allocate)". Root cause is upstream: **no module anywhere reads a live battery charge level.** `ship["power"]["battery"]` is a static capacity; `sfb_rules.battery_tap` is a plotted-power input, not a stored charge.
**Correct:** batteries discharge as soon as reserve power is tapped (H7.0); recharging them is a legal and normally correct EA line. The engine's own comment at `:167` lists it as a legal sink, then disables it.
**Impact:** after turn 1 of any real battle, batteries are partly spent and the advisor still asserts they are full, pushing surplus into ECM past the useful cap or declaring it "NOT PRODUCED". The ship enters the next turn with no reserve for emergency decel, HET recovery, or a mid-turn TAC. Note the fallback allocation is itself legal, so this is wrong-*priority*, not illegal.
**Fix:** seed charge from capacity at scenario start; decrement on every reserve-power expenditure the log reports; `batt_room = capacity - charge` with `to_batt` ranked above the ECM sink.

#### E4 — `reinforce_plan` targets DOWN shields (MAJOR, D3.343)

> "(D3.343) DOWN SHIELDS: A shield that is down (or which has been dropped) cannot be reinforced, but general reinforcement would still block fire coming from that direction (for as long as it lasts)."

**Now:** `sfb_command.py:1817-1836`. `idx = H.shield_hit(...)` is pure geometry; `sh[]` is read only at `:1821` to compare the two neighbours — and the comparison **actively prefers the weaker** (`second = nxt if sh[nxt] <= sh[prv] else prv`), so a neighbour at 0 boxes is the one chosen. There is no `sh[i] > 0` test on either `primary_idx` or `second_idx`. Consumed unguarded at `:196-204`, printing "REINFORCE {facing} +N … (specific, D3.342: 1 energy = 1 box)".
**Correct:** specific reinforcement may not go to a down or dropped shield at all. The only legal protection for a down facing is *general* reinforcement (D3.341) — which the engine explicitly tells the player not to buy (`:1800`).
**Impact:** in exactly the situation that decides the game — a shield knocked down, enemy bearing on it — the engine issues an illegal allocation and simultaneously advises against the one legal alternative.
**Fix:** filter candidate indices to `sh[i] > 0` for both primary and secondary; when the threatened facing is down, switch to general reinforcement citing D3.341/D3.343 and suppress the blanket anti-general line for that case.

#### E5 — Blanket "never use general reinforcement" (MAJOR, D3.3412) — *needs playtest*

> "(D3.3412) In the case of certain weapons, such as enveloping plasma torpedoes (FP5.0) and hellbores (E10.0), general reinforcement is subtracted from the weapon's strength before damage is calculated. It is the number of general reinforcement points, not the number of energy points, that is subtracted."

**Now:** `sfb_command.py:1799-1801` emits, unconditionally and with no reference to the enemy weapon mix: "SHIELD do NOT use general reinforcement here: it is HALVED (D3.341) and the first damage from any direction wipes the lot". `GENERAL_REINFORCE_RATE` (`:1710`) is defined and never consumed; `reinforce_plan` only ever emits specific reinforcement.
**Correct:** against a hellbore or enveloping plasma, GR points are subtracted from the weapon's *strength* before the damage is divided across the shields — so each GR point cancels a full damage point off the total rather than protecting one facing, worth roughly six times a specific point in that matchup. (Not, as sometimes stated, "protects all six facings simultaneously" — the damage is already being divided; GR shrinks the total before division.) The halved *cost* (2 energy per point) remains true and should still be stated. GR is also the only reinforcement that helps a down facing.
**Impact:** against Hydran hellbore ships or enveloping-plasma Gorn/Romulan opponents the advisor steers the player off the single most efficient defensive expenditure available.
**Fix:** gate the anti-GR line on the enemy weapon list already parsed into `e["weapons"]`; when a hellbore or enveloping-capable plasma is present, recommend GR (energy/2, round down) citing D3.3412, and let `reinforce_plan` emit a general line.

---

### 3.3 Direct fire

#### D1 — Weapon damage tables fabricated (MAJOR, E2.411/E2.412/E3.4/E4.12)

> "(E2.411) EXAMPLE #1: A phaser-3 is being fired at a target three hexes away. There is no electronic warfare. The die roll is '2' which means that two points of damage have been scored."
> "(E2.412) … a phaser-1 … The die is rolled and the result is a '1,' which would normally mean six damage points (it would have been eight if the fire control systems had been working and the effective range shorter)."

**Now, `sfb_rules.py:58-74, 132-211`:**
- `_PH1_TBL` row "r 1-3" is `[10,9,8,7,6,5]`. A phaser-1 can never do 10, and at range 3 a die roll of 1 must give 6. The project's own sourced table (`sfb_command.py:672 PH1_EXPECTED R0=6.500`) implies a 9-8-7-6-5-4 row.
- Ph-3 (`:150-151`, `:200-201`) returns `allocated` at range ≤2 and 0 beyond — zero at range 3, where E2.411 says 2.
- `_DISR_TBL` treats the disruptor as damage-per-die capped at range 15; E3.4 makes it hit-or-miss with fixed damage out to **40** hexes (Damage(S) 5/4/4/3/3/2/2/1, hit 1-5/1-5/1-4/1-4/1-4/1-3/1-2/1-2).
- Photon `roll_damage` (`:202-206`) uses hit numbers 4/3/2/1 at ≤8/≤15/≤25/≤30; E4.12 is 1-5 at R2, 1-4 at R3-4, 1-3 at R5-8, 1-2 at R9-12, 1 at R13-30.

**Correction to note:** `damage_at` *does* honour `overloaded` for photons and disruptors (`:152-159`); only `roll_damage` ignores it, and `roll_damage` is reached only via `sfb_ew.py:40`.
**Impact:** these tables drive `sfb_advanced.firing_solutions` (`:53`) and `sfb_tactics.plan_fire` (`:205`) — both order producers, and `plan_fire` *drops a target outright* when `dmg <= 0`. A disruptor boat is told it has no shot beyond 15 hexes when it has one to 40. A photon ship is told it hits 33% at R25 when the truth is 17%. Ph-3 point defence is declared useless at range 3.
**Fix:** delete the tables in `sfb_rules`; move `sfb_command`'s sourced `DISR_STD/PHOT_STD/PH*_EXPECTED` (`:525-680`) into `sfb_rules` and have `sfb_command` import them — exactly one chart per weapon in the codebase.

#### D2 — Overload damage ignores range and energy invested (MAJOR, E3.52/E4.413)

> "(E4.413) The strength of an overloaded torpedo is determined as follows: Total Energy / Warhead Strength … 4.5 → 9, 5 → 10, … 8 → 16. (E4.414) … if not armed with at least 4.5 points of warp power it cannot be fired as an overload."
> "(E3.52) The warhead strengths of overloaded disruptors are doubled." Master Weapons Chart: "Damage (O) 10 10 8 8 6 0 0 0 0". "(E3.54) Overloaded disruptors can be fired at Range 'Zero'."

**Now:** `sfb_rules.py:152-159` — overloaded photon is a flat `16 if 2 <= rng <= 8 else 0`; overloaded disruptor is a flat `12 if rng <= 8 else 0`.
**Correct:** the photon energy scaling **is** already implemented correctly in `sfb_command.photon_overload_damage()` (`:590-601`, used by `expected_damage()` at `:734`) — the defect is that `sfb_rules.damage_at` duplicates it with a flat 16. Genuinely unimplemented: the disruptor overload curve 10/10/8/8/6 (which exists in *no* module — 12 is a value that appears nowhere on the chart), and the range 0-1 overload window for both weapons, where `damage_at` returns 0 for the shot the rules most reward.
**Impact:** the overload/standard decision is made on these numbers. Flat-12 overstates the close-range disruptor overload by 50-100% at R5-8, biasing the whole approach geometry; the zero at R0-1 steers the AI away from point-blank overloads entirely.
**Fix:** route `damage_at`'s overload branches through `photon_overload_damage()` and a new `DISR_OVL` table; carry actual allocated overload energy into the `Weapon` object rather than assuming full.

#### D3 — Photons ordered at true range 0-1 (MAJOR, E4.14)

> "(E4.14) MINIMUM RANGE: Even when firing without a 'lock-on' (where the range would be doubled), photons cannot be fired at a true range of one hex or less. Exception, see overloads (E4.43)."

**Now:** `sfb_actions.py:151-190 fire_actions()` tests only `if rng <= hi:` (`:181`) with the photon band `(1, 8)` from `sfb_command.py:884`. There is no lower-bound test at all, and no distinction between standard and overloaded, so at true range 0 or 1 it emits "FIRE {n} PHOTONS".
**Partially known elsewhere:** `sfb_rules.py:203` (`if rng < 2 … return 0`) encodes E4.14 in the damage-chart path, and `sfb_command.py:603-616` models the E4.43 feedback. Neither is consulted by the advice path.
**Impact:** at the exact moment a Federation ship crosses to range 1 in an overrun, the advisor issues an order the client will reject, or the human executes it and loses the volley.
**Fix:** in `fire_actions`, gate the photon family — `rng <= 1 and not overloaded` → "PHOTONS: HOLD — E4.14 minimum range"; if overloaded, fire plus `cmd.feedback_warning('photon', rng, True)`.

#### D4 — E1.50 reload tracked per FAMILY (MAJOR, E1.50)

> "(E1.50) BASIC RULE: No weapon may be fired twice within a period of one-fourth of a turn. … For example, if a specific phaser were fired during Impulse #29 of one turn, it could not be fired again before Impulse #5 of the next turn."

**Now:** `sfb_log.py:147` records `fired[ship][weapon.lower()].append((turn, imp))` — keyed by weapon *type string*, with the fired count discarded. `sfb_actions.py:156,173-179` looks up `fired.get(fam)` and, if within `RELOAD_IMPULSES = 8`, emits "{LABEL}: RELOADING — cannot fire" and `continue`s. The same family-level treatment is duplicated at `sfb_command.py:931-960`.
**Correct:** per individual weapon. A ship with four photon tubes may fire tube 1 on impulse 12 and tubes 2-4 on 13.
**Impact:** a Klingon with 6 disruptors that fires 2 as a ranging shot is told it has no disruptors for 8 impulses — it forfeits the alpha strike at closest approach. The 8-impulse threshold is right; the granularity is not. Over-conservative, never illegal.
**Fix:** the root cause is upstream — `sfb_log.py:147` must retain the fired *count* (and ideally a tube/box id) so `fire_actions` can report "FIRE k of n DISRUPTORS (m reloading)". A key change in `sfb_actions` alone cannot fix this.

#### D5 — Phaser advice applies no reload check at all (MAJOR, E1.50 / E2.22)

**Now:** `sfb_actions.py:193-217 phaser_actions(ship, tgt, rng, state, cmd)` has no `log` parameter and no reload/fired test; it emits "FIRE {nph} PHASERS" on every consecutive impulse the target is within range 5. `impulse_actions` (`:220+`) *has* `log` in scope and passes it to `fire_actions` and `fighter_actions`, but not to `phaser_actions`.
**Data is live and available:** `sfb_log.py:147` records phaser fire stamps, and `sfb_command.py:1487-1499 weapons_history()` prints per-weapon "reloading, ready in N imp"; `:1099-1101` gates capacitor reallocation on phaser fire this turn. So the briefing *shows* the conflict while the ACTION line contradicts it.
**Correct:** E1.50/E2.22 are per-*weapon*. A ship with 6 phasers that fired 2 on impulse 12 may legally fire the other 4 on 13 — so the order is wrong in **count**, not automatically illegal.
**Impact:** overstates damage-per-impulse and encourages staying in the kill zone an extra impulse.
**Fix:** pass `impulse, turn, log` into `phaser_actions`; apply the same lockout; enforce once-per-turn (E2.22) by counting firings against box count; exempt ph-G (E2.151). Note the same family-granularity limitation as D4 applies.

#### D6 — Disruptor hold-charge cites the wrong rule (MINOR, E3.24)

> "(E3.24) HOLDING: Armed disruptors cannot be held and fired on a later turn. If energy is allocated to fire a disruptor, and it is not fired on the turn of arming, the energy is lost and cannot be regained."

**Now:** the advice string reads "E4.24: disruptors cannot hold a charge across the turn break". E4.2x is the *photon* section. The bad citation appears in **six places**: `sfb_actions.py:10` and `:184`, `sfb_command.py:946, 963, 965, 1484`, `sfb_log.py:8`. `sfb_command.py:625` gets it right as "E3.24/E3.51".
**Impact:** the substance is correct; a human checking the citation lands in the photon rules and loses confidence. Two modules cite the same fact with different numbers.
**Fix:** change all six to E3.24/E3.51.

---

### 3.4 Seeking weapons

#### S1 — `drone_profile()` called with the SHIP (MAJOR, FD1.23)

> "(FD1.23) SPEED: Drone speed is determined by the drone type."

**Now:** `sfb_actions.py:128-149` does `prof = cmd.drone_profile(ship)` and `speed = prof.get("speed", 8)`, then gates on `rng <= speed * 2`. But `sfb_command.py:441-460 drone_profile(s)` is written for a **seeking-weapon record** from `state["seeking"]`: it reads `s["loadout"]` and `s.get("speed") or s.get("max_speed") or 12`. Its only other caller, `incoming_seekers` (`:472`), correctly passes a seeking record.
A ship dict has no `loadout` (so type/warhead/module all silently default) and its `speed` is the launcher's own current movement speed — falling through to the ship's `max_speed`, or 12, at speed 0.
**Correct:** drone speed comes from the type/speed module in the rack (type-I 8, type-II/III 12, -M 20, -F 32), never from the launcher.
**Impact:** a ship coasting at speed 4 is told to HOLD DRONES beyond range 8 even though its speed-20 drones easily reach; a ship sprinting at 26 is told to LAUNCH at range 52, wasting the rack. The stated reason ("range 12 is within a speed-4 drone's reach") is simply false.
**Fix:** add `rack_drone_profile(ship)` reading the drone type/speed module from the ship's own loadout/SSD; keep `drone_profile()` for `state["seeking"]` records. Gate on `speed * endurance_turns`, not `speed * 2`.

#### S2 — Seeker control channels never enforced (MAJOR, F3.21)

> "Each unit has a specified number of 'control channels' to be used in guiding seeking weapons. Each channel can control one weapon at a time. (F3.21) SHIPS … can control a number of seeking weapons (drones, plasma torpedoes, pseudo-plasma torpedoes, scatter-packs, suicide shuttles) equal to their sensor rating at any given time (usually six)."

**Now:** `sfb_command.py:436-438 SEEKER_CONTROL_ARMED, SEEKER_CONTROL_UNARMED = 6, 3` — referenced nowhere else in the tree (only prose in `sfb_actions.py:148` and `sfb_situations.py:347`). `incoming_seekers` walks `state["seeking"]` but filters to seekers targeted *at* this ship (defence), never seekers launched *by* it. No outbound-channel accounting exists.
**Inputs already exist:** `state["seeking"]` carries x/y/kind/target, and `sfb_log.py:38-40 RE_ADDED_LAUNCH` already parses "…was launched by \<ship\>", giving per-launcher attribution. Joining those two sources gives the count for free.
**Correct:** count in-flight seekers under this ship's control against its sensor rating; halve (round up) per F3.211 for a ship not armed with drones/plasma; double per F3.212 (Kzinti CV); scouts may add channels (F3.213); the ceiling falls with sensor damage at end of turn (F3.215).
**Impact:** at saturation the engine keeps ordering "LAUNCH DRONES (x3 racks)"; the excess cannot be guided. The F3.211 half-rating case is where wrong advice is likeliest early — a phaser/photon ship advised to launch a scatter-pack or suicide shuttle with only three channels. A naive ceiling of 6 would give a Kzinti CV *wrongly conservative* advice, so implement F3.212 alongside.
**Fix:** compute `in_flight` by joining `state["seeking"]` against `RE_ADDED_LAUNCH` attribution; pass ceiling and in-flight into `drone_actions` and gate the LAUNCH headline on free channels.

#### S3 — Plasma torpedoes modelled as drones (MAJOR, FP1.51/FP1.53)

> "(FP1.51) STRENGTH CALCULATION: The warhead strength of a plasma torpedo is determined at the instant of impact, based on two factors: the distance that the torpedo has traveled (it grows weaker the farther it travels) and damage done to it (FP1.6) by phasers … Once the warhead strength reaches zero, the torpedo has no further effect or function and the counter is removed from the board."

**Now:** `sfb_command.py:463-479 incoming_seekers()` calls `drone_profile(s)` on **every** entry in `state["seeking"]` regardless of `s["kind"]` — and `sfb_log.py:124` does classify added units as 'plasma', so plasma counters really do reach this path. `seeker_defence` (`:487-519`) then uses `warhead = 12` and `kill_each = 4`, printing "up to {total_warhead} damage" and "phaser PD: N phasers, 4 dmg kills each (~X pass(es))". No FP1.53 table or plasma-strength function exists anywhere; `sfb_rules.py:160-164`'s plasma formulas are the internal sim model and are never consulted here.
**Correct:** strength from the FP1.53 table by type and hexes travelled (R 50, S 30, G 20, F 20, D 10, decaying with distance), *weakened point-for-point* by phaser damage (FP1.6) rather than destroyed by a fixed 4.
**Impact:** facing an incoming plasma-S the advisor understates the threat as 12 instead of up to 30, and tells the captain that one or two phaser passes will "kill" it — when in reality those phasers only shave points off a torpedo that still lands. Badly under-allocates point defence against Romulans/Gorns.
**Fix:** branch on `s["kind"]` in `incoming_seekers`: derive the type letter, compute hexes travelled from launch position/impulse, look up FP1.53, set `kill=None`, and switch `seeker_defence` to attrition language ("N phaser points reduce the warhead to X") plus the real answers — wild weasel, ESG is NO help (G23.81), outrun it, absorb on a strong shield.

#### S4 — Seeker endurance never tracked (MAJOR, FD2.1 / FP1.51)

**Now:** `sfb_command.py:377-378 DRONE_TYPES` carries an endurance field; a repo-wide grep for "endurance" finds only two comments and the advice string at `:503`. `drone_profile()` drops endurance entirely. `incoming_seekers` computes `turns_to_impact = range / (seeker speed - own speed)` and nothing else — no age, no launch impulse, no remaining endurance. Line 503 tells the captain to "hold speed and let their endurance expire" while the engine has no idea when that is.
The launch timestamp needed to age a seeker **is** in the log (`sfb_log.py:33-40`) but is consumed only for carrier fighter counts and never attached to a seeker record.
**Correct:** endurance is finite per type (doubled for extended range, FD2.222); plasma strength hits zero at a known distance. A seeker whose remaining endurance is less than its time-to-impact at your current speed is already dead.
**Impact:** evasive speed, wild weasels and phaser PD are advised against seekers that will go inert on their own, burning shuttles and power and distorting speed/heading. Combined with S3, a near-burnout plasma worth 3 points and a fresh S-torp are given identical urgency.
**Fix:** attach the launch turn/impulse to each seeker record; look up endurance in `DRONE_TYPES`; drop or flag seekers whose remaining endurance < turns_to_impact.

#### S5 — Launch ignores FA arc and per-rack rate (MAJOR, FD1.21 / FD1.1)

> "(FD1.21) PROCEDURE: When launched, the drone is placed on top of the launching ship, facing any direction at the option of the owning player. Drones must have their target in their FA arc when launched. The target ship for each drone must be announced on launch; exception (F3.6)." … "four spaces of drones and can fire one per turn."

**Now:** `sfb_actions.py:128-149` gates only on rack count, target existence, and `rng <= speed*2` — never reads ship facing, never calls `H.target_in_arc`, carries no per-turn state, and is called unconditionally every impulse. `sfb_command.py:1165-1169` is worse: it appends "+ launch drones (x N racks)" purely from rack count with no range/arc/rate test at all. `sfb_tactics.py:200` explicitly bypasses arc checking for seekers (`if not is_seek and not hexlib.target_in_arc(...)`), and `sfb_rules.py:230 make_drone` builds the rack with `ARC_ALL` — the model asserts a 360° launch arc, flatly contrary to FD1.21.
**Impact:** with an enemy astern the engine still emits "LAUNCH DRONES at X", and because nothing remembers a rack fired, it re-emits every impulse — telling the human to launch four drones from a four-drone type-A rack in one turn. The parallel fighter path (`sfb_actions.py:105-125`) *does* track a launch gap, so the pattern was available and simply not applied.
**Note:** there is **no** 8-impulse lockout on shipboard drone racks. The 8/16-impulse lockouts (J1.341/J1.342) apply to newly launched fighters. The confirmed constraints are FA arc at launch and one drone per rack per turn.
**Fix:** gate `drone_actions` on `H.arc_of(ship, tgt)` being FA; if not, downgrade to "TURN TO BEAR then launch" and feed the required heading into the movement order. Track per-rack launches per turn from `RE_ADDED_LAUNCH` and cap the advertised volley.

#### S6 — Wild weasel preconditions not checked (MAJOR, J3.131/J3.132) — *needs playtest*

> "(J3.131) The ship may move at a maneuver rate (C2.42) of no more than four."
> "(J3.132) The ship must deactivate its active fire control system (D6.6). The ship may not fire weapons, even with passive fire control (D19.21). All lock-ons are lost. The ship immediately releases (F3.4) control of all seeking weapons in flight."

**Now:** `sfb_command.py:433` defines `WW_MAX_MANEUVER_RATE = 4` and it is referenced nowhere else. `weasel_advice` (`:1855-1890`) covers charge cost (J3.12/J3.121), 32-impulse lead time (J3.122), one-at-a-time (J3.116), protects-only-launcher (J3.202), held-does-nothing (J3.24), type-VI/Boar counters and doctrine — but never J3.131 or J3.132. `sfb_shuttles.py:135-148` has the same list and the same omission. A grep for "J3.131"/"J3.132" returns zero hits.
**Already handled:** the ESG interaction (G23.48) is covered in both places and at `sfb_actions.py:40`; the EM/weasel conflict is flagged at `sfb_maneuver.py:250`. (The earlier framing of this as a C10.512 problem was wrong — C10.512 governs Erratic Manoeuvres and is already covered.)
**Impact:** a weasel caps maneuver rate at 4 (constraining the speed/EAF plot the engine produces in the same impulse), shuts fire control, and **releases control of every seeking weapon in flight**. The engine can therefore recommend charging a weasel alongside a drone-guidance or direct-fire plan the launch would nullify.
**Fix:** make `weasel_advice` consume the plotted speed/maneuver rate and the current impulse's other recommended actions; suppress or explicitly flag the conflict when the same impulse also recommends drone launch, seeker guidance, EM, or direct fire.

---

### 3.5 Fighters and carriers

#### F1 — Fighter landings never parsed (MAJOR, J1.52 / J1.61)

> "(J1.52) LAUNCH-LAND SEQUENCE: A shuttle can launch once per turn. A shuttle can land once per turn. A shuttle can launch and land during the same turn (in either order), but cannot perform either action twice in the same turn. There is a minimum 1/4-turn delay between launching and landing, except that a shuttle may land under (J1.62) less than 1/4 turn after it is launched."
> "(J1.61) LANDING ABOARD: … A shuttle may only land aboard a ship under its own power if both the ship and the shuttle are in the same hex and the ship is not moving faster than the shuttle (current speed of each)."

**Now:** `sfb_log.py` has **no** land/recover/removed pattern — the event vocabulary is `RE_LAUNCH / RE_ADDED_LAUNCH / RE_ADDED / RE_DEST` only. `sfb_actions.py:77-84` counts every `kind=="launch"` over the whole log with no decrement and computes `remaining = max(0, ftr - launched_so_far)`. So `launched_so_far` is monotone for the entire game and `remaining` can only fall.
**Impact:** after a full deck cycle the engine emits "fighters: all N airborne — deck clear / nothing left aboard to launch" (`:88-90`) and the `remaining <= 0` early-out at `:110` suppresses further launch orders, even though recovered fighters are legally relaunchable next turn. It also never advises a recovery. **This silently disarms a carrier's main weapon for the rest of the battle** — the same shape as the two bugs already fixed.
**Note:** J1.52's 1/4-turn delay is a launch-*then*-land delay (with the J1.62 tractor exception); relaunch is gated by once-per-turn and the J1.50 bay cycle, not by that delay.
**Fix:** add a recovery regex (the client's removal/"has been removed"/"landed on" line for shuttle-type units) emitting `kind='land'` with ship+unit; compute `remaining = ftr - (launches - landings)`, tracking unit ids so a destroyed airframe is not credited back.

#### F2 — `carrier_advice` never reads the log (MAJOR, J1.50 / D12.0)

> "(J1.50) LAUNCH RATE: A given shuttle bay (J1.51) may not launch or recover more than one shuttle during any given impulse or two consecutive impulses"

**Now:** `sfb_command.py:1244 carrier_advice(ship, enemies, rng, impulse)` takes no `log`/`state`. `:1257 group = ftr[0] or 6` uses the static SSD box count; `:1262` passes `has_armed_fighters=bool(ftr[0])`; `:1266` prints a full `CAR.launch_schedule` from scratch **every impulse**. `sfb_carrier.py:226-229` has the "No armed fighters aboard — D12.0 chain reaction does not apply" branch, which can never fire from this call site.
**Impact:** on turn 3 the console still prints "LAUNCH ORDER — 12 fighters … imp 4: launch 1 …" for a carrier whose group has been up two turns, and keeps warning about chain reaction. Worse, it appears in the **same output block** (`:1187`) as the log-aware line from `sfb_actions.impulse_actions` (`:1143`), so the two panels contradict each other outright.
**Fix:** small — the parsing machinery already exists. Thread `log`/`turn` into `carrier_advice`; factor the launch/land tally out of `sfb_actions` into a helper; pass `group_size=remaining` and `has_armed_fighters=(remaining>0)`.

#### F3 — J1.61 speed cap fed fighter BOXES (MAJOR, J1.61)

**Now:** `sfb_command.py:1033, 1056, 1076` all call `CAR.carrier_speed_limit(f, fighters_out=bool(systems['fighter'][0]))`. That slot is fighter *boxes still aboard* (confirmed by `sfb_actions.py:60,84` computing `remaining` from it, and `sfb_command.py:820` using `>=4` on it as a carrier hull test). `sfb_carrier.py:130-141` returns None (no cap) when `fighters_out` is falsey.
**Impact — the test is inverted.** A carrier with its whole group in the bay is capped to fighter speed 8, and via `sfb_command.py:1030-1041` drags the **entire squadron's** ordered speed down to 8 (escorts get MATCH THE CARRIER). The emitted note asserts a false state ("With the group out…"). Conversely the cap is retained only accidentally when fighters are genuinely out — boxes track undestroyed airframes, not occupancy — and vanishes entirely once the boxes are shot away, exactly when the survivors need to land.
The correct airborne count already exists (`RE_ADDED_LAUNCH` → `sfb_actions.py:76-83`) and simply is not wired in. J1.62 tractor recovery gives a speed-independent fallback, so stranding is not absolute.
**Fix:** compute `airborne = launches - landings` and pass it at all three call sites; drop the `bool()`.

#### F4 — Any shuttle launch debited from the fighter pool (MAJOR, J1.4)

> "Each SHUTTLE box on the SSD represents the capacity to operate one administrative shuttle or fighter." — J1.4

**Now:** `sfb_log.py:122-125` classifies an added unit by substring — drone/plasma/"shuttle" → 'shuttle', and **any other string falls through to 'fighter'**. `sfb_actions.py:78-83` counts every launch with `what in (None,'shuttle','fighter')` against the fighter total. The client's observed Type field is "User-Defined Shuttle", so weasels, suicide shuttles and scatter-packs are debited from the fighter pool.
**Impact:** a Kzinti ship that launches a wild weasel is told one fewer fighter is aboard; on a 2-fighter escort two weasels make the engine report "all fighters airborne — deck clear" and refuse to launch the actual fighters when drones arrive.
**Important rules correction:** J1.4 does *not* establish separate pools by default — it says the opposite ("one administrative shuttle **or** fighter"), i.e. a shared pool. Separate pools exist only where the SSD differentiates the boxes or section R gives per-type counts (carriers). So a blanket "only fighter launches reduce fighters remaining" is wrong: on an undifferentiated ship a launched admin shuttle legitimately consumes a box. **Which ships are differentiated depends on SSD/section-R data, and the SSD books extracted empty — the per-ship split cannot be verified from this corpus.**
**Defensible fix:** (a) stop defaulting unknown Type strings to 'fighter' — treat generic 'shuttle' as ambiguous; (b) exclude units the engine itself just advised as WW/SS/SP; (c) on ships known to be differentiated (carriers, from section R), debit only fighter launches.

#### F5 — No rearming clock (MAJOR, J4.8172)

> "Loading cannot commence until the impulse after the fighter enters the bay; the fighter cannot launch until the impulse after reloading is complete (unless the deck crew is interrupted). A fighter could land during Impulse #10 of Turn #2, be reloaded (by two deck crews) with two type-I drones, and launch on Impulse #11 of Turn #3. No more than two deck crews can work on one fighter or shuttle."

**Now:** `sfb_deckcrew.py` has `ACTION_IMPULSES = 32` and `MAX_CREWS_PER_SHUTTLE = 2`, but its only cross-module consumer is `scatterpack_advice` (`sfb_shuttles.py:86`). `crew_load_summary` (`sfb_deckcrew.py:134`) — the one function that mentions the ~1-turn rearm — is referenced by no module. `sfb_carrier.recovery_advice` covers J1.50/J1.52/J1.61/J1.62 mechanics but starts no clock. `ready_rack` is read only inside `deck_crews`' own escort fallback.
**Correct:** a drone-armed fighter that lands is unavailable for roughly a full turn (32 impulses per one-space drone, two crews max).
**Note:** the rack is *normally already full* — J4.8223: "Normally, a carrier keeps the ready racks filled but the fighters unloaded … The deck crews then reload the racks while the fighters are on their mission so that the fighters can be reloaded quickly when they return." The empty-rack blocker is the WS-III exception in J4.8224, not the general case.
**Impact:** the engine mis-prices the entire strike cycle. Once landings are parsed (F1), it will order a re-launch on the next bay impulse of a fighter still being rearmed, sending unarmed fighters back out.
**Fix:** per-airframe state machine keyed off land events: landed → reload start (impulse+1) → ready at `+ACTION_IMPULSES*spaces/min(crews,2)` → +1 impulse. Gate LAUNCH on it, and wire `crew_load_summary` into the carrier panel so competing SP/WW/rearm demands on the same crews are visible.

#### F6 — J1.61 cap uses rated max speed (MAJOR, J1.331)

> "(J1.331) SPEED: When a shuttle is crippled, its maximum speed is reduced to 1/2 of its rated maximum (round fractions up; a shuttle with a speed of eleven can move six when crippled)."

**Now:** `sfb_carrier.py:114-127 fighter_speed()` returns a static chart value; `carrier_speed_limit()` (`:130-141`) returns it verbatim as the cap. A grep for "crippl" across the tree shows every hit concerns *ships*; nothing halves a fighter's ceiling.
**Impact:** once fighters take 2/3 of destruction-point damage the true landing ceiling halves (speed-8 AAS → 4, HAAS 11 → 6) and the engine still tells the carrier to run at 8/11 — stranding the group exactly when it most needs to recover. Wrong in the unsafe direction.
**Two qualifications:** the J1.612 "available shuttle box" limb is weak (with the group out, those boxes are empty by construction unless shot away). And `FIGHTER_SPEED` holds only four Kzinti entries; the race fallback at `:124-126` returns `min()` of the charted entries with `known=True`, so an unparsed Kzinti TAAS (speed 12) is reported as a hard "8" *presented as fact* — a separate mislabelling defect.
**Fix:** take `min` over surviving airframes of `ceil(max/2)` if crippled else max, using damage from the log's shuttle lines. **Per-ship fighter speeds beyond the four Kzinti types cannot be verified here** — the Master Fighter Chart (G3 Annex) would have to be transcribed by hand.

---

### 3.6 Damage

#### DM1 — `reinforce_plan` on down shields — see **E4** (same defect, damage-domain view).

#### DM2 — Hellbore has no enveloping branch (MAJOR, E10.412/.413/.441)

> "(E10.412) STEP B: Determine which is the weakest shield (there may be two or more equally weak shields). Divide the damage scored by the hellbores (by all hellbores which struck the ship in that volley) by 1+X, where X is the number of 'weakest shields.' One of the resulting groups of damage points will be applied to each 'weakest shield' while the remaining group will be distributed over all of the other shields in Step C."

**Now:** `sfb_rules.py:92-94 _HELLBORE_DF` is explicitly the **direct-fire** chart (E10.711 base damage 10/8/7/6). The HELLBORE branch (`:168-175`) returns `P(2d6<=hit#) * DF base` as a single scalar, and the comment claims "enveloping hits the WEAKEST shield regardless of facing" — itself wrong, since enveloping splits across *all* shields. Downstream, `sfb_command.py:775-781 absorb_capacity` sums only the facing shield + hull, and `:983-985 es_val/es_max` come from `facing_shield(tgt, ship)` alone. A grep for "envelop" finds no numeric model anywhere — only prose in `sfb_situations.py`.
**Scope note:** the client resolves the actual allocation, so the engine need not reproduce Step B/C arithmetic. The defect is that the **decision model has no enveloping branch at all.**
**Impact:** (a) enveloping mode — the default and generally superior mode against a ship with one weak or down shield — is never evaluated, so the engine understates hellbore output and never advises "fire enveloping to exploit the down #3"; (b) the soak comparison is wrong for enveloping fire, which bypasses the strong facing shield entirely and produces its own separate volley (E10.441) — an extra Mizia volley the `commitment()` math ignores. Both feed `optimal_band` (pref hellbore=15 at `sfb_command.py:980`) and threat assessment, so range and engage/avoid advice against Hydrans is wrong in either direction.
**Fix:** add an enveloping branch returning a per-shield distribution (or at minimum an expected-internals figure that bypasses the facing shield); have `absorb_capacity`/`expected_damage` consume it; fix the misleading E10.7 comment.

#### DM3 — Only the Cadet DAC exists, and it is dead (MINOR, D4.21/D4.221) — *needs playtest*

> "(D4.221) For each damage point of the volley, roll two dice and find the resulting number in the 'die roll' column of the DAMAGE ALLOCATION CHART (D4.21)."

**Now:** `sfb_rules.py:387-407` defines `DACEntry` and `CADET_DAC` (the 1d6 simplified chart). A grep over all modules finds no other reference to either — dead code. No module estimates what a volley of N internals will destroy.
**Honest scoping:** the client resolves damage and enforces D4.321, so **no order the engine issues today is illegal or wrong because of this**, and the qualitative Mizia tradeoff (mini-volleys strip weapons vs one massive volley kills) is already modelled in `sfb_doctrine.py:50-66`, `sfb_situations.py:30-59`, `sfb_advanced.py:94-99`, `sfb_command.py:1590`. Downgraded from the headline gap it first appeared to be.
**Residual value:** a 2d6 DAC with an `expected_hits(volley_size, target_boxes)` function would let `commitment()` answer "will this volley take his photons off" instead of reasoning about raw damage totals. Worth doing, but it is an enhancement, not a bug fix. If implemented, use the 2d6 Captain's chart with column A-F chains and D4.321 phaser directionality — not the Cadet chart.

#### DM4 — Boarding parties / hit-and-run / capture absent (MINOR, D7.0) — *needs playtest*

**Now:** no boarding-party, marine or crew-unit model exists. `sfb_crew.py` is LLM bridge-officer roleplay; `sfb_deckcrew.py` is J4.81 shuttle deck crews. A grep for boarding/hit-and-run/transporter/capture returns only prose strings fed to the LLM (`sfb_doctrine.py:99`, `sfb_situations.py:217`, `sfb_ai.py:46`). The emitted order vocabulary (`sfb_ai.py:107-146`: POSTURE/SPEED/ECM/REPAIR) has no transporter or boarding action, so hit-and-run raids and capture are never advised as concrete orders.
**Two corrections to the original framing:** (a) D4.223 is a generic system-box-marking rule and says nothing about crew; (b) D9.2 damage control is **capped by the SSD damage-control track and consumes ENERGY** ("For each two units of energy allocated to damage control, the damage scored to one shield box may be erased at the end of the turn" — D9.21), *not* boarding-party strength. The claim that crew gates the repair rate is wrong.
**Real residual:** D7.0 raids and capture are unmodelled and would change orders (whether to close to transporter range). Also incidental: `sfb_ai.py:113` asks for "REPAIR=\<0-9 in multiples of 3\>", which matches neither D9.21's 2-energy-per-shield-box arithmetic nor the DC-rating cap — a separate wrong-numbers issue.
**Per-ship crew/BP counts cannot be cross-checked** — SSD books extracted empty.

---

### 3.7 Electronic warfare

This is the tightest cluster in the audit: three defects that compound into "the engine believes the enemy has no EW and that its own is free and unlimited."

#### EW1 — Enemy ECM never parsed (MAJOR, D6.32 / D6.315) — **root cause of EW2**

> "(D6.32) ANNOUNCEMENT (Standard): In the sensor lock-on segment of each turn, players announce their ECM and ECCM strength (the number of energy points expended)."
> "(D6.315) ADJUSTMENT: The power used by each unit for electronic warfare can be adjusted each impulse as part of the Fire Decision Step…"
> "(D17.194) The EW levels of all units are always known (D6.32)."

**Now:** `sfb_advanced.py:113 enemy_ecm = getattr(enemy, "ecm", 0) or 0`. A repo-wide grep finds exactly **one** assignment of an `ecm` attribute anywhere — `sfb_tactics.py:256 eaf.ecm = 2`, which is the *friendly* EA output, not observed enemy EW, and itself a hardcoded constant. `sfb_log.py`, `sfb_ssd.py`, `sfb_console.py`, `sfb_client.py`, `sfb_bridge.py`, `sfb_advisor.py` contain no ecm/eccm token at all.
**Consequence:** `enemy_ecm` is 0 on every call, forever. `eccm = min(pool, enemy_ecm) if enemy_ecm else 0` → always 0. The note always renders "+0 shift" even against 6 ECM (+2) or a Wild Weasel. The whole 6-point pool goes to ECM.
D17.194 makes enemy EW public information, so this is a straightforward missing-parser bug, not hidden information.
**Fix:** add an EW-announcement parser to `sfb_log.py` (lock-on phase plus mid-turn ECM/ECCM lines) storing per-unit ecm/eccm with the impulse set; feed it into the ship view used by `recommend_ew` and fire ranking. **This is the single highest-leverage fix in the EW domain — EW2 cannot self-correct without it.**

#### EW2 — ECCM never allocated by the EA path (MAJOR, D6.310 / D6.34)

> "(D6.310) GENERATION: Ships may use energy for ECM or ECCM. … Energy must be designated for use as ECM or ECCM at the time it is allocated (or when reserve power is committed)."

**Correction to a common misreading:** ECCM *is* modelled and *is* advised — `sfb_ew.py:30-34` implements the D6.34 subtraction (`effective_ecm = max(0, target_ecm - firer_eccm)`) and `sfb_advanced.py:110-119` splits the pool, surfaced at `:243` as "EW: set ECM {n} / ECCM {n}".
**The real defect is two-part:** (a) that recommender is pinned to ECCM 0 by EW1; (b) `sfb_command`'s energy-allocation output has **no ECCM line at all** and cannot express the split — `:128` sets `ew = 2 if threatened else 0` and `:184-188` dumps surplus into ECM up to `ECM_USEFUL_CAP = 4`. So the two advice paths also disagree about whether ECCM exists.
**Impact:** against an opponent running 4-6 ECM the advisor always says buy ECM instead of the ECCM D6.34 needs to clear the shift off your own shots. Legal, persistently suboptimal.
**Fix:** replace the flat ECM line in the EA builder with a call to `recommend_ew`; print both values.

#### EW3 — Order engine never computes an EW shift (MAJOR, D6.35 / E1.811)

> "(D6.35) EFFECT ON DIRECT-FIRE WEAPONS: In the case of direct-fire weapons (E0.0), the effect of ECM/ECCM is determined at the instant of firing. Electronic warfare produces a die roll shift; see (E1.8)."

**Now:** `sfb_command.py` imports only `sfb_hex`, `sfb_rules`, `sfb_log`, `sfb_situations` (`:18-24`) — **`sfb_ew` is absent**. `grep -l sfb_ew *.py` returns exactly one file, `sfb_advanced.py:17`, reached only via `sfb_brain.py:23` — a different entry point. `sfb_command.expected_damage` (`:710-755`) multiplies raw chart damage by hit fraction with no shift term, and that function feeds `commitment()` (`:783`), `threat_assessment()` (`:1502`), `incoming_at()` (`:1726`), `likely_engagement()` (`:1740`), `shield_advice()` and `reinforce_plan()` — i.e. range choice, target ranking, and reinforcement sizing.
**Impact:** overloaded alpha strikes ordered at ranges where the shift makes them near-worthless; a heavily EW-protected ship looks identically vulnerable to an unprotected one; `commitment()` recommends exchanges that are actually losing. (Not a *legal* problem — the client rolls the dice.)
**Fix:** import `sfb_ew` in `sfb_command`; compute `shift = EW.net_shift(target_ecm, my_eccm)` per target and run each weapon's damage through `EW.degrade_expected` before ranking targets and recommending fire. Depends on EW1 for real inputs.

#### EW4 — `SENSOR_EW_POOL` is a constant 6 (MAJOR, D6.3141)

> "GENERATED: Points received for power expended by the ship … The total of both ECM and ECCM cannot exceed the highest unchecked box on the sensor track (usually six). Note that the total is six; a ship cannot generate six ECM and six ECCM…"

**Now:** `sfb_ew.py:20 SENSOR_EW_POOL = 6`, commented "undamaged ship" — used directly at `sfb_advanced.py:111-119` for both the ECCM cap and the ECM remainder. Nothing reads a sensor-box count from SSD, save, or damage log. Sensor boxes are among the first things hit by damage allocation, so it goes stale fast.
**Impact:** on a ship down to 2-3 sensor boxes the engine still tells the player to buy 6 points — an illegal allocation, and 3-4 points misbudgeted away from reinforcement or movement. It also feeds an inflated `eccm` into the shift math, so the advisor believes it has cleared an enemy ECM shift it has not.
**Caveat:** per-ship sensor-track length cannot be verified here (SSD books empty). The fix must read remaining unchecked sensor boxes from live damage state, not substitute a different constant.
**Fix:** make the pool a per-ship live value; pass it into `recommend_ew`.

#### EW5 — Lock-on rolls for tractor / transporter / SFG (MAJOR, D6.371/.372)

> "(D6.372) PROCEDURE: For each individual action (i.e., for each of three transporters used on the same impulse), roll a single die and add the net ECM shift to the result. If the total is more than six, the lock-on is not strong enough and the system cannot be used."

**Now:** absent. `sfb_ew.net_shift()` feeds only to-hit/damage degradation. No lock-on roll exists and no D6.37x citation appears anywhere. `sfb_command.py:493-515` offers "tractor x{n}" as unconditional seeker defence with no EW gate; `sfb_doctrine.py:99` and `sfb_situations.py:627` recommend transporters / the Gorn Anchor ungated; `sfb_advanced.py:172` is bare prose ("an ECM shift can make tractors fail") with no numbers.
**Correct:** each tractor, transporter and stasis-field attempt needs d6 + net shift ≤ 6; a failure cannot be retried with that box for **eight impulses** and the allocated energy is lost. Carve-out: G7.412 makes lock-on automatic once a tractor link is already established, so the roll applies to initial attachment only.
**Impact:** the roll resolves *after* energy allocation, so a failure sinks the energy and locks the box — which changes how many transporters/tractors to power and whether to build a plan around one landing. Against 4-9 ECM each attempt has a 33-50% failure chance, and a failed Gorn Anchor is often game-losing.
**Fix:** add a d6+shift>6 success model keyed off the target's net shift; annotate every tractor/transporter/SFG recommendation with the probability and the box-lockout consequence.

#### EW6 — Scout EW lending is free text (MINOR, D6.392 / D6.3144)

> "No more than six units of ECM and no more than six units of ECCM may be received (D6.3144) by any unit from all outside lending sources, such as scouts, MRS or SWAC shuttles, ECM drones, or wild weasels (J3.23)."

**Now:** `sfb_command.py:824, 843-845` returns a fixed tuple — "stay out of the line — lend ECM/ECCM to the flagship, hold the sensor picture" — an unconditional constant string with zero live inputs: no recipient EW state, no quantity, no channel count. `sfb_ew.py` has no concept of lent EW at all.
**Important rules correction:** lent EW does **not** lapse when lock-on is lost. D6.3172, verbatim: "unless voluntarily dropped (if permitted), lending units from (D6.3144) will continue to lend their ECM or ECCM points to the receiving unit, even if the receiving unit, for whatever reason is unable to currently use them (e.g., the receiving unit moves out of range, the lending unit doesn't have a lock-on to the receiving unit, the receiving unit cloaks or launches a wild weasel, etc.)." The points are **wasted, not released** — the channel stays committed. An engine that auto-released the channel on lost lock-on would give *illegal* advice. Only the WW case (D6.3171) is a temporary suspension with restoration. Also, the six-point cap is per ECM and per ECCM separately, not a combined budget.
**Impact:** the order names no quantity so it cannot be illegal; what is lost is order *quality* — the advisor cannot say how many points, on which channel, to which recipient, nor detect the redundant case where the flagship is already receiving its six (from a WW or a second scout), where the right order is to lend elsewhere or switch to offensive EW.
**Fix:** give the scout posture a concrete figure clamped to `min(6, ceiling - already_received)` per recipient and per EW type; track channel count. Do **not** implement auto-release on lost lock-on.

#### EW7 — ECM hard-wired to 2 points (MINOR, D6.34)

> "(D6.34) STEP 5 … Players may recognize this calculation as taking the square root and dropping all fractions."

**Now:** `sfb_tactics.py:254-257`: `if profile.use_ecm and not retreating and power >= 2: eaf.ecm = 2`. A hard constant, never keyed to enemy ECCM, never scaled to 4 for a second shift. `sfb_ew.points_to_shift` correctly uses `isqrt`, faithful to the rule: 1-3 → +1, 4-8 → +2, 9-15 → +3.
**Nuance:** the second point is **not** pure waste — against 1 point of enemy ECCM, 1 ECM nets 0 and buys no shift while 2 keeps the +1. Two points is also the quantum the rulebook's own D6.33 example uses. The real defect is that the allocation is *blind to observed enemy ECCM* (2 ECM vs 2+ enemy ECCM buys literally nothing and should be spent elsewhere) and never considers 4 for +2 with spare energy.
**Fix:** replace the literal 2 with a threshold picker over {0,1,2,4,9} using `points_to_shift` against live enemy ECCM; mirror `recommend_ew`'s split. Blocked on EW1.

---

### 3.8 Scenario and campaign

#### SC1 — Disengagement-by-acceleration ignores the 15-point clause (MAJOR, C7.11)

> "At the end of that turn, if the starship in question still has total warp power available equal to either 50% of his original warp power (rounding fractions up) **or fifteen points of warp power, whichever is lower**, the owning player simply announces that he is 'disengaging.'"

**Now:** `sfb_command.py:1435-1444` computes `frac = warp/warp_max` and passes only that; `sfb_situations.py:534-548` tests `elif warp_fraction >= 0.5`. The failure message even says "need >=50% or 15" while never testing the 15-point alternative — the absolute count is never passed in, so it cannot be implemented downstream.
**Impact:** any ship with original warp above 30 — DN/BB/BCH — is told disengagement is BLOCKED when it is legal. A ship at warp 16 of 40 is 40% but legal under the 15-point clause. One-directional: it never permits an illegal disengage, it only denies legal escapes from losing positions.
**Fix:** pass `warp_original`; compute `need = min(ceil(0.5*warp_original), 15)` and compare warp at turn start. Report actual box counts in the message. (The C7.121 "beginning of turn" point is already surfaced as prose, though the numeric test still uses current warp.)

#### SC2 — C7.22 seeker block ignores the endurance escape (MAJOR, C7.22)

> "If there is no possibility of the seeking weapon catching the escaping unit (i.e., escaping unit is faster, or the weapon is faster but does not have the endurance to close the range), this does not apply."

**Now:** `sfb_command.py:1434 can_catch = any(int(s.get("speed") or 0) > my_speed …)` — speed half only. Passed as `seekers_can_catch` into `disengage_check`, which appends "unresolved seeking weapons capable of catching you (C7.22)" and removes separation from `available`.
**Impact:** falsely reports separation blocked when a spent drone trails out of endurance, so the engine advises staying on the board instead of taking a legal free disengagement. Conservative over-blocking.
**Fix:** two-part and dependent on S4 — the parser must track each seeker's launch turn / remaining endurance before `can_catch` can add an endurance term.

#### SC3 — Weapons Status entirely unimplemented (MAJOR, S4.0)

> "(S4.10) WEAPONS STATUS 0: … Phasers not energized (E2.3), no energy in phaser capacitors (H6.0). No torpedoes (or other multi-turn arming weapons) are armed. No special shuttles (scatter-pack, wild weasel, or suicide shuttles) may be prepared. Drone racks and plasma-F launchers … are loaded. No energy may be stored in ESG systems (G23.23). … **The batteries are fully charged.**"

**Now:** a grep for weapon_status / ws_level / "S4." finds one hit — `sfb_situations.py:580`, doctrine prose. Nothing carries a WS value; no parser reads one. `sfb_command.py:1094-1095` hardcodes the assumption in code and comment: "The capacitor is full at scenario start and stays full until you fire (H6.1)", and `fired_phasers` comes from the log, so on turn 1 it is always False and `compute_eaf` never budgets a capacitor charge.
**Correction:** batteries are fully charged at **every** weapon status including WS-0, so `batt_room = 0` / "batteries already charged" is correct at all statuses and is *not* part of this gap. Only the phaser-capacitor assumption is defective.
**Correct by status:** WS-0/I — capacitors EMPTY (turn 1 must pay the full charge), no prior arming, no SP/WW/suicide shuttle, no ESG charge; drone racks and plasma-F **are** loaded. WS-II — capacitors may be full, all prior-turns-but-the-last arming done, one special shuttle, two fighters as CSP within 2 hexes, ESG charged. WS-III — all fighters armed, four deployed within 2 hexes, multi-turn weapons fully armed and HELD (holding energy must be allocated turn 1), drones may be pre-launched within 4 hexes but not within 3 of an enemy.
**Impact and blast radius:** turn-1 EA under-allocates by up to a full capacitor (11 points on a large ship) at WS-0/I, and turn-1 tactical advice can recommend a scatter-pack, weasel or suicide shuttle that cannot exist. **But at WS-II/III — tournament play and most published scenarios — the hardcoded assumption is exactly right.** The gap bites only scenarios that roll or specify a low status.
**Fix:** add a scenario-context `weapon_status` field plus a WS table in `sfb_rules.py` giving per-status booleans (capacitor_full, prior_arming_turns, special_shuttles_allowed, esg_charge_allowed, held_multiturn, fighters_prearmed, csp_count). Gate `sfb_actions` shuttle recommendations and turn-1 EA on it.

#### SC4 — `_is_crippled` uses 50% warp (MINOR, S2.41)

> "A ship is crippled when: A: 10% or less of its original warp engine boxes are undestroyed. Ships that have no original warp power, e.g., Romulan Warbirds, are never crippled under this heading. Bases of size class 3 or larger are crippled when 10% or less of their total power-generation … remains, bases smaller than size class 3 are crippled when half or less … B: 50% or more of interior boxes destroyed; does not include shields, armor, sensor, scanner, DamCon, or excess damage."

**Now:** `sfb_command.py:1333-1339` returns crippled on `hull <= hmax*0.5` OR `warp <= warp_max*0.5`. The docstring admits it is "roughly 'cannot keep fleet speed'".
**Only the warp branch is mis-numbered** — the 50% hull test is a fair proxy for criterion B, which really is a 50% threshold. The `warp_max`-vs-original complaint is unverified.
**Impact:** feeds `uncrippled_enemies_within_15` in the C7.3 sublight-evasion modifier (`:1445-1447`), under-counting pursuers and reporting an easier evasion roll; and target-priority doctrine (`sfb_situations.py:780-781`) tells the player to finish off "cripples" that are not crippled, score no crippling points, and still fight at strength. Own ships are declared cripples far too early, triggering premature break-off.
**Also unmodelled:** ships with no original warp (Warbirds are never crippled under heading A) and base size-class variants.
**Fix:** split into a rules-accurate `_is_crippled` (warp ≤10% of original, OR ≥50% interior destroyed) and a separate `_cannot_keep_fleet_speed` heuristic for the doctrine layer.

#### SC5 — Victory conditions not modelled (ABSENT, S2.2) — *needs playtest*

> "(S2.21) VICTORY POINTS RECEIVED: … For scoring any internal damage = 10% of BPV / For forcing a ship to disengage = 25% of BPV / For crippling an enemy ship = 50% of BPV / For destroying an enemy ship = 100% of BPV / For capturing an enemy ship = 200% of BPV … Only one of the above (the greatest) may be scored for each enemy ship."

**Now:** a grep for BPV / "victory point" over all 33 modules returns **zero** hits. No BPV field, no percentage bands, no score differential, no S2.3 victory-level ratio.
**Fair scoping:** the engine *does* model CRIPPLED and DISENGAGE as live tactical states (`sfb_command.py:1333`, `:925`, `:1410`), which act as a rough proxy for the same pressures; what is missing is the BPV *scoring* of those states. Nothing produced today is illegal or self-defeating because of this. And S2.202 notes "Many scenarios ignore (S2.2) altogether and define victory in terms of specific actions or accomplishments," so the bands are the default frame, not universal.
**Residual:** the engine can recommend a line that survives but loses on points — trading a cripple for a cripple, or refusing a cheap internal-damage hit that would bank the 10% band — and cannot advise the standard endgame decision of running a damaged ship off the board to protect the score.
**Blocked on data:** per-ship BPV cannot be sourced from `rules_txt` (SSD books empty). It must come from the Master Ship Chart / Annex #1, which is not present in the corpus, or from the save file.

---

### 3.9 Doctrine (ADB5703 — Tactics Manual, doctrine not rules; nothing here is illegal)

#### DO1 — Klingon saber-dance unreachable (MAJOR overall, MINOR for the transcription)

> "One tactic often used is the 'Klingon Saber Dance.' Here, the disruptor-armed ship maneuvers at long range, moving into the 9-15 hex range bracket to deliver a narrow salvo of disruptors (followed by a volley of phasers), meanwhile staying out of overload range."

**Now:** `sfb_doctrine.py:135-136` says "saber-dance at 13-15 hexes where disruptors beat photons", discarding 9-12 for no rules reason. The engine's *own* glossary at `sfb_doctrine.py:124` already states the correct principle ("Attrition via repeated attacks from beyond overload range"), so this is an internal inconsistency too.
**The load-bearing half is the second one:** `BASE_PROFILE["KLINGON"] = Profile(2, ...)` (`sfb_tactics.py:62`), and `compute_profile` can only push it *down* — `short_only` clamps to `min(p,3)` at `:88`, AGGRESSIVE posture subtracts 1 at `:100`. Nothing anywhere raises a Klingon toward 9-15. So the movement layer steers to knife range while the advisor prints doctrine telling the human to stand off. **No code path can ever produce saber-dance range.**
**Fix:** correct the string to 9-15 / "outside overload range"; expose a SABER_DANCE posture setting `preferred_range` 9-12 for disruptor ships.

#### DO2 — `plan_eaf` reinforces before charging phasers (MINOR)

> "Electronic warfare is always a useful place for otherwise unused energy points. Any way you look at it, shield reinforcement should be the last refuge for your energy… Reinforcement should be used when and as needed; it should not become a dumping ground for homeless energy points." (p8) — "Putting reserve power into general shield reinforcement is ineffective with one shield down and can be countered by minimal damage scored on a different shield." (p11)

**Now:** `sfb_tactics.py:259-272` — step 6 takes up to 4 points of **general** reinforcement on the bare condition `me.hull_pct < 100`, i.e. reflexively for the rest of the battle after first blood, and sits **ahead** of step 7 which fills phaser capacitors from what remains. No shield-down check; no EW/HET/tractor/transporter/battery alternative (the manual explicitly offers all of those). Step 5 caps ECM at a flat 2.
`sfb_advanced.energy_plan` (`:181-199`) encodes the opposite, correct order and warns against "reflexive shield reinforcement" — another advisory-vs-order-path contradiction.
**Narrowing:** step 9's leftover-to-*directional*-reinforcement is defensible — it runs after phaser capacitors, and directional reinforcement on the threat facing is not the "general reinforcement" the manual condemns. Only step 6 is the defect.
**Fix:** move the phaser-fill block above the general-reinforce block; gate general reinforcement on the threatened shield still being up AND no better sink existing.

#### DO3 — No target selector ranks by shield state; volleys never split (MINOR)

> "Mizia Concept: A theory under which the most effective attack is a series of small volleys against the same down shield." — glossary, p11 article

**Now, confirmed:** none of the three target selectors consults shield state. `sfb_tactics.py:196-213` scores `dmg / max(1, e.hull)`; `sfb_advanced.py:221-224` scores `alpha_strike * (1 + (1 - hull_pct/100))`; `sfb_command.py:977` picks purely by proximity (`min(enemies, key=hex_distance)`). And no module splits direct fire into deliberate mini-volleys, or separates seeking / direct-fire / T-bomb volleys in the emitted order.
**Correction — Mizia is *not* absent:** `sfb_command.py:983-986` and `:1160-1161` read live `tgt["shields"][es] / shields_max[es]`, compute `es_down`, and override the fire line with "his {shield} is DOWN — concentrate {fam} + phasers (Mizia)". `sfb_advanced.py:93-103 mizia_recommendation()` branches on live `facing_shield_is_down()`. Inputs are live-parsed and do update. So the earlier framing ("prose appended after the target is chosen") was wrong.
**Residual, and it is genuine:** in a multi-enemy fight the engine will not prefer the ship *showing* a down shield over a nominally thinner-hulled one — the manual's "a medium-range shot on a down shield beats a short-range shot on a strong shield" is quoted in `sfb_doctrine.py:74` and never applied to selection. The manual also cautions Mizia can be counterproductive when the goal is a kill.
**Fix:** add a term keyed on `hexlib.shield_hit(e.pos, e.facing, me.pos)` shield value to `plan_fire` and `assess.score`.

#### DO4 — No bolt-vs-launch state, so the plasma Glory Zone is unreachable (MINOR) — *needs playtest*

> "Plasma bolts can be used effectively in the 'Glory Zone' (range 9-10). At this range they have a 50% chance of a hit, the same warhead strength (except for types-F and -D) as bolts fired from closer ranges, and you are outside of the enemy's overload range. Here you must use the Oblique Attack to ensure that the enemy can't close to 8 hexes while you are turning away."

**Now:** `sfb_tactics.py:59-69` ROMULAN `preferred_range = 7`, GORN `= 4`; `compute_profile` (`:78-90`) only does `max(preferred_range, 5)` for plasma, so Romulan stays 7 and Gorn becomes 5. `choose_heading` (`:126-146`) scores neighbours by `10.0 - abs(nd - preferred_range)`, actively steering to sit there. `sfb_doctrine.py:136-138` prints "Fight in the plasma glory zone (bolt r9-10)".
**Important scoping:** the Glory Zone applies to **bolted** plasma only. A plasma ship holding launched torpedoes generally wants to close — a torpedo launched at r9-10 is trivially weaseled or outrun. So unconditionally steering to 9-10, as first proposed, would be *worse* for a launching Romulan. The manual also notes the tactic "can be countered, to some extent, with electronic warfare, normal loads on some heavy weapons, and speed changes."
**The confirmed defect is narrower:** `sfb_tactics` carries no bolt-vs-launch input at all, so `preferred_range` is a static constant that can never reach the glory zone even when the prose the engine prints tells the human to fight there.
**Fix:** add a bolt/launch posture; set `preferred_range` 9-10 for bolt mode only; keep close range for launched-torpedo and anchor postures; add the Oblique Attack constraint to `choose_heading` (reject headings that let the enemy reach range 8).

---

## 4. Implementation plan

Batches are ordered so each is independently testable in a real game. Effort is rough coding time, not including playtesting.

---

### Batch 1 — "Wrong today in a Kzinti-vs-Lyran carrier fight" (~1-1.5 days)

Every item here fires in that specific matchup on turn 1-3. This is the batch that stops the advisor lying about the state of your own air group.

| Item | Files | Why it fires in this fight |
|---|---|---|
| **F1** Parse fighter landings | `sfb_log.py` (new regex), `sfb_actions.py:77-84` | Kzinti CVs cycle their group. After the first recovery the engine says "deck clear" forever and never orders a second launch. |
| **F4** Stop debiting shuttle launches from the fighter pool | `sfb_log.py:122-128`, `sfb_actions.py:78-83` | Kzinti fly weasels against Lyran drones and scatter-packs against ESGs. Two weasels on an escort and the engine reports the fighters gone. Minimum fix: stop defaulting unknown Type strings to 'fighter'; exclude units the engine itself just advised as WW/SS/SP. |
| **F3** Fix the inverted `fighters_out` | `sfb_command.py:1033,1056,1076`; `sfb_carrier.py:130` | Right now the whole Kzinti squadron is ordered to crawl at speed 8 *while the fighters are still in the bay* — and the escorts get MATCH THE CARRIER. This single line is costing you the approach. |
| **F2** Thread `log` into `carrier_advice` | `sfb_command.py:1244-1266` | Removes the contradiction where one panel says "deck clear" and the panel beneath it prints a 12-fighter launch schedule and a D12.0 warning. Cheap once F1 lands. |
| **S1** `rack_drone_profile(ship)` | `sfb_actions.py:128-149`, `sfb_command.py:441` | Lyrans and Kzinti both throw drones. The launch/hold gate currently keys off your own throttle. |
| **S5** FA-arc gate + per-rack rate | `sfb_actions.py:128-149`, `sfb_command.py:1165` | Stops "LAUNCH DRONES" at a target astern and stops re-issuing the same rack every impulse. |
| **E1** Shield cost chart → Total column | `sfb_rules.py:452`, comment at `sfb_command.py:86` | 2-point EA error on every cruiser, every turn, both sides. Confirm SC2 against a clean scan; leave SC4/SC5 at 1 (D3.321). |
| **E4/DM1** Skip down shields in `reinforce_plan` | `sfb_command.py:1817-1836` | Lyran ESG passes and Kzinti drone waves knock shields down fast; the current tie-break actively *prefers* the weaker neighbour. |
| **D3** Photon minimum range gate | `sfb_actions.py:181` | Only if a Fed is on the table — but it is a five-line fix, take it. |

**Watch for in-game:** launch a fighter group, recover it, and confirm the console's "fighters remaining" goes back **up** and a second launch is offered. With the group in the bay, confirm the carrier is no longer capped to speed 8. Launch a wild weasel from a 2-fighter escort and confirm the fighter count is unchanged. Point the ship away from the enemy and confirm the drone order becomes "TURN TO BEAR then launch". Check a cruiser's EA line shows 4 points of shield housekeeping, not 2. Knock a shield to zero and confirm the reinforcement order moves to a live facing (or switches to general).

---

### Batch 2 — Weapon charts and reload granularity (~1 day)

The engine currently has two disagreeing sets of weapon charts and a reload model that mutes whole weapon families.

- **D1** Delete `_PH1_TBL`/`_PH2_TBL`/`_DISR_TBL` from `sfb_rules`; move `sfb_command`'s sourced `DISR_STD/PHOT_STD/PH*_EXPECTED` into `sfb_rules` and have `sfb_command` import them. **One chart per weapon in the codebase.**
- **D2** Route `damage_at`'s overload branches through `photon_overload_damage()`; add the missing `DISR_OVL` table (10/10/8/8/6); allow R0-1 overload damage instead of returning 0.
- **D4** Retain the fired *count* in `sfb_log.py:147` (currently discarded), then report "FIRE k of n DISRUPTORS (m reloading)" in `sfb_actions` and `sfb_command:931-960`. Note: full per-tube identity may not be recoverable from the log — count-based is the honest ceiling.
- **D5** Pass `impulse, turn, log` into `phaser_actions`; apply the same lockout; enforce E2.22 once-per-turn; exempt ph-G.
- **D6** Fix the six `E4.24` → `E3.24/E3.51` citations.

**Watch for:** fire two of six disruptors as a ranging shot and confirm the other four are still offered next impulse. Confirm a disruptor boat is given a firing solution beyond range 15 (out to 40). Confirm a phaser-3 is not declared useless at range 3. Confirm an overloaded disruptor at R6 is valued at 6, not 12.

---

### Batch 3 — Electronic warfare (~1-1.5 days, EW1 first)

EW1 is the keystone: EW2, EW3 and EW7 are all downstream of it and cannot be verified until enemy ECM is a real number.

1. **EW1** Add an EW-announcement parser to `sfb_log.py` (lock-on segment plus mid-turn ECM/ECCM lines); store per-unit ecm/eccm with the impulse. D17.194 makes this public information.
2. **EW3** Import `sfb_ew` into `sfb_command`; run `expected_damage` through `EW.degrade_expected` before target ranking and fire recommendation.
3. **EW2** Replace the flat ECM line in the EA builder with `recommend_ew`; print both ECM and ECCM.
4. **EW4** Make `SENSOR_EW_POOL` a per-ship live value from remaining unchecked sensor boxes.
5. **EW7** Threshold picker over {0,1,2,4,9} against live enemy ECCM.
6. **EW5** d6+shift>6 lock-on model annotating every tractor / transporter / SFG recommendation (with the G7.412 carve-out for an established link).

**Watch for:** run 6 ECM on one side and confirm the other side's brief reports a +2 shift and recommends ECCM. Take sensor damage and confirm the advised EW total falls below 6. Attempt a Gorn Anchor against a high-ECM target and confirm the advice now quotes a success probability and warns about the box lockout.

---

### Batch 4 — Seeking weapons beyond the launch gate (~1 day)

- **S2** Join `state["seeking"]` with `RE_ADDED_LAUNCH` attribution to count outbound channels; gate the LAUNCH headline. Implement F3.211 halving and F3.212 doubling together — a naive ceiling of 6 makes a Kzinti CV wrongly conservative.
- **S3** Branch `incoming_seekers` on `s["kind"]`; add an FP1.53 plasma table keyed on type and hexes travelled; switch `seeker_defence` to attrition language and the correct answers (weasel, ESG is no help per G23.81, outrun, absorb).
- **S4** Attach launch turn/impulse to seeker records; drop or flag seekers whose remaining endurance < time to impact.
- **SC2** Add the endurance term to `can_catch` (depends on S4).
- **S6** Make `weasel_advice` consume plotted maneuver rate and the impulse's other recommendations; flag J3.131/J3.132 conflicts.

**Watch for:** with six drones out, confirm a seventh launch is refused on channels. Take an incoming plasma-S and confirm the brief reports its decayed strength (not a flat 12) and describes phasers as *reducing* the warhead rather than killing it. Let a drone run out of endurance and confirm it stops appearing as a live threat and stops blocking disengagement.

---

### Batch 5 — Energy and movement corrections (~0.5-1 day)

- **E2** Delete `MANDATORY_POWER`; have `sfb_tactics`/`sfb_advanced` call `compute_eaf`'s size-keyed logic.
- **E3** Track live battery charge; make `batt_room = capacity - charge`; rank recharge above the ECM sink.
- **E5** Gate the anti-general-reinforcement line on the enemy weapon list; let `reinforce_plan` emit a general line vs hellbore/enveloping plasma.
- **M3** Delete the impulse-1 HET refusal; move it into `tac_assessment` where C5.11 puts it.
- **M1** `since_slip = 1` on a turn event; derive slip mode from speed rather than hardcoding 1.
- **M2** `turn_mode(..., em, nimble)` and thread an EM flag.
- **M4** Special-case speed 0 (not `<=1`); offer HET/TAC/speed-1.
- **M5** Parse reverse/breakdown/emergency-decel; record `accel_base`.
- **SC1** Pass `warp_original`; `need = min(ceil(0.5*orig), 15)`.
- **SC4** Split `_is_crippled` (10% warp, 50% interior) from `_cannot_keep_fleet_speed`.

**Watch for:** turn and then immediately sideslip — confirm the order is offered. Request a HET on impulse 1 and confirm it is now evaluated on cost and breakdown risk. Tap reserve power, then confirm next turn's EA offers a battery recharge line. Damage a DN to warp 16 of 40 and confirm disengagement-by-acceleration is reported as legal.

---

### Batch 6 — Structural / larger work (~2-3 days, do last)

- **DM2** Enveloping hellbore branch in the decision model (per-shield distribution or expected-internals bypassing the facing shield).
- **F5** Per-airframe rearming state machine; wire `crew_load_summary` into the carrier panel. **Depends on F1.**
- **F6** Crippled-fighter speed halving; fix the mislabelled `known=True` race fallback in `fighter_speed`.
- **SC3** Weapons Status as a scenario input plus a per-status table in `sfb_rules`.
- **DO1/DO2/DO3/DO4** Doctrine corrections: saber-dance band and a SABER_DANCE posture; phaser capacitors before general reinforcement; shield-state term in target selection; bolt-vs-launch posture for the Glory Zone.
- **EW6** Concrete scout lend figures clamped to the D6.3144 cap. **Do not** implement auto-release on lost lock-on — D6.3172 says the points stay committed.
- **DM3/DM4/SC5** 2d6 DAC, boarding parties, BPV scoring. All need external data (Annex charts) and a playtest to justify; treat as enhancements.

---

## 5. Open questions needing a game

1. **General reinforcement vs hellbores (E5).** The rule is unambiguous that GR is subtracted from weapon strength before division. What is *not* settled is the crossover point — at what enemy hellbore count does GR at 2 energy per point beat specific reinforcement at 1 energy per point on the facing shield? A Hydran game would settle the threshold the advice should use.
2. **Wild weasel interaction budget (S6).** The rules give the constraints; what needs a live check is whether the engine should *suppress* the conflicting recommendation or merely flag it. Suppressing a drone-launch order because a weasel is charging may be more annoying than useful when the player intends to abort the weasel.
3. **Plasma bolt-vs-launch posture (DO4).** The Glory Zone is a bolt tactic; the code has no bolt/launch state. What proportion of the time you actually bolt versus launch — and therefore what the default `preferred_range` should be when the mode is unknown — is a playstyle question the rules do not answer.
4. **DAC-based commitment scoring (DM3).** Whether replacing raw-damage `commitment()` with expected-boxes-killed actually improves the engage/avoid verdict, or just adds variance, needs a real comparison. The Cadet chart is definitely the wrong chart; whether the 2d6 chart is worth the wiring is an empirical question.
5. **Boarding parties and capture (DM4).** Whether hit-and-run raids are worth adding to the order vocabulary at all depends on how often they are live in your games. Currently they exist only as LLM prose.
6. **BPV scoring (SC5).** Blocked on data — per-ship BPV is not in `rules_txt` (SSD books are image scans) and must come from Annex #1 / the Master Ship Chart or the save file. Before doing that work, confirm your scenarios actually use S2.2 standard victory rather than the scenario-specific conditions S2.202 says are common.
7. **Fighter speeds beyond Kzinti (F6).** `FIGHTER_SPEED` holds four Kzinti entries; everything else falls back to an assumed 8 presented as fact. The Master Fighter Chart is in the G3 Annex and would have to be transcribed by hand.
8. **Shield cost SC2 (E1).** The SC2 row in the text extract is column-garbled; only the minimum "=1" is legible. Confirm the Total against a clean scan before hardcoding 5.
9. **Sensor track length per ship (EW4).** Not verifiable from the corpus. The fix must read remaining unchecked boxes from live damage state rather than substitute a constant — confirm the save file exposes sensor boxes.
10. **Whether shuttle boxes are differentiated per ship (F4).** J1.4 says the pool is shared by default and separate only where the SSD or section R differentiates. Which of your ships are which cannot be determined from the extracted rules — needs the SSDs.

---

## 6. Refuted (considered and rejected)

- **"TACs and HETs are never offered — the manoeuvre module is unreachable."** Bad grep. `sfb_command.py:1234` calls `MAN.maneuver_advice`, itself called at `:1184`; the module is also hot-reload registered. Residual is tiny: `het_advice` gates on `reason in {escape, shield}`, so a plain TAC for firing-arc reasons is never proposed. MINOR at most.
- **"J1.52 once-per-turn and 1/4-turn gap quoted but never enforced."** `LAUNCH_LAND_GAP` is indeed prose-only, but once-per-turn launch is *effectively* enforced by the per-airframe log tally, and the 8-impulse delay plus the J1.62 exception are surfaced verbatim to the operator. No illegal or losing advice results.
- **"Mizia advice tells the player to fire everything at once, which D4.22 merges into one volley."** The auditor read two UI one-liners and missed `sfb_doctrine.py:48-67`, which encodes the exact D4.22/E10.44/E11.332 exception set, and `sfb_advanced.py:93-101`, which emits the split-volley advice. The proposed "correct" behaviour (split across different *facings*) is also doctrinally backwards — Mizia requires the same down shield.
- **"'hull' is every non-shield box, so `absorb_capacity` massively overstates soak."** Factually wrong (phaser and photon boxes are peeled off earlier in `sfb_ssd.py:95-100`), and the metric is scale-invariant where it matters: `commitment()`'s ratio cancels a shared inflation factor. `choose_posture` uses a *fraction*, not an absolute. Real but second-order, biting only atypical hulls (tugs, freighters).
- **"F&E campaign layer has no rulebook grounding — condition bands invented."** `sfb_fne.py` issues no orders; `condition` is read live off the piece attribute (`:100`), not invented; the `<90` cut is display bucketing; the 3-hex "front" is a labelled concentration heuristic, not a ZOC ruling.
- **"Mizia advice omits the precondition that the down shield must stay facing you."** The cited passage is from the *defending* section — doctrine for the victim, not a precondition on the attacker — and the advice already ends with "…separate volleys **on the same impulse** where possible", which is precisely the case where the target's turn mode is irrelevant. Withholding Mizia advice unless the target is turn-locked would suppress correct advice in the common case.
- **"Impulse of Decision is fed a hardcoded impulse=1 and never fires."** The production path is live: `sfb_bridge.py:932` → `sfb_command.py:1639` → `sfb_situations.py:798-800` tests `imp > IMPULSE_OF_DECISION` on the parsed value. The cited defaults are in `sfb_brain` functions that have **no callers anywhere in the tree** — a dead legacy path, not the advisor you run.

---

## 7. Second pass — resolutions from client data

The SFU Online Client ships its own machine-readable rules data (`sfb.jar` → `data/sfbol/`,
53 files, extracted to `client_data/`). Formats are fully decoded in **`CLIENT_DATA.md`** — that
document is the reference; this section records only what the data *settles* for this audit.

**Epistemic rule used throughout.** The client is an implementation, not the rulebook. It is
authoritative for what a live game will enforce; the printed rules are authoritative for what is
correct. Most of the charts at issue (phaser, disruptor, photon, hellbore, plasma, R0.6 size class)
exist in `rules_txt` only as image scans that extract to nothing, so where a verdict below rests on
the client alone it says so.

### 7.1 Open questions now ANSWERED

**Q6 — BPV scoring (SC5). ANSWERED, cleanly.** `master_ship.chart` column 4 is BPV, keyed
`(race section, ship type)`, with `A/B` splits where a unit has an economic/combat distinction
(Kzinti FRD `200/50`, CVD `149/116`, SR `120/100`; Lyran CVD `144/120`, NSR `151/128`). There is
nothing to reconcile against the engine: `grep -n "BPV\|bpv" sfb_*.py` across all 34 modules returns
**zero hits**. The engine has no BPV table, no scoring, no fleet-point balance and no Commander's
Options machinery at all — the chart is the uncontested source. *Residual:* which side of an `A/B`
split is economic and which is combat is not stated by either the chart or `rules_txt` (S2.1 is not
present as text). The empirical half of Q6 — whether your scenarios actually use S2.2 standard
victory — is untouched and still needs a game.

**Q7 — Fighter speeds beyond Kzinti (F6). ANSWERED as a firm negative on the client, and
redirected.** Fighter speeds are **not** in the client data, and this is not a search failure:
every `SPEED` record in `client_data` belongs to a seeking-weapon family in a `*.expendable` file
(drones 8/12/20/32, plasma 32/40, mines 0, chaff 0, EW pod 0, tachyon 18-36, HEAT 32/64, missile
rack 10/20). Fighters appear only as SSD box types (`boxtypes.names:60 Fighter=61`), DAC hit types
(`hittypes.names:31 Fighter Box=32`), sequence-of-play prose, and purchase-menu rows
(`nwo_hdw.list:12 1,Fighter,`). `master_ship.chart` is Annex #3 only — no `AAS`/`Z-Y`/`Stinger` rows
exist. The client models a fighter as a *box on a ship*, not as a unit with performance data.

The chart that does have them is **`rules_txt/SFB_Module_G3.txt` from line 9626** — Annex #4, the
Master Fighter Chart, header `Type Size Spd Phaser Drones Dmg Other Weapons BPV Year DFR Prod Ref`,
race-sectioned by R-rule exactly like Annex #3. Cleanly readable rows (Type label and data on the
same line):

- General: A-Admin 8, Admin-P 6, Admin-Y 4, A-GAS 8, A-GBS 8, A-HAS 8, A-HTS 8, A-MLS 8, A-MRS 10,
  A-MSS 8, GAS 6, GAS-Y 4, GBS 6, GBS-Y 4, HAS 6, HAS-Y 4, HFS 6, HFS-Y 4
- Kzinti: AAS 8, AASM 16, AAS-E 8, AAS-EM 16, **TAAS 15**, TAASM 30, TAAS-E 15, TAAS-EM 30
- Klingon: Z-1 6 / Z-1M 12, Z-1R 6 / Z-1RM 12, Z-1V 9 / Z-1VM 18, Z-1Y 12 / Z-1YM 24, Z-1C 12,
  Z-1E 6 / Z-1EM 12; Z-Y family 15, Z-Y\*M 30
- Federation: A-6A 8 / A-6AM 16, A-6B 10 / A-6BM 20, A-6D 10 / A-6DM 20, EA-6B 10 / EA-6BM 20,
  A-20FM 30; LAS 12 / LASM 24, LKS 12 / LKSM 24, LKF 15

The invariant across every family is that the `M` (refit) variant is exactly **double** the base.
*Residual:* the PDF text layer is column-shifted for roughly half the rows — Type labels are
orphaned from their data (e.g. the Kzinti HAAS/HBMR and ABMR/AS/BMR blocks at `SFB_Module_G3.txt`
:10714-10760). A trustworthy full `FIGHTER_SPEED` needs the printed Annex #4 page images (G3
pp.121-144), not the text layer. See 7.3 for what this does and does not license changing.

**Q10 — Shuttle boxes differentiated per ship (F4). PARTIALLY ANSWERED, and a working assumption
corrected.** `master_ship.chart` column 7 is **spare shuttles**, read `admin + fighters` — it is
*not* a count of SSD shuttle boxes (J1.42; the G3 "EXPLANATION OF TERMS → SHUTTLES" is explicit).
This is why a Lyran CA reads `1` while its SSD shows two shuttle boxes. Carriers put the fighter
group in the second term: Kzinti CV `3+3`, CVL `2+2`, CVA `2+6`, BB `3+3`, SCS `3+3`; Lyran CVA
`2+6`, CV `2+4`, BCV `2+4`, SCS `2+4`, CVD `2+6`; Klingon D7V `1+2`, CVA `2+4`. Non-carrier line
ships are mostly `1` (Lyran CA/CC/CCH/CF/NCA) or `2` (Kzinti CA/CC/BC, Klingon D7C). Klingon
F5/F5W/E4 read `-` (zero spares — these hulls do have shuttle boxes, so `-` cannot mean "no boxes").
The engine hardcodes no shuttle or fighter counts anywhere (`sfb_deckcrew.py:134` takes
`shuttle_boxes` as an argument; `sfb_command.py:1410 PF_TYPES` and `sfb_silhouettes.py:160
SMALL_TYPES` are classification sets, not counts), so there is no engine-vs-chart conflict to
resolve. **What is still not answered** is the actual J1.4 question — which of your ships have a
shared shuttle/fighter pool and which are differentiated — because that lives on the SSD, not in
Annex #3. F4's Batch 1 fix is unchanged.

**Q4 — DAC-based commitment scoring (DM3). The DATA half is answered; the empirical half is not.**
The client's `ship_dac.table` is fully decoded: leading `2` (⇒ 2d6), 11 rows for rolls 2-12, each
13 ordered `(hittypeId, flag)` pairs = DAC columns A-M, with column M = id 13 *Excess Damage* on
every row. `pf_dac.table` and `dragon_dac.table` are the 1d6 analogues (6×6 and 6×4). The flag is
**BOLD, meaning D4.31 "once per volley"** — the earlier reading as "underlined / damage is lost" was
wrong. Verified three ways: the binary table, the `misc.chart` DAC block (which wraps exactly the
flag-`1` cells in `<B><U>`), and printed D4.221/D4.222/D4.31. Client and rulebook agree completely;
there is no disagreement to report on the DAC itself. So DM3 is no longer blocked on data — it is
blocked only on the original empirical question (does expected-boxes-killed beat raw damage in the
engage/avoid verdict?), which still needs a real comparison.

### 7.2 Still open, and what would settle each

- **Q1 (GR vs hellbores), Q2 (weasel budget), Q3 (bolt-vs-launch), Q5 (boarding parties)** —
  untouched. These are playstyle/threshold questions the client cannot answer.
- **Q4 empirical half** — a side-by-side of `commitment()` with and without a DAC-weighted
  expected-loss vector, in a real fight.
- **Q6 residual** — the semantics of the `A/B` BPV split. Needs printed S2.1.
- **Q7 residual** — printed Annex #4 page images (G3 pp.121-144) to realign the column-shifted rows.
  Also unresolved: whether the `Spd` column is a fixed speed or a maximum under J1.x, and whether
  `FIGHTER_SPEED_ACE_MAX=31` (`sfb_carrier.py:111`, cited to J6.23) is right.
- **Q8 (shield cost SC2)** — the client ships no shield-cost chart. Still needs a clean scan.
- **Q9 (sensor track length)** — the client ships no per-ship sensor-box table either. Unchanged:
  read it from live damage state.
- **Q10 residual** — needs the SSDs.
- **New, from this pass:**
  - Drone **warhead damage in points** and **rack ammo capacity** are absent from the client data
    entirely (the `.expendable` magnitude column is armour/damage-to-kill for drones, warhead
    strength only for plasma). `DRONE_WARHEAD=12` / heavy 24 could be neither confirmed nor refuted;
    needs printed FD1.x.
  - What functionally distinguishes drone-rack subtypes **A through H** — `boxtype.defs` gives only
    the letter.
  - `boxtype.defs` field 1 (values 1/2/3) is unidentified; see `CLIENT_DATA.md` §5.
  - Whether the client **enforces** the sequence of play or merely displays it — the `.act` files are
    pure display data (id/parent/prose, no flags, no conditions). Enforcement is in the jar bytecode;
    settling it needs decompilation.
  - Annex **#7E** ("every third hit on the best available type", D4.3221-3223) and the D4.321 phaser
    directional restriction are not in any client file.

### 7.3 New confirmed discrepancies, ranked

Severity below is the *verified* severity after checking liveness of the code path, not the
first-pass estimate. Several first-pass claims were refuted outright and are listed in 7.5.

| # | Finding | Severity | Batch |
|---|---|---|---|
| N1 | `sfb_rules._PH2_TBL` matches the client Phaser-2 chart in **zero** rows, and zeroes the weapon past range 15 where the real Ph-2 reaches 50 | **CRITICAL** | 2 (D1) |
| N2 | `PH3_EXPECTED` disagrees at 6 of 7 brackets — 3× too high at R6-8, 2× at R3 — on a live threat-evaluation path | **CRITICAL** | 2 (D1) |
| N3 | Plasma-R/F/G modelled as linear decay formulas; a Plasma-R launches at **50**, not 30 | **CRITICAL** | 2 (D1) + 4 (S3) |
| N4 | Engine has no working DAC; `CADET_DAC` is the wrong chart *and* is dead code (zero references) | **CRITICAL** (was DM3 MINOR) | promote to 2 |
| N5 | `_DISR_TBL` models the disruptor as a per-die damage curve; it is roll-to-hit with fixed damage, and its values exceed the weapon's maximum | **MAJOR** | 2 (D1) |
| N6 | Hellbore table: wrong hit numbers at R3-4, damage understated ~50% at every bracket, truncated at range 15 (real: 40), no range-0 standard-fire prohibition | **MAJOR** | 2 |
| N7 | `PH2_EXPECTED` R0/R1/R2 are Ph-1 values (18% overstatement point-blank); tail mis-bracketed | **MAJOR** | 2 (D1) |
| N8 | `PH4_EXPECTED` collapses the client's 13 bands into 6 (+48% at R11-13) and zeroes past range 50 (real: 100) | **MAJOR** | 2 (D1) |
| N9 | Fusion: standard truncated at range 10 (real: 25) and overloaded at range 2 (real: 8) — cell values were transcribed *correctly*, trailing columns simply dropped | **MAJOR** | 2 |
| N10 | `absorb_capacity()` is a flat `facing shield + hull` pool with no system-loss model, systematically undervaluing early internals | **MAJOR** | 6 → now 2, once N4 lands |
| N11 | `drone_profile(ship)` reads the **starship's** speed as the drone's speed, driving the `rng <= speed*2` launch gate | **MAJOR** (confirms **S1**) | 1 — unchanged |
| N12 | Whole client decision stages the engine never advises on: shields, tractors, transporters, mines, fire control, cloak, chaff, probes | **MAJOR** | new Batch 7 |
| N13 | `SIZE3` hull-letter heuristic returns 3 for DN/CVA/SCS/BATS (client: 2) and can never return 1, 2 or 5 | **MINOR** (fallback only — live saves carry `size_class`) | 5 |
| N14 | `FIGHTER_SPEED[("KZINTI","TAAS")] = 12`; Annex #4 says **15** (TAASM 30) | **MINOR** | 6 (F6) |
| N15 | `fighter_speed()` returns `known=True` on a `min()` over a 4-row partial table — a guess reported as charted fact | **MINOR** | 6 (F6) — already listed |
| N16 | `drone_profile`'s `re.search(r'Speed Module:\s*([A-Z]+)')` can never fire; client speed codes are bare single chars (` `/S/M/F). `prof['module']` is always `None` | **MINOR** (informational field only) | 1, alongside S1 |
| N17 | `impulse_actions` emits direct fire **before** seeker and shuttle launch, inverting 6B → 6D; plasma is handled inside `fire_actions` as if it were direct fire when the client launches it at 6B06.03 | **MINOR** (advisory ordering, not illegal) | new Batch 7 |

**Batch 7 (new) — "advise the whole impulse, in the client's order" (~0.5-1 day).** N12 + N17.
Drive the checklist directly off `tourn_dec_abbrev.act` so it cannot drift from the client, tag each
headline with its stage id (`[6B06.05] LAUNCH DRONES`, `[6D2.04] FIRE DISRUPTORS`), and add
generators for at least the tournament subset: chaff drop when seekers are inbound (hook the
existing `cmd.incoming_seekers` call — this is the sharpest gap, since the engine already computes
inbound seekers and only ever answers with phasers), tractor activation, shield drop/restore,
transporter hit-and-run. Do after Batch 2; nothing here is wrong, the engine is simply silent.

### 7.4 What the client data confirms the engine has RIGHT

Recorded so it is not re-litigated.

- **`sfb_command`'s heavy-weapon tables are exact.** `DISR_STD` (all 9 brackets, damage *and* hit
  number), `DISR_OVL`, `PHOT_STD` and `PHOT_PROX` reproduce `weapons.chart` cell for cell.
- **`PH1_EXPECTED` is exact** — all 11 non-zero brackets match the client Ph-1 column means to three
  decimals (6.500, 5.333, 4.833, 4.333, 3.833, 3.500, 2.167, 1.000, 0.500, 0.333, 0.167). Note this
  is the *only* phaser expectation table that survives; whatever cross-check produced it evidently
  covered the Ph-1 alone.
- **`_FUSION_TBL` and `_FUSION_OL_TBL` cell values are exact** for every column the engine
  implements (13,8,6,4 / 11,8,5,3 / 10,7,4,2 / 9,6,3,1 / 8,5,3,1 / 8,4,2,0). N9 is a truncation, not
  a transcription error.
- **Turn mode handling is correct.** `sfb_command.py:26 TURN_CAT` maps AA..F to 0..6 and
  `turn_category()` reads the ship's own `turn_mode` from live state rather than guessing from hull
  type; `_TURN_MODE_BRACKETS` matches `turnmode.chart` exactly. The docstring's "a Federation CA is
  category D" matches Annex #3 exactly — and note Kzinti CA and Lyran CA are **C** and the Klingon D7
  is **B**, so any code assuming D for all cruisers would be wrong. None does.
- **Nothing is hardcoded that should be live.** No move-cost, crew, shuttle-count or drone-count
  constant tables exist; all come from the save. `FLAG_RANK` and `CAPITAL_TYPES` are doctrine-priority
  sets, not stat tables, and carry no chart obligation.
- **Drone speed handling agrees with the client.** `DRONE_SPEED_UPGRADE = {"-M": 20, "-F": 32}`
  matches the client's `M`=20 / `F`=32 exactly, and the live path prefers the seeker's own speed
  field from game state. (The only residue is the last-resort literal `12` where the client's
  no-module default is 8 — cosmetic, and unreachable when data is present.)
- **Drone kill thresholds agree with the client.** `DRONE_KILL_DAMAGE` = 4 standard / 6 for heavy
  Type-IV/V matches the `.expendable` armour column exactly, and drone endurance *is* modelled
  (`DRONE_TYPES` carries `(speed, endurance, warhead, kill)` plus
  `DRONE_EXTENDED_RANGE_DOUBLES_ENDURANCE` per FD2.222).
- **The heavy-warhead regex does not misfire.** `r'\btype[- ]?(IV|V)\b'` does **not** match
  `Type-VI`/`VII`/`VIII` in Python (the `\b` after `V` fails and there is nothing to backtrack into),
  so dogfight drones are not reported as heavy warheads. Only `Type-H` lacks an explicit case.
- **Reserve power is modelled correctly at source.** `sfb_command.py:159-206` deliberately refuses to
  put a reserve line on the EA form, citing H7.0/H7.113 — reserve power comes from *batteries*,
  charged in EA and spent later. That is the rulebook-correct model.
- **Overloaded photons at range 0-1 are legal** and the engine's `PHOT_OVL` `(1,16,6)` is right.
  Here the **client** is the one that disagrees with the rulebook: `weapons.chart` lossily flattens
  standard and overload photon hit numbers into a single `STD/OLVD` row showing `NA` at range 0-1,
  whereas printed E4.1 gives overload its own row (`1-6` at 0-1) and E4.14 exempts overloads
  explicitly (E4.43). Treat the photon block of `weapons.chart` as unreliable.

### 7.5 First-pass claims REFUTED by this pass

- **"HAAS speed 11 contradicts Annex #4 (15)."** The `15` is a misread of a column-shifted
  extraction: the values double for every `M` variant (HAAS 15 / HAASM 30, HBMR 15 / HBMRM 30), and
  fighter speed does not double on a refit — that is a size/cost column. The engine's 11 sits in the
  adjacent (plausible-speed) column and is the better-supported value. TAAS (N14) is different: that
  row is intact on one line, `TAAS 1 15 …`, and is validated by the control row `AAS 1 8 …` which
  matches the engine's own AAS=8.
- **"`damage_at()` returns photon damage without the to-hit probability."** A row/column misread —
  the client's `STD/OLVD` row is *hit numbers*, the `DMG-STD` row is a flat 8 at every range 2-30,
  which is exactly what the engine returns. (A real but separate disruptor defect survives; see N5.)
- **"Engine drone speed defaults conflict across three constants."** `sfb_rules.DRONE_SPEED=32` is
  dead (no readers) and `sfb_actions`' `8` is an overwritten local initializer that happens to equal
  the client's default. One live path, and it agrees with the client.
- **"Engine models no drone endurance."** It does; see 7.4. The claim also misread the
  `.expendable` magnitude column as endurance when it is armour.
- **"Heavy-warhead regex mis-classes Type-VI/VII/VIII."** False as a matter of Python regex
  semantics; see 7.4.
- **"ESG treated as an immediate same-impulse toggle."** The engine has no ESG state machine at all —
  `esg_actions` returns advice prose, and its `rng <= 6` trigger leads the 1-3 hex sphere radius by
  three hexes, i.e. it allows for announcement lag rather than ignoring it.
- **"No concept of 6E postcombat or reserve power as a mid-impulse decision."** Erratic Manoeuvres
  (`sfb_maneuver.py:204-219`, live), UIM (`DISR_OVL_UIM`) and battery-sourced reserve power
  (`sfb_command.py:159-206`) are all present. The residue is a modest advice-surface gap (no 6D2.01
  window, no 6E label), folded into Batch 7.
- **"`sfb_ssd.py` cannot supply the box census a real DAC needs."** Accurate as stated, but it is a
  coverage gap, not a client-vs-engine disagreement — it is a *dependency* of N4/N10, not a separate
  finding.

### 7.6 Effect on the Batch 1 plan

**Batch 1 is unchanged in scope and its priority is confirmed.** The one Batch 1 item the client data
speaks to directly — **S1** — is confirmed as a real defect on a live path (N11), with the extra
detail that `drone_profile` was written for an in-flight seeker record (its sibling caller
`incoming_seekers` passes one correctly) and `sfb_actions.py:150` passes the ship instead. Fold N16
into the same fix. Nothing else in Batch 1 is contradicted or superseded.

The material change is to **Batch 2**, which grows: it now also carries N4 (a real DAC, promoted out
of Batch 6), N3/N6/N9 (plasma, hellbore, fusion), and N10 once N4 lands. Batch 2's own framing —
"the engine has two disagreeing sets of weapon charts" — is now settled by data: **`sfb_command`'s
tables are right and `sfb_rules`' are wrong**, so D1's resolution is not "pick one file" but "delete
`_PH1_TBL`/`_PH2_TBL`/`_DISR_TBL`, drive `sfb_rules` from `sfb_command`, then fix
`PH2`/`PH3`/`PH4_EXPECTED` in `sfb_command`". Note also that several of the `sfb_rules` tables are on
the `sfb_ew.apply_shift_roll` path, which itself has no callers — so parts of the fix are correctness
housekeeping rather than behaviour change, while `PH2`/`PH3`/`PH4_EXPECTED` are live in
`expected_damage()` and do change advice today.

## 8. DM3 (real DAC) — NOT implemented, and why

Batch 2 completed except DM3. The dead `CADET_DAC` has been removed (wrong chart,
zero callers). The real chart was **not** wired in, deliberately.

**Established** from `client_data/ship_dac.table` and rule D4.31:
- 11 rows = a 2d6 allocation roll of 2..12
- 13 columns, walked left to right as damage is scored; `Excess Damage` always last
- per-cell flag `1` = BOLD = D4.31 "a given BOLD result can only be scored ONE
  time in each volley" (this is what makes three hits on the same roll land on
  three different systems)

**Not established: the box-kind mapping.** D4.31's worked example states that a
roll of 12 scores on *auxiliary control, emergency bridge, and scanners*. The
table's roll-12 row begins `25, 26, 3`, which both `boxtypes.names` and
`abbrev_boxtypes.names` decode as **Cargo, Shield, Scanner** — only the third
matches. Row order was also tried reversed and column-major; neither reconciles.

**Why it was left alone.** Finding E1 in this same report was retracted after
being implemented on an unverified reading of an ambiguous source, and it
over-booked every cruiser by 2 energy a turn until a live measurement caught it.
Wiring a DAC whose mapping contradicts the rulebook's own example would repeat
that mistake in a place that feeds target selection and engage/avoid decisions.

**How to settle it:** apply a known volley to a throwaway ship in the client and
read which boxes it checks off. The combat log records both sides of it
("Allocation of damage for: X" / "Damage: n/n/n"), so a handful of volleys yields
the mapping from observation. Until then `commitment()` continues to use raw
expected damage, which is coarser but not wrong.
