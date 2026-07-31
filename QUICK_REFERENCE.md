# Star Fleet Battles — Quick Reference

## Setup Screen

| Key / Action | Effect |
|---|---|
| Click ship row | Toggle ship included / excluded |
| Click **CTRL** button | Cycle controller: Human → AI → Human |
| `[` / `]` | Decrease / increase turn count (min 4, max 32) |
| `S` | Save scenario (Windows file dialog) |
| `L` | Load scenario (Windows file dialog) |
| Click **START** | Begin game |

---

## Energy Allocation (Planning Phase)

One round per ship, human ships first.

| Key / Action | Effect |
|---|---|
| Click `+` / `−` buttons | Adjust each power allocation |
| Scroll wheel | Scroll allocation list |
| **COMMIT ALLOCATION** (top) | Lock in allocations and move to next ship |
| `Tab` | Hide / show EAF panel (see map underneath) |

**Power budget:** Total power = warp + impulse + APR + battery. The bar at the top of the panel shows remaining power. Commit is only available when you are not over-budget.

**Weapons:**
- Instant weapons (phasers) — allocate power each turn, fire any time during Impulse.
- Armed weapons (photons, disruptors, plasma, drones) — allocate arming power over 1–3 turns; they fire automatically when armed, or you can hold.
- Seeking weapons (drones, plasma, fighters) — once launched they fly independently.

---

## Impulse Phase (Movement & Combat)

### Movement

| Action | Effect |
|---|---|
| Click your ship | Select it; valid move hexes highlight green |
| Click a green hex | Move ship there |
| Click ship again (selected) | Rotate facing without moving |

Movement allowance = speed ÷ 32 hexes per impulse, roughly 1 hex per 4 impulses at speed 8.

### Firing

| Action | Effect |
|---|---|
| Click your ship | Select it |
| Click a weapon in the sidebar | Select weapon; fire arc highlights |
| Click a target ship in the arc | Fire |

### Keys (Impulse Phase)

| Key | Effect |
|---|---|
| `Space` | Advance one impulse |
| `Tab` | Toggle SSD (System Status Display) / sidebar |
| `L` | Toggle LLM crew advice on / off |
| `Escape` | Quit |

---

## Weapons Reference

| Weapon | Range | Damage | Notes |
|---|---|---|---|
| Ph-1 (Phaser 1) | ≤15 | Up to 10 | Drops off with range |
| Ph-2 (Phaser 2) | ≤15 | Up to 5 | Drops off with range |
| Ph-3 (Phaser 3) | ≤15 | Up to 2 | 360° arc |
| Photon Torpedo | ≤15 | 8 | 2-turn arm, forward arc |
| Disruptor | ≤15 | 6 / 3 | 1-turn arm; 6 at ≤8, 3 at ≤15 |
| Plasma-F | ≤10 | 20−2×range | 2-turn arm, seeking |
| Plasma-R | ≤15 | 30−2×range | 3-turn arm, seeking |
| Plasma-G | ≤10 | 10−range | 2-turn arm, seeking |
| Drone | ≤20 | 8 flat | 1-turn arm, seeking, 360° |
| Hellbore | ≤10 | 12 / 8 | 1-turn arm; 12 at ≤5 |
| Gatling Phaser | ≤15 | Up to 15 | High-power Ph-1 curve |
| Fusion Beam | ≤6 | 10 / 5 | 1-turn arm; 10 at ≤3 |
| ESG | ≤2 | = allocated | Area, all ships in radius |
| Fighter | contact | 6 | Seeking, recalls each turn |

---

## Damage & Shields

- Ships have 6 shields (one per facing). Hits on the facing shield reduce shield strength first.
- Once a shield is down, hull boxes are destroyed (internal damage — DAC roll).
- **ECM** reduces incoming damage; **ECCM** counters enemy ECM. Effect is halved at range ≤3, increased 50% at range ≥9 (Aegis D13.0).
- Repair costs 3 power per hull box per turn.

---

## Crew Advice Panel (bottom-left, human ships only)

Shows tactical advice from your bridge crew (AI-generated). During EAF you can click a crew name to rename them.

---

## Factions at a Glance

| Faction | Signature Weapon | Notes |
|---|---|---|
| Federation | Photon Torpedo | Durable, balanced |
| Klingon | Disruptor | Fast, aggressive |
| Romulan | Plasma-R | Can cloak; slow arm |
| Gorn | Plasma-F / Plasma-G | Heavy plasma, slow ships |
| Kzinti | Drone | Drone swarms, 360° |
| Hydran | Hellbore + Fighters | Fighters recall each turn |
| Lyran | Fusion + ESG | Close-range specialist |
| Orion | Mixed | Engine double for burst speed (33% DAC risk) |
| WYN | Mixed | Radiation zone bonus |
| Andromedan | Mixed | Power absorption |
