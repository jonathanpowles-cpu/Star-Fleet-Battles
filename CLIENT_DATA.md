# CLIENT_DATA.md — the SFU Online Client's machine-readable rules data

Extraction: `sfb.jar` → `data/sfbol/` → `C:\Users\jonat\Projects\Star Fleet Battles\client_data` (53 files).

**Epistemic status.** This is the *client's own implementation* of SFB. It is authoritative for
what a live game will enforce, and it is machine-readable where the printed SSD/Annex books are
image scans that extract to nothing. It is **not** the rulebook. Where client and printed rules
disagree, say which you are relying on. Every format below was decoded by inspection and
cross-checked against `misc.chart` (the human-readable render of several of the same tables),
against `rules_txt/SFB_Module_G3.txt` (the Master Annex File), and against live save state.

---

## 0. File inventory

| Group | Files |
|---|---|
| Ship stats | `master_ship.chart` |
| Damage allocation | `ship_dac.table`, `pf_dac.table`, `dragon_dac.table` |
| Weapon charts | `weapons.chart`, `lmc_weapons.chart`, `omega_weapons.chart`, `module_c6_weapons.chart`, `module_e2_weapons.chart`, `module_e3_weapons.chart`, `module_y_weapons.chart` |
| Seeking weapons / expendables | `alphadrones`, `alphaantidrones`, `alphamines`, `alphaplasma`, `alphaplasmarack`, `missilerack`, `chaff`, `ewpod`, `HEAT`, `PST`, `TEM`, `PlasmaWhip`, `MassDriverMissiles`, `deathbolts`, `hyperdrone`, `kwt`, `qwt`, `magellanic_plasma`, `omega_imp_torps`, `omega_tachyon`, `omegadmt` (all `.expendable`) |
| Box / hit-type id spaces | `boxtypes.names`, `abbrev_boxtypes.names`, `hittypes.names`, `boxtype.defs`, `boxtypekind.category` |
| Sequence of play | `imp.act`, `decision.act`, `tourn_imp.act`, `tourn_decision.act`, `tourn_dec_abbrev.act` |
| Option-mount menus | `options.list` (index) + `optmounts.list`, `nwo_hdw.list`, `apr_hdw.list`, `awr_hdw.list`, `jind_box.list`, `power_option.list`, `mag_option.list`, `wyn_optmounts.list` |
| Misc player-facing charts | `misc.chart`, `turnmode.chart` |

**Not present** (checked, do not go looking again): no fighter chart / Annex #4 analogue, no
`impulse.actions`, no `AlphaDrones.lib`, no `expendable.types`, no `expendable.box_def`, no drone-rack
ammo-capacity table, no BPV/fleet-point machinery, no Annex #7E "best type" priority table.
Fighter speeds must come from `rules_txt/SFB_Module_G3.txt` Annex #4 (from line 9626), not from here.

---

## 1. `master_ship.chart` — Annex #3, the Master Ship Chart

Flat CSV, RFC-style quoting (fields containing commas are `"…"`-quoted). Two record kinds:

- **Section header** (3 fields): `Kzinti,R5,The Kzinti Battle Fleet (R5.0)`; also `General,R1,General Units`.
- **Unit row** (15 fields).

The General/R1 block is repeated verbatim at the head of *every* race section (`F-OP` occurs
identically 9×, at lines 31, 554, 950, 1232, 1929, 2175, 2563, 2822, 3143). **Key rows on
`(section, type)`, never on type alone.**

Section starts (line numbers): Klingon 579, Kzinti 1257, Lyran 2275, Federation ~150.

