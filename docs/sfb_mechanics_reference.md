# Star Fleet Battles — Core Mechanics Reference

Extracted from: Captain's Master Rulebook (2012) and Cadet Training Handbook (1996).
Rule references in parentheses, e.g. (B2.2), are from the Master Rulebook unless noted [CTH].
Rule letter prefixes: A–B = general; C = movement; D = combat; E = direct-fire; F = seeking weapons
(FD = drones, FP = plasma); G = systems; H = power; J = shuttlecraft.

---

## 1. Turn Structure — Sequence of Play (B2.2)

Each game turn has these phases, in order:

1. **Energy Allocation Phase** — all players secretly fill out EAF for all ships (B3.0)
2. **Speed Determination Phase** — speeds announced; controller prepares movement chart
3. **Self-Destruction Phase** — ships that plotted self-destruct do so now (D5.0)
4. **Sensor Lock-On Phase** — all ships roll for sensor lock-on (D6.1)
5. **Initial Activity Phase** — tractor rotations, undocking, assigning guards, etc. (Annex #2)
6. **Impulse Procedure** — repeated for each of **32 impulses** (see §3):
   - A. Movement Segment
   - B. Activity Segment (transporters, launch drones/shuttles, lay mines, etc.)
   - C. Dogfight Resolution Interface (fighters only)
   - D. Direct-Fire Weapons Segment (secret simultaneous declare → resolve)
   - E. Post-Combat Segment
7. **Final Activity Phase** (Annex #2)
8. **Record Keeping Phase** (Annex #2)

**Cadet variants:** 8 impulses/turn (Scenarios 1–3), 16 impulses/turn (Scenarios 4–6), 32 impulses/turn (Standard, Scenarios 7–12).

---

## 2. Energy Allocation (B3.0–B3.1)

Filled out **secretly** before the turn. Power does **not** carry over between turns (exceptions: phaser capacitors, batteries, multi-turn weapons).

### Power Sources
| Line | Source | Notes |
|------|--------|-------|
| 1 | Warp Engines | Count unchecked warp boxes on SSD. Fed CA = 30 |
| 2 | Impulse Engines | Fed CA = 4 |
| 3 | Reactor / APR | Klingon D6 = 2; APR power **cannot** be used for movement |
| 4 | **Total Power** | Sum 1+2+3. Fed CA undamaged = 34 |
| 5 | Batteries Available | Fully charged at scenario start |
| 6 | Batteries Discharged | Lines 5+6 always = undestroyed battery boxes |

### Expenditures
| Line | Use | Notes |
|------|-----|-------|
| 7 | Life Support | **Required** every turn; cost by ship size class |
| 8 | Fire Control | **Required** to fire weapons; 1 pt = full FC; 0 = cannot fire |
| 9 | Phasers | Goes to phaser capacitor (H6.0); can carry over turn-to-turn |
| 10 | Torpedoes / Heavy Weapons | Per tube; weapon-specific arming schedules |
| 11 | Shields | Raise / maintain; cost by ship size class (D3.32); standard = 2 pts |
| 12 | General Shield Reinforcement | Any amount; ÷2 (round down) = points blocking damage from any direction |
| 13 | Specific Shield Reinforcement | 1 pt = 1 extra box on one named shield for this turn |
| 14 | Movement | Warp: ÷ movement cost = MPs; impulse: max 1 pt/turn = 1 MP |
| 15 | Damage Control | 2 pts = repair 1 shield box at end of turn |
| 16 | Recharge Batteries | Restores previously discharged batteries |
| 17 | Tractor Beam | 1 pt per beam (G7.0) |
| 18 | Transporters | (G8.0) |
| 19+ | Misc (ECM, cloak, shuttles, ESG) | Various |
| 20 | **Total Power Used** | Must not exceed line 4 + batteries discharged |
| 21 | Battery Power Used | Batteries discharged this turn |

### Movement Cost (C2.12)
- Most ships: 1 warp pt = 1 movement point (MP)
- Heavy ships (Fed DN): 1.5 warp pts = 1 MP
- Light ships (Klingon F5): 0.5 warp pts = 1 MP
- Max warp contribution: **30 MPs/turn** (C2.112)
- Impulse contribution: max **1 MP/turn** always = 1 hex regardless of ship size (C2.111)
- Max speed: **31** (30 warp + 1 impulse)

### Acceleration (C2.2)
Maximum speed **increase** per turn = **max(previous speed, 10)**.
- Examples: speed 3 → can increase to at most 13; speed 15 → at most 30 (then capped at 31)
- No limit on deceleration (can drop to 0 in one turn)
- Turn after using Tactical Maneuvers: treated as previous speed 0 for acceleration (C5.4)

### Reserve Power (H7.0) [Advanced]
Batteries can be discharged **mid-turn** for: reinforcing shields (even after enemy fire declared), firing weapons, operating transporters/tractors. Cannot be used retroactively within same impulse.

---

## 3. Impulse Movement System (C1.4)

Each unit moves **one hex per impulse** on impulses specified by the **Impulse Movement Chart** (a lookup table). The chart distributes movement proportionally across 32 impulses so faster units move more often.

- A ship at speed N moves in exactly N of the 32 impulses
- Which impulses: the chart staggers them evenly (e.g. speed 8 → every 4th impulse)
- Maximum speed: 31 (C1.42); drones/plasma torpedoes can move at 32
- Speed 1: moves on impulse 32 only

### Order of Movement within each impulse segment
1. Monsters
2. Ships (slower before faster; ties: better turn mode moves last)
3. Nimble ships (C11.0)
4. Fighters and shuttles
5. Seeking weapons (drones, plasma torpedoes)
6. Base rotations
7. Tactical Maneuvers

### Turn Modes (C3.0)
A **turn mode** is the number of hexes a ship must move in a straight line before making a 60° turn.

Each ship has a **Turn Mode Category** (AA, A, B, C, D, E, F — AA most manoeuvrable). Combined with current speed, this yields the actual turn mode number.

#### Turn Mode Chart (C3.31) — hexes required straight before each 60° turn
| Speed | AA | A | B | C | D | E | F |
|-------|----|---|---|---|---|---|---|
| 1     | 1  | 1 | 1 | 1 | 1 | 1 | 1 |
| 2–5   | 2  | 2 | 2 | 2 | 2 | 2 | 2 |
| 6–10  | 2  | 2 | 2 | 3 | 3 | 4 | 5 |
| 11–15 | 3  | 3 | 4 | 5 | 6 | 7 | 8 |
| 16–20 | 4  | 5 | 6 | 7 | 8 | 9 |   |
| 21–25 | 5  | 6 | 8 |10 |   |   |   |
| 26–30 | 6  | 7 |   |   |   |   |   |
| 31    | 7  |   |   |   |   |   |   |

*(Approximate brackets — verify exact values from Master Annexes. Federation CA = category D.)*

#### Turning Rules
- Turn happens at the **start** of an impulse the ship is scheduled to move (before entering next hex)
- Turn then move (not move then turn)
- Turn mode count carries over between turns (C3.41) — same direction only
- Speed change resets carryover credit if new speed requires longer turn mode (C3.44)
- HET, full stop, or reversing direction resets accumulation (C3.45)
- To reverse direction: three consecutive 60° turns, each requiring full turn mode

### Sideslips (C4.0) [Advanced]
After satisfying sideslip turn mode of 1, ship enters a forward-diagonal hex but retains original facing. Normal turn and sideslip turn modes tracked independently; cannot do both in same impulse.

### Tactical Maneuvers (C5.0)
- **Sublight TAC (C5.1):** Ship skips movement but makes one free 60° turn; costs 1 impulse engine point
- **Warp TAC (C5.2):** Up to 4 turns/turn; earned on scheduled movement impulses (except last); same energy as 1 hex movement; cannot use impulse energy
- After TAC turn: previous speed treated as 0 for next turn's acceleration (C5.4); turn mode count resets

---

## 4. Shields (D3.0)

### Six Shields
Each ship has 6 shields, one per hex-facing (A–F → #1–#6):
- **#1** = forward (ship's current facing direction)
- **#2** = forward-right, **#3** = rear-right, **#4** = rear, **#5** = rear-left, **#6** = forward-left

Shields are fixed — cannot rotate. If #1 is down, nothing covers that arc.

### Operation (D3.2)
- Each damage point checks off 1 shield box
- When all boxes gone: shield **down**; further damage = internal hits
- Damage on one shield never affects another shield
- Standard game cost to raise shields: **2 energy points** (D3.32)

### Determining Which Shield is Hit (D3.4)
- **Seeking weapons:** the shield facing the hex the weapon approached from
- **Direct-fire weapons:** draw line from target's hex centre to firer's hex centre; the shield boundary that line crosses is hit
- If line runs exactly along a boundary: roll d6 to split between the two adjacent shields

### Reinforcement (D3.341–D3.342)
- **General (line 12):** energy ÷ 2 (round down) = points absorbed from **any** direction; used before specific reinforcement; not used points lost each turn
- **Specific (line 13):** 1 pt = 1 extra temporary box on one named shield; cannot reinforce a shield that is already down; unused points lost
- General reinforcement also blocks transporters even through dropped shields (D3.343)

### Damage Control (D9.0)
- DC rating = highest undestroyed box on DC track
- Allocate up to DC rating in energy per turn (line 15)
- Every 2 energy = 1 shield box repaired at **end** of turn
- Repaired boxes do not block damage until the **following** turn

---

## 5. Firing Arcs (D2.0)

Six arcs, each 60°, relative to the ship:

| Code | Arc |
|------|-----|
| LF | Left Forward |
| RF | Right Forward |
| L  | Left |
| R  | Right |
| LR | Left Rear |
| RR | Right Rear |
| FA | LF + RF (full forward) |
| RA | LR + RR (full rear) |
| LS | LF + L + LR (left side) |
| RS | RF + R + RR (right side) |
| FX | L + LF + RF + R |
| RX | L + LR + RR + R |

Adjacent arcs share one boundary row of hexes. Range measured by counting hexes (include target's hex, not firer's).

---

## 6. Direct-Fire Weapons — General (E1.0)

- Fired in **Direct-Fire Weapons Segment**; effects resolved immediately
- All players secretly decide simultaneously; announce together; fire in any order
- Damage is **simultaneous** — a committed weapon fires even if destroyed in the same segment
- Each weapon fires **at most once per turn**
- Cannot fire twice within ¼ turn across consecutive turns (if fired on last impulse of turn N, cannot fire before impulse 9 of turn N+1 in 32-impulse game)
- Cannot fire outside weapon's arc
- No means of deflecting a direct-fire weapon; only shields absorb it

---

## 7. Phasers (E2.0)

### Types
| Type | Used By | Range | Energy Cost | Notes |
|------|---------|-------|-------------|-------|
| ph-1 | Federation, most races | ~15 effective, 30 max | 1 pt | Most powerful |
| ph-2 | Klingon, others | Shorter | 1 pt | Less accurate |
| ph-3 | Point defence, shuttles | 1–2 hexes | 0.5 pts | Anti-drone; can fire 360° |
| ph-G | Graduate scenarios | — | 1 pt | Uses ph-3 table |

### Firing
1. Measure range (count hexes to target)
2. Roll 1d6
3. Cross-index die roll × range on **phaser table on ship's SSD** → damage points

### Phaser Capacitors (H6.0)
- Capacity = total energy to fire all phasers once (Fed CA: 3 phasers × 1 = cap. 3)
- Energy allocated on line 9 accumulates in capacitor; can hold across turns
- Drawn any impulse to fire; total cannot exceed capacity
- If phaser destroyed, its portion of capacitor is also destroyed (uncharged elements first)

---

## 8. Photon Torpedoes (E4.0) — Federation

### Arming
- Takes **2 turns**: allocate 2 warp energy points to the specific tube on **each** of 2 consecutive turns (4 total; must come from warp engines)
- **Holding** an armed torpedo: 1 energy/turn from any source; if not paid, torpedo ejected
- Two tubes (A and B) are independent

### Firing
- Minimum range: **2 hexes** (cannot fire at 0 or 1)
- Maximum range: **30 hexes**
- Roll 1d6; if ≤ hit number for range bracket (from SSD table) → **hit**
- Hit damage: **8 points** (standard); **16 points** (overloaded)
- Overloaded: allocate extra energy; maximum effective range ~8 hexes

### Approximate Hit Table (verify on ship SSD)
| Range | Hit on |
|-------|--------|
| 2–8   | 1–4 |
| 9–15  | 1–3 |
| 16–25 | 1–2 |
| 26–30 | 1   |

---

## 9. Disruptors (E3.0) — Klingon and others

- 2 warp energy points per bolt per turn; energy **not** carried over
- Unfired bolts at end of turn are lost
- Fire any impulse in Direct-Fire Weapons Segment; no counter placed
- Each disruptor fires at most once per turn
- Roll 1d6 vs. range on disruptor table on SSD → damage
- When target is cloaked: use doubled effective range for hit probability; use true range for damage
- Firing at drones: add 2 to die roll (harder to hit)
- **Overloaded:** 4 energy pts; more damage at short range (max range ~8)

---

## 10. Plasma Torpedoes (FP1.0) — Romulan, Gorn, ISC

Seeking weapons represented by counters on the map. Extremely powerful; weaken with range.

### Types
| Type | Arming Energy (turn 1 / 2 / 3) | Notes |
|------|--------------------------------|-------|
| Type-S | 2 + 2 + 4 (over 3 turns) | Cadet game |
| Type-R | 2 + 2 + 5 | War Eagle; cannot be held |
| Type-F | 1 + 1 + 3 | Graduate training; smaller |

### Arming (FP1.2)
- Exactly the specified energy each turn for 3 consecutive turns; can come from any source
- If not launched on turn 3: pay 2 energy/turn to hold, or torpedo is ejected
- Ships start scenarios without torpedoes armed

### Launching (FP1.3)
- During Activity Segment (Launch Seeking Weapons Step)
- Launched in the 3rd (or later holding) turn of arming
- Can be launched in same hex as target (with feedback risk)

### Movement (FP1.4)
- Speed = maximum speed for scenario (8/16/32)
- Turn mode: 1 (can turn every hex)
- Endurance: Type-S = 25 hexes total travel
- Homes on declared target; self-guiding (unlike drones — continues even if launching ship is destroyed)

### Warhead / Damage (FP1.5)
- Strength determined at impact based on: (a) distance travelled (reduces with range — see SSD chart) and (b) phaser damage received
- Only phasers can reduce warhead strength: every 2 phaser damage pts = 1 warhead point reduction (permanent to that torpedo)
- Applied in Resolve Seeking Weapons Step against the shield the torpedo approached from

### Firing Arcs (FP3.0)
- **Fixed launchers** (Romulan Cadet cruiser): fires directly ahead only; target must be in FA arc
- **Swivel mounts** (KR and others): left torpedo fires LS arc; right torpedo fires RS arc

### Feedback (FP1.8)
If launched in same hex as target and hits before target moves: firing ship receives feedback damage on shield facing target = 25% of warhead strength (round 0.5 up).

---

## 11. Drones (FD1.0) — Klingon, Kzinti, others

Small homing missiles. Seeking weapons.

### Launchers (FD1.1)
- Drone rack: holds 4 drones; 1 drone launched per rack per turn
- Cannot launch within ¼ turn of previous launch from same rack
- Destroying the launcher destroys remaining drones in it; ammo track cannot absorb internal damage

### Launching (FD1.2)
- Activity Segment (Launch Seeking Weapons Step)
- No energy cost; place counter on top of ship facing any direction with target in drone's FA arc
- Must announce target; drone must move 1 hex forward before first turn (turn mode = 1)

### Movement (F2.0)
- Speed = scenario maximum (8/16/32); turn mode = 1
- Each scheduled impulse: move 1 hex toward target (into closer hex if possible)
- If same impulse as target: homes on hex target is entering

### Damage and Destruction
- Impact: drone explodes on entering target's hex; damage applied in Resolve Seeking Weapons Step
- Warhead: **6 pts** (Cadet drones), **12 pts** (Standard game drones)
- To destroy a drone: **4 damage points** total (in one or more volleys)
- Phasers, plasma torpedoes, other drones: no penalty to hit
- Photon torpedoes, disruptors: add 2 to die roll (harder to hit)
- Endurance: 3 turns; removed if not hit target by then

### Control (D6.132)
- Can control up to (sensor rating) drones simultaneously — usually 6
- Lose sensor lock-on → drones removed from map (FD5.0)

---

## 12. Damage Allocation (D4.0)

### Step 1: Which Shield?
Determine facing of attacker relative to target → hit that numbered shield (§4 above).

### Step 2: Apply to Shield
Check off boxes. If all gone: shield down; excess = internal hits.

### Step 3: Internal Damage — Cadet DAC (D4.21)
Roll 1d6 per internal damage point; look up in table:

| Die | A | B | C | D | E |
|-----|---|---|---|---|---|
| 1 | Hull | Engine | Other | Weapon | Excess |
| 2 | Hull | Other | Weapon | Engine | Excess |
| 3 | Engine | Hull | Other | Weapon | Excess |
| 4 | Other | Engine | Weapon | Hull | Excess |
| 5 | Weapon | Hull | Other | Engine | Excess |
| 6 | Other | Weapon | Engine | Hull | Excess |

Go to column A; if no boxes of that type remain, shift to B, C, D, E.

**Categories:**
- **Hull**: hull boxes
- **Engine**: impulse or warp engine boxes
- **Weapon**: phasers, photon/plasma torpedoes, disruptors, drone racks
- **Other**: transporters, labs, APR, batteries, shuttles
- **Excess Damage**: EX DAM boxes (outside hull outline)

Bridge and Security boxes **cannot** be destroyed in the Cadet game.

### Step 4: Standard DAC (D4.3) [2d6 — Standard Game]
More granular; additional rules:
- Bold-face results: only once per volley (D4.31)
- Phaser hits scored against phasers facing the direction of fire (D4.321)
- TORP result applies to disruptors, photon, and plasma torpedo boxes (D4.323)
- Forward hull (F) hit from forward volleys; aft hull (A) from aft
- Sensor/scanner/DC tracks: destroyed best to worst; last box never marked destroyed (D4.33)

### Ship Destruction (D4.4)
- All Excess Damage boxes filled + one more hit = ship destroyed; all crew die
- Drones controlled by destroyed ship are removed
- Self-guiding weapons (plasma torpedoes) continue after ship destroyed

---

## 13. Sensors (D6.0)

### Lock-On (D6.1)
- Roll 1d6 in Sensor Lock-On Phase; if ≤ sensor rating → lock-on to all targets
- Undamaged ships: sensor rating 6 → lock-on automatic
- Either locked on to all targets or none (some exceptions: cloaks, planets)

### Effects of No Lock-On (D6.12)
- Cannot launch seeking weapons (D6.121)
- Drones on map are removed (D6.122)
- Firing range to all targets **doubled** (D6.123)

### Scanner Adjustment (D6.2)
- Scanner factor = lowest undestroyed scanner track number (starts 0)
- Effective range = true range (×2 if no lock-on) + scanner factor

### ECM / ECCM (D6.3) [Advanced]
- ECM allocated via misc EAF lines; degrades enemy weapon effectiveness
- ECCM counters ECM; net ECM = firer's ECM − target's ECCM

---

## 14. Cloaking Device (G13.0) — Romulan

### Basic Operation
- Announce cloaked/uncloaked at start of each turn (after Energy Allocation Phase)
- Costs: Romulan War Eagle = 6 energy/turn; KR = 8 energy/turn (Cadet) / 20 (Standard)
- Cloaked ship **cannot fire any weapons**
- If uncloaking this turn after being cloaked last turn: cannot fire on impulse 1

### Effect on Combat Against Cloaked Ship
- Direct-fire: effective range = (true range × 2) + 5
- Seeking weapons entering cloaked ship's hex: roll d6; 1–4 = miss and removed; 5–6 = hit for half normal damage
- Disruptors vs. cloaked: doubled range for hit probability; true range for damage

### Standard Mid-Turn Cloaking (G13.33)
- Cloaking/uncloaking takes 4 impulses (fade-in / fade-out)
- During fade: range penalties reduce each impulse (4, 3, 2, 1 added to range)
- Cannot fire weapons during cloaking, cloaked, or uncloaking phases

---

## 15. Special Systems

### Tractor Beams (G7.0)
- Range: same hex or adjacent only; 1 energy/beam
- No arc restriction (360°); once released, cannot reuse within 8 impulses
- Can hold drones (friendly drone loses tracking; enemy drone continues)
- Cannot tow other starships (only freighters, drones, shuttles in standard rules)
- Cannot hold plasma torpedoes

### Transporters (G8.0)
- Range: 5 hexes; 1 energy = up to 5 transporters; 2 energy = 6–10
- Each transporter usable once per turn
- Cannot work through raised shields; target shield must be dropped for ≥ ¼ turn
- General reinforcement blocks transporters even through dropped shields

### Expanding Sphere Generator — ESG (G23.0) [Lyran ships]
- Store energy over multiple turns (up to 5 pts for standard ships)
- Announce release 4 impulses in advance; release at any point in that window
- Release at radius 0–3 hexes; strength = (5 − radius) × energy stored
- Field scores automatic damage on anything entering its radius
- Each damage point scored on a target reduces field strength by 1
- Duration: 32 impulses; cannot reactivate within 32 impulses of dropping

### Shuttlecraft (J1.0)
- Max speed 6; max acceleration 3 hexes/turn; any deceleration
- Turn mode: 1 at all speeds; must move 1 hex before first turn
- 1 ph-3 firing 360°; destroyed at 6 damage pts; crippled (3+ pts) = max speed 3

### Boarding Party Combat (D7.0)
- Arrive via transporter during Activity Segment
- Resolved at end of turn; both sides attack simultaneously
- Groups of 10 BPs + remainder; roll d6 per group → casualty points from table
- Capture all control stations (Bridge, Emergency Bridge, Aux Control) = ship captured

**Boarding Party Combat Table:**

| Die \ BPs | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|-----------|---|---|---|---|---|---|---|---|---|----|
| 1 | 0 | 0 | 0 | 0 | 1 | 1 | 1 | 1 | 1 | 1 |
| 2 | 0 | 0 | 1 | 1 | 1 | 1 | 1 | 2 | 2 | 2 |
| 3 | 0 | 1 | 1 | 1 | 2 | 2 | 2 | 2 | 3 | 3 |
| 4 | 0 | 1 | 1 | 2 | 2 | 2 | 3 | 3 | 4 | 4 |
| 5 | 1 | 1 | 2 | 2 | 3 | 3 | 4 | 4 | 5 | 5 |
| 6 | 1 | 1 | 2 | 2 | 3 | 4 | 4 | 5 | 5 | 6 |

---

## 16. Ship Systems (SSD Reference)

| System | Boxes | Function |
|--------|-------|----------|
| Warp Engines (L/C/R) | Many | 1 box = 1 energy point; destroyed box = −1 available power |
| Impulse Engines | 2–4 | 1 box = 1 impulse energy; max 1 pt/turn for movement |
| APR / Reactor | 2 | Energy only; cannot be used for movement |
| Batteries | 2–4 | 1 box = 1 energy storage; charged/discharged each turn |
| Phaser Capacitor | (virtual) | Stores phaser energy across turns up to stated capacity |
| Phasers (ph-1/2/3) | Per weapon | 1 box = 1 phaser; 1 damage pt destroys it |
| Photon Torpedo (PHOT) | Per tube | Tracked per tube (A and B independent) |
| Disruptor (DISR) | Per firing pt | Each box tracked separately |
| Plasma Torpedo (PL-S/R/F) | Per tube | Fixed or swivel mount (noted on SSD) |
| Drone Rack (DRN) | Launcher + 4 ammo | Ammo track cannot absorb internal damage |
| Shields #1–#6 | Per SSD | Box groups; cannot be targeted for internal damage |
| Bridge | 1 | Ship control; Emergency Bridge and Aux Control are backups |
| Sensors | Track 1–6 | Destroyed best to worst; last box never destroyed |
| Scanner | Track | Destroyed best to worst; last box never destroyed |
| Damage Control | Track 0–4 | Last box never destroyed |
| Transporters (TRAN) | Per unit | 5 transporters per box; range 5 hexes |
| Lab (LAB) | Per box | Scientific use; also needed for some advanced rules |
| Shuttle Bays (SHTL) | Per bay | Holds and launches shuttlecraft |
| ESG | Per unit | Lyran ships only |
| Tractor Beams | Per unit | Range 0–1 hexes |
| Excess Damage (EX DAM) | Outside hull | Destroyed last before ship destruction |

### Reference Ships (Cadet Game)
| Ship | Faction | Warp | Impulse | APR | Batt | Phasers | Heavy Weapons | Total Power |
|------|---------|------|---------|-----|------|---------|---------------|-------------|
| Federation CA (Constellation) | Federation | 16 | 2 | 2 | 2 | 3 × ph-1 | 2 photon tubes | 20 |
| Klingon D7 (Destruction) | Klingon | 16 | 2 | 2 | 2 | 4 × ph-2 + 2 disruptors | 1 drone rack | 20 |
| Romulan War Eagle | Romulan | 16 | 2 | 0 | 2 | varies | 2 plasma-S (fixed) | 18 |
| Romulan KR | Romulan | — | — | — | — | varies | 2 plasma (swivel) | 18 |

*(Standard game Fed CA: 30 warp + 4 impulse + 0 APR = 34 total)*

---

## 17. Ship Roles — AI System Design

| Role | Owns | Decision Domain |
|------|------|----------------|
| **Engineering** | EAF | Power allocation every turn: shields vs. weapons vs. movement vs. batteries vs. damage control |
| **Helm** | Movement | Speed selection, turn timing (turn mode satisfaction), approach/retreat geometry, arc management |
| **Weapons** | Arming + firing | Which weapons to arm (2-turn torpedo commitment), when/what impulse to fire, overload vs. standard, target selection, phaser capacitor management |
| **Tactical** | Overall | Relative position planning, when to close/break off, reinforcement decisions |
| **Science/Sensors** | Sensors + EW | Sensor lock-on, ECM/ECCM allocation, drone tracking, threat assessment |

---

## 18. Key Constants

| Parameter | Value |
|-----------|-------|
| Impulses per turn (standard) | 32 |
| Max speed | 31 |
| Max warp MPs | 30 |
| Max impulse contribution | 1 pt/turn = 1 MP |
| Max acceleration | max(prev speed, 10) per turn |
| Photon arming cost | 2 warp pts/turn × 2 turns = 4 total |
| Photon damage | 8 pts (standard) / 16 pts (overloaded) |
| Photon minimum range | 2 hexes |
| Photon maximum range | 30 hexes |
| Disruptor energy cost | 2 pts (standard) / 4 pts (overloaded) |
| Drone speed | 32 (max scenario speed) |
| Drone warhead | 6 pts (Cadet) / 12 pts (Standard) |
| Drone destruction threshold | 4 damage points |
| Drone endurance | 3 turns |
| Drone launcher capacity | 4 drones; 1 per turn |
| Plasma-S arming | 2+2+4 pts over 3 turns |
| Plasma-S endurance | 25 hexes |
| Shield count | 6 (one per hex facing) |
| Standard die | d6 |
| Life support cost | 1 pt/turn (standard ship) |
| Fire control cost | 1 pt/turn |
| Shield activation cost | 2 pts/turn |
| Transporter range | 5 hexes |
| Tractor beam range | 0–1 hexes |