The printed header, from `rules_txt/SFB_Module_G3.txt:973` (repeated on every Annex #3 page):

```
Ship  G9.0  D7.0    S2.1  C6.5  C2.12 J1.42 R0.6 C3.3 Product        Year  C13.3 D5.2  F&E War  Notes
Type  Crew  Brdg    BPV   Break Move  Spare Size Turn Pubished  Rule  In    Dock  Explo CMD      Ship
      Units Partys        down  Cost  Shttl Class Mode          Nbr   Srvc  Pts   Str   Rating   Status
```

The CSV is this chart with *Product Published* and *Warship Status* dropped (16 printed → 15 CSV).

| # | Field | Meaning | Rule | Confidence |
|---|---|---|---|---|
| 1 | Type | Class designation | — | certain |
| 2 | Crew | Crew units. `X+Y` = crew + non-crew passengers. Boarding parties are included (2 BP = 1 CU) | G9.0 | certain (6/6 vs live save) |
| 3 | Brdg Partys | Boarding parties aboard | D7.0 | high |
| 4 | BPV | Build point value. `A/B` = economic/combat split (e.g. Kzinti FRD `200/50`, CVD `149/116`) | S2.1 | certain |
| 5 | Breakdown | HET breakdown die range (roll in range ⇒ breakdown). Bases/freighters `-` or `1-6` | C6.5 | certain |
| 6 | Move Cost | Decimal (`0.33`=1/3, `0.67`=2/3, `0.13`=1/8) | C2.12 | certain |
| 7 | Spare Shttl | **Spare** shuttles as `admin+fighters` — *not* SSD shuttle boxes. Suffixes A=assault/heavy fighter, B=bomber, G=ground assault, H=heavy fighter/transport, P=prospecting. Multi-term forms exist (`2+2+2+1`, Fed SCS) | J1.42 | high |
| 8 | Size Class | 1–5, `5*` = sub-size. Distribution over the file: 1 ×47, 2 ×307, 3 ×1231, 4 ×1143, 5 ×252, `5*` ×31 | R0.6 | certain |
| 9 | Turn Mode | `AA`/`A`…`F`, `-` = immobile | C3.3 | certain |
| 10 | Rule Nbr | Section-R rule / SSD number, with optional product suffix (`76-R7`, `Y6`, `old13`, `A6`, `32A`) — the dropped *Product* column folded in | — | high |
| 11 | Year In Srvc | Service year. Tokens `N-F`, `N-P`, `N-PF`, `N-SCS` = "same year fighters/PFs/SCS became available" | — | high |
| 12 | Dock Pts | Space the unit occupies when docked; `-` for bases | C13.3 | high |
| 13 | Explo Str | Explosion strength on destruction. `NN+` (bases, variable), compound `16+4`, or a rule ref (`F-SL` → `R1.33A`) | D5.2 | high |
| 14 | CMD Rating | **F&E** Command Rating (ships allowed in a battle) — *not* an SFB in-game stat. FF 3 / DD 5 / CA 8 / CC 9 / DN 10 / freighters 0 | S8.2 | high |
| 15 | Notes | Annex #3 Notes codes: `A` full Aegis, `LA` limited Aegis, `V` true carrier, `VH`/`VA`, `D%` extra special drones, `DB` drone bombardment, `E` carrier escort, `ML` maneuver limits, `MS`/`MW` minesweeper/layer, `N`/`N#` nimble / empire note, `P` PF tender, `R` refit, `S` shock, `T` troopship, `TG` tug, `X` X-tech, `Y1`/`Y2` service-date notes, `UNV`, `OS`, `L`, `U`. Client-only extras: `t`, `CJ`, `S(13)`, `Unique` | — | high |

Two earlier working assumptions were **wrong**: col 15 is Notes, *not* Commander's Options (the chart
carries no Commander's Options data at all); col 10 is the *Rule Number*, not an SSD page number.

Real rows:

```
Fed:      CA,43,10,125,5-6,1,3,3,D,4,130,8,18,8,
Fed:      DN,50,14,180,3-6,1.5,4,2,E,2,148,10,24,10,Y1
Kzinti:   CA,40,16,126,5-6,1,2,3,C,48,...
Lyran:    CA,42,12,133,5-6,1,1,3,C,4,...
Klingon:  D7,45,14,121,5-6,1,1,3,B,4,...
```

Note `CA` is turn mode **D** for the Federation but **C** for Kzinti and Lyran, and the Klingon D7 is
**B** — turn mode is a per-row attribute, never derivable from the hull letters. The same is true of
size class (Fed `DN-Scr` is `4*` while `DN` is `2`).

**Known client-vs-print candidates** (PDF text extraction is spatially garbled; check the page image
before acting): `F-OP` client `Dock 6, Explo 5, CMD 0` vs G3 `9, 6, 3` — every neighbouring row
(F-OL, HAC, HAM, HAV, HAP) matches exactly, so this one stands out. Fed `CA` service year: client
`130`, extracted G3 row `175`; Y130 is historically correct and the client separately lists
`CA+`=165 / `CAR+ay`=175, so column bleed in the extraction is the likely explanation.

---

## 2. DAC tables — `ship_dac.table`, `pf_dac.table`, `dragon_dac.table`

```
<first-die-roll>                      # 2 for ship (⇒ 2d6), 1 for pf/dragon (⇒ 1d6)
<one row per die roll>: repeated pairs  <hit-type-id>,<flag>
```

- **ship**: 11 rows (rolls 2–12) × 13 pairs = DAC columns **A–M**. Column M is id 13 *Excess Damage*
  on every row.
- **pf**: 6 rows (1–6) × 6 pairs. Column E is `34,-1` (*Any Box*) on all six rows.
- **dragon**: 6 rows × 4 pairs (Body/Wing/Tail/Claw), with **no** Excess Damage terminator.

`<hit-type-id>` indexes **`hittypes.names`**, *not* `boxtypes.names` — the two id spaces coincide for
the first entries then diverge (Sensor is 24 in hittypes but 22 in boxtypes). Do not cross-use them.

`<flag>`: **`1` = BOLD on the printed DAC ⇒ rule D4.31, the result may be scored only ONCE per volley**;
subsequent points rolling it shift right. `-1` = normal. (An earlier reading of this flag as
"underlined / damage is lost" was wrong.)

Verified against the `DAC` block of `misc.chart`, which wraps exactly the flag-`1` cells in
`<HTML><B><U>…</U></B></HTML>`:

```
ship_dac roll 7 : 27,-1 11,-1 18,-1 17,-1 9,-1 20,-1 10,-1 15,-1 28,-1 21,-1 5,-1 23,-1 13,-1
misc.chart roll 7: Cargo,F Hull,Battery,Center W En,Shuttle,APR,Lab,Phaser,Any W En,Probe,AHull,Any Weapon,Excess Damage

ship_dac roll 12: 25,1 26,1 3,1 21,1 11,1 12,-1 …
misc.chart roll 12: <B><U>Aux Control</U></B>,<B><U>Emer Bridge</U></B>,<B><U>Scanner</U></B>,<B><U>Probe</U></B>,<B><U>F Hull</U></B>,Right W En,…
```

**Resolution algorithm** (matches D4.221/D4.222): per internal damage point, roll 2d6 (1d6 for
PF/dragon), take that row, walk columns A→M left to right, take the first hit type of which the
target still has undestroyed boxes, skipping any flag-`1` entry already consumed this volley;
column M is the terminal sink.

**Wildcards / categories needing special handling**: 23 *Any Weapon* (D4.324), 28 *Any Warp Engine*,
34 *Any Box* (PF only) — resolved by owner choice, not a fixed box. 14 *Drone* and 22 *Torpedo* are
category results (D4.323: TORP = disruptor/photon/plasma-D/fusion/TR/plasma; DRONE = drone racks,
PPDs, web casters, hellbores, ESGs, PA panels, ADD).

Not encoded anywhere in client data: the D4.3221–3223 "every third hit on the best available type"
priority (Annex #7E), and the D4.321 phaser directional restriction — those live in the jar bytecode.

---

## 3. Weapon charts — `weapons.chart` and friends

Two parts separated by a blank line.

**(1) Index block**, one line per UI menu group: `<Group label>,<chartKey>,<chartKey>,…`

```
Phasers, Phaser1, Phaser2, Phaser3, Phaser4
Heavy Weapons, Disr, Photon, Plasma
Hydran, Hellbore, Fusion, FusionOL, FusionS
Lyran, ESG
```

**(2) Chart blocks**, blank-line separated:

```
<chartKey>,<Display Title>
<axis-header row>
<data rows…>
```

Three row shapes, identified by the first header cell:

- **`RANGE,<band>,… ` + rows keyed `1..6`** — 2-D lookup: **row = die roll, column = range band**,
  cell = damage. (Phaser-1/2/3/4, Fusion, FusionOL, FusionS, CosmicCloud, MindMonster.)
  ```
  RANGE,0,1,2,3,4,5,6-8,9-15,16-25,26-50,51-75     (Phaser1)
  1,9,8,7,6,5,5,4,3,2,1,1
  ```
  Read a *column* down to get the six die outcomes; the mean of the column is the expected damage.
  Phaser-4 has 13 bands (`0-3,4-5,6,7,8,9,10,11-13,14-17,18-25,26-40,41-70,71-100`), individually
  enumerating ranges 6,7,8,9,10.
- **`RANGE,<band>,…` + rows keyed by named mode** — the mode row carries either to-hit ranges
  (`1-5`, `NA`) or damage (`DMG-STD`, `DMG-OVLD`, `DMG-PROX`, `TO HIT`, `SPLASH`, `HIT`, `DMG-PEN`,
  `DMG-EXP`). (Disr, Photon, Hellbore, PPD, Axion, KKH/KKL, TTORP.) This is roll-to-hit + fixed
  damage, **not** a per-die damage curve:
  ```
  RANGE,0,1,2,3-4,5-8,9-15,16-22,23-30,31-40       (Disr)
  STD,NA,1-5,1-5,1-4,1-4,1-4,1-3,1-2,1-2
  DMG-STD,0,5,4,4,3,3,2,2,1
  OVLD,1-6,1-6,1-5,1-5,1-4,1-4,NA,NA,NA
  DMG-OVLD,10,10,10,8,8,6,0,0,0
  ```
- **`ROLL,1,2,…,6` + a `DAMAGE`/`DMG` row** — single-roll tables (Carronade, PlanetCrusher,
  MorayEel, SunSnake).

Gotchas: `NA` cells (cannot fire); free text spanning a row
(`DMG-OVLD,1 DMG PER 1/2 POWER,,,,NA,NA`, `REINF,+1 DMG PER 1 POWER > COST,,,,,,`); trailing `NOTE:`
rows (Hellbore: `NOTE: STD CANNOT FIRE AT RANGE 0`); band strings needing range parsing (`0`, `3-4`,
`26-50`, `41+`); inconsistent whitespace after the key (`KKL, LIGHT KINETIC…`).

The photon block is visibly **lossy**: it collapses standard and overload hit numbers into one
`STD/OLVD` row and carries a mangled `DMG-OVLD`. The printed E4.1 table gives overload its own row
(`1-6` at range 0-1, per E4.14/E4.43). This is one place where the *rulebook* wins.

`lmc_weapons.chart`, `omega_weapons.chart`, `module_{c6,e2,e3,y}_weapons.chart` use the identical
grammar with disjoint keys: module_e2 = pulse phasers (EPP/SPP/LPP), proton pulse emitters
PE/PD/PC/PB/PA, Arachnid, Helgardian, Mallaran, Imperium; module_e3 = Borak Type-M Megaphaser only;
lmc = Heavy/Medium/Light/Early Lasers plus Baduvai, Eneen, Maghadim, Uthiki.

---

## 4. `*.expendable` — seeking weapons

One grammar for every file:

```
NAME,<FamilyId>,<seekingClass>
SPEED,<speedCode>,<hexes per turn>                                # repeated; code may be a single space = default
CONTAINER,<code>[,<description>],<space:float>,<magnitude:int>    # repeated
PACKAGE,<code>[,<description>],<space:float>,<magnitude:int>      # repeated
```

```
alphadrones.expendable        alphamines.expendable
NAME,AlphaDrones,1            NAME,AlphaMines,5
SPEED, ,8                     SPEED, ,0
SPEED,S,12                    CONTAINER,NSM,1.0,1
SPEED,M,20                    CONTAINER,T-Bomb,1.0,1
SPEED,F,32                    PACKAGE, ,Real,1.0,1
CONTAINER,Type-I,Standard Drone,1.0,4     PACKAGE,D,Dummy,1.0,1
```

- `<FamilyId>` is the join key referenced by field 4 of `boxtype.defs`.
- `<seekingClass>`: 1 drone-like, 2 plasma-like, 5 mine, 6 static/decoy (chaff, EW pod). *Medium
  confidence* — inferred from which families carry which value.
- **CONTAINER arity varies**: `CONTAINER,Type-I,Standard Drone,1.0,4` (5 fields) vs
  `CONTAINER,Plasma-R,1.0,50` (4). Parse by testing whether field 3 is a float; if yes there is no
  description.
- `<space>` = rack space consumed (0.5/1.0/1.5/2.0/3.0/8.0), matching the `DronePackages` legend in
  `misc.chart`.
- **`<magnitude>` is family-dependent — do not type it generically:**
  - plasma: **warhead strength** (Plasma-R 50, -M 40, -S 30, -SS 48, -SL 16, -G 25, -GS 32, -GL 10,
    -L 22, -F 20; rack-launched Plasma-D 10, Plasma-K 5). Exact canonical SFB values → certain.
  - drones: **damage-to-kill / armour**, *not* endurance and *not* warhead. Type-I/II/III 4,
    heavy Type-IV/V 6, dogfight Type-VI 2, X-drones VII/VIII/IX 3/2/1, Type-H 6, ADD **0**.
    Proof it is armour: the same column on PACKAGE rows gives `Armor-0.5SP`/`Armor-1SP`/`Ext. Armor`
    = 4 and every explosive/probe/ECM package = 1, and ADD=0 matches "ADDs die to any hit" (E5.2).
  - PACKAGE rows generally: effect magnitude (armor packages `r/R/e/E` = 4, everything else 1;
    PST Single/Double/Triple = 12/24/36; HEAT = 12).
- **Drone warhead damage in points is NOT in the client data at all.** Neither is rack ammo capacity.
  Both need printed FD1.x.
- Drone PACKAGE codes: `d/x/X` Explosive 0.5/1/2 SP; `u/n/N` Null; `r/R` Armor; `e/E` Ext. Armor
  (0.0 space); `P` Probe; `M` MW; `C` ECM; `f/F` Swordfish; `s/S` Spearfish; `t/T` Starfish;
  `v/V` Stingray; `w` Stonefish; `ATG` Active Terminal Guidance (0.0); `XX` Extended Range (0.0).
  `misc.chart`'s `DronePackages` block is the player-facing legend; the `.expendable` file is a
  **superset** (t/T, v/V, w are absent from the legend).

Speed codes actually in use: AlphaDrones & AlphaAntiDrones ` `=8, `S`=12, `M`=20, `F`=32;
AlphaPlasma ` `=32, `-Sabot`=40; AlphaPlasmaRack ` `=32, `Sabot`=40; MissileRack ` `=10, `M`=20;
AlphaMines/chaff/ewpod = 0; omega_tachyon 18–36; HEAT 32/64.
**There is no `Speed Module: XX` string anywhere in the client data** — codes are bare single chars.

---

## 5. Id spaces — `boxtypes.names`, `abbrev_boxtypes.names`, `hittypes.names`, `boxtype.defs`, `boxtypekind.category`

- `boxtypes.names` (324 entries) and `abbrev_boxtypes.names` (324, same ids) — `Name=id`, long and
  abbreviated SSD box labels: `Bridge=1`, `Shuttle=9`, `Fighter=61`; abbrev `Brdg=1`, `Dam Con=4`.
- `hittypes.names` — `Name=id`, ids run to 41 (`Dragon Claw=41`) in 40 lines (file has no trailing
  newline / one id unused). **Separate id space** used only by the DAC tables: `Fighter Box=32`,
  `Sensor=24`.
- `boxtypekind.category` (285 lines) — `<box label>,<category>`, e.g. `APR,General`. UI grouping.
- `boxtype.defs` (257 lines):
  ```
  <boxId>=<f1>,<f2>,<f3>[,<expendableFamily>[,<rackSubtype>[,<mode>,…]]]
  1=1,12,0            # Bridge
  14=1,3,0,AlphaDrones
  15=1,1,1            # Phaser-1
  62=1,3,0,AlphaDrones,A
  ```
  - `f2` = system category: 1 direct-fire, 2 plasma, 3 drone, 5 warp, 6 impulse, 7 AWR, 9 APR,
    10 shuttle, 11 fighter, 12 control, 13 plasma rack, 15 mass driver, 16 special. *Medium confidence.*
  - `f3` = 1 only on phaser-family boxes (33–37 = Ph-1/2/3/4/G), i.e. "uses the phaser
    capacitor/phaser table". *High confidence.*
  - `f1` ∈ {1,2,3} — **unidentified.** f1=2 clusters exclusively on ids 197–254 (fighter/shuttle-mounted
    variants); the 1-vs-3 split (Ph-1 and Ph-4 = 1, Ph-2/3/G = 3; plasma = 1, photon/disruptor/
    hellbore/fusion = 3) has no defensible explanation.
  - Trailing `mode` entries are the arming options shown in the UI. `$NNN` cross-references another
    boxId (a Ph-1 degrading to `$34`/`$35`; a Plasma-R firing as `$43`/`$44`/`$45`); `^x.y` is a
    power/damage multiplier and `~x.y` a second multiplier of unknown role.
  - **Drone racks are bound per box, with a subtype letter**: 39=Drone-B, 62=A…67=G, 155=Drone-GX,
    160=DX, 161=Drone-H, 162=HX, 178=CX, 213/214=BX. ADD racks are a separate family
    (`AlphaAntiDrones`, subtypes ADD/ADD6/ADD12/ADD30). **Drone characteristics are per-rack, never
    per-ship** — a ship can carry several rack subtypes at once. What functionally distinguishes
    subtypes A–H is *not* encoded here; only the letter.

---

## 6. Sequence of play — `*.act`

Flat CSV, three fields: `<nodeId>,<parentNodeId>,<display text>`. First row is always
`Acts,root,Activities`. Node ids encode the printed sequence-of-play numbering, so the id itself
carries the hierarchy and the parent field makes it a renderable collapsible outline. Rows with
empty id *and* parent (`,,text`) are unattached commentary.

```
Acts,root,Activities
6,Acts,6. IMPULSE PROCEDURE
6A,6,6A. MOVEMENT SEGMENT
6A1,6A,6A1: INVOLUNTARY MOVEMENT STAGE
6A1.01,6A1,Move playing pieces in using black hole rules (P4.1).
,,In each of the following steps, allocate the damage (D4.0) as it is resolved, step by step.
```

Top-level structure (`imp.act`, 150 lines):

- **6A MOVEMENT** — 6A1 involuntary, 6A2 voluntary, 6A3 damage during movement, 6A4 final movement actions
- **6B IMPULSE ACTIVITY** — 6B01 initial, 6B02 cloak, 6B03 lockon, 6B04 ship system functions,
  6B05 scout functions, 6B06 seeking weapons, 6B07 marines, 6B08 shuttle & PF, 6B09 satellite ships,
  6B10 separations, 6B11 final functions
- **6C DOGFIGHT RESOLUTION INTERFACE** — 6C01–6C13, flat
- **6D DIRECT-FIRE WEAPONS** — 6D1 allocation, 6D2 fire, 6D3 web caster, 6D4 damage resolution, 6D5 consequences
- **6E POSTCOMBAT** — 6E1–6E3

Key ids for anyone ordering actions: plasma launch **6B06.03**, drone launch **6B06.05**, probes
6B06.06, chaff **6B06.07**, ESG **6B06.08**, shuttle/fighter/WW launch **6B08.05**, mines 6B10.03 —
all inside 6B, i.e. **strictly before** direct-fire declaration 6D1.02/6D1.03 and firing 6D2.04.

The six files are **filtered projections of one master tree with identical ids**:

| File | Lines | Content |
|---|---|---|
| `imp.act` | 150 | full sequence of play |
| `decision.act` | 101 | `imp.act` minus purely automatic steps — drops all of 6A1.*, drops 6A3.01–05/07–11, keeps 6A3.06 (controlled mines can be *ordered* to detonate). This is the client's own **player-decision** subset |
| `tourn_imp.act` | 94 | tournament: non-tournament content stripped and texts rewritten |
| `tourn_decision.act` | 46 | tournament decision subset |
| `tourn_dec_abbrev.act` | 44 | same, prose compressed to button labels (`6B01.02,6B01,Change fire control status`) |

**No structured per-impulse gating.** Impulse-number restrictions appear only inside display prose —
`6A1.04,6A1,Andromedan ships take nebula damage (P6.31) on impulses #8 and #24.`,
`,,(Only on impulses #4, #12, #20, #28.)`. A parser can render but not evaluate them; the gating logic
is in the jar bytecode. Likewise, nothing here proves the client *enforces* the order — these files
are pure display data (no flags, no conditions).

---

## 7. Option-mount menus — `options.list` + the `*.list` files

`options.list` is the index: `<optionName>,<boxId>,<filename>`.

```
option_mounts,55,optmounts.list
nwo_mounts,157,nwo_hdw.list
apr*_mounts,158,apr_hdw.list
awr*_mounts,159,awr_hdw.list
jind_box,261,jind_box.list
pwr_option,262,power_option.list
mag_option,239,mag_option.list
wyn_option_mounts,324,wyn_optmounts.list
```

Each target file is `<count>,<item>,<arc>` — arc is empty for non-arc systems:

```
1,Phaser-1,FA
1,Drone-A,
1,Shuttle,
```

These are **purchase/mount menus**, not performance data. They are the reason `Shuttle` and `Fighter`
appear as options in the client, and they carry no speeds or stats.

---

## 8. `turnmode.chart` and `misc.chart`

`turnmode.chart` — `<category>=<speed band>,<speed band>,…`; the position of a band is the turn mode
number (hexes that must be moved before turning).

```
Seeking Weapon=1-32
Shuttle=1-11,12-23,24+
D=2-4,5-8,9-12,13-17,18-24,25+
```

Read as: a turn-mode-D ship at speed 5–8 must move 2 hexes between turns, at 25+ it must move 6.
This matches the engine's corrected `_TURN_MODE_BRACKETS` exactly. `misc.chart` carries the same
table transposed (`TurnModes` block, `Turn Mode, Seeking Weapon, Shuttle/Fighter,AA,A,B,C,D,E,F`),
which also gives the shuttle/fighter turn mode (1 hex to speed 11, 2 to 23, 3 above).

`misc.chart` (207 lines) uses a related but distinct grammar: an index block naming sections, then
blank-line-separated blocks of `<key>, <Title>` + free-form CSV. Blocks: `HR`/`HRvsGuards`/… hit &
run, `SmallMod` target size modifiers, `WW` collateral damage, `TurnModes`, cloaking lock-on
(`LockOnEquation`, `RangeFactor`, `SpeedFactor`, `RetainLockOn`, `GainingLockOn`, `AdjustRoll`),
`DronePackages`, `DAC`, `LAB`. Three entries are **image pointers** rather than data:
`ARCS, FIRING ARCS,data/sfbol/charts/firing_arcs.gif` (also `WING-ARCS`, `BOOM-ARCS`).

`SmallMod` is worth noting for fighter work: it bands `Shuttles & Heavy Fighters,0-11,12-24,25+`
separately from `Fighters & Drones,0-9,10-19,20+`.

---

## 9. Parsing checklist

1. `master_ship.chart`: split on section headers first; key `(section, type)`; RFC-quoted fields.
2. DAC: read the first line to learn the dice; ids → `hittypes.names`; flag `1` = once-per-volley.
3. `weapons.chart`: dispatch on the first header cell (`RANGE`+numeric rows / `RANGE`+mode rows / `ROLL`).
4. `.expendable`: CONTAINER arity is variable; `magnitude` semantics depend on the family.
5. `boxtype.defs` field 4 joins to `NAME,<FamilyId>` in the `.expendable` files; field 5 narrows to
   the rack subtype.
6. `.act`: `id,parent,text`; ignore `,,` rows or attach them to the previous node.
