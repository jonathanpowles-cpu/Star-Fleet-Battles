"""
Situational doctrine — the "which playbook applies right now" layer.

Extracted cover-to-cover from the Captain's Tactics Manual (ADB5703) with page
citations. sfb_doctrine.py holds timeless principles; this module holds the
SITUATIONS (outnumbered, defending a base, fleet action, terrain, disengaging)
and a selector that picks the applicable doctrine for a concrete position.

Every string here traces to a cited page. Nothing is invented. Where the manual
is thin (disengagement has no dedicated article), that is flagged in place.

    from sfb_situations import situations
    for pri, tag, text in situations({"outnumbered": True, "race": "Lyran"}):
        ...
"""
from __future__ import annotations

IMPULSE_OF_DECISION = 25   # fire by this impulse or weapons won't recycle by #1 (p7)

# --------------------------------------------------------------------------
# Scenario framing — MUST be resolved before disengagement can be evaluated
# --------------------------------------------------------------------------
SCENARIO_KIND = {
    "campaign": "Ship has a past and a future: SURVIVAL OUTRANKS WINNING. Disengage "
                "rather than risk destruction. (p5)",
    "pickup":   "Patrol/pick-up battle: no reason to disengage; fight it out. (p5)",
}


def volley_pattern(objective):
    """MIZIA (several mini-volleys) vs MASSIVE (single alpha strike) — p11.

    Mizia strips WEAPONS but leaves power to escape, so the enemy disengages:
    you hold the field but get no kill. A massive volley leaves him WEAPONS but
    no POWER to disengage, so you get the kill. In wartime/campaign play, where
    the strategic objective is to DESTROY enemy ships, Mizia works to your
    long-run disadvantage. This is an explicit objective-dependent switch.
    """
    if objective in ("destroy", "campaign", "war"):
        return ("MASSIVE",
                "Objective is destruction: deliver ONE massive volley — it strips his POWER "
                "so he cannot disengage. Mizia would leave him alive to limp away. (p11)")
    return ("MIZIA",
            "Objective is control of the field: distribute the same weapons over consecutive "
            "impulses onto ONE DOWN shield — far more weapon hits per point. (p11)")


MIZIA_REQUIREMENT = (
    "Mizia requires the damage to strike a DOWN shield that STAYS facing you across the "
    "volleys. A ship that has just turned — and cannot turn again, short of an HET, for "
    "several impulses — is the prime target. This requires careful study of the impulse "
    "chart. (p11)"
)

MIZIA_FREE_VOLLEYS = (
    "Free extra volleys: seeking and direct-fire weapons are always separate; mine/T-bomb "
    "damage is a separate volley in the Movement Segment; hellbores may fire before AND/OR "
    "after other direct fire; enveloping plasmas resolve separately; each PPD counts as a "
    "separate volley even on the same impulse. (p11)"
)

MIZIA_DEFENCE = (
    "Defending against Mizia: general shield reinforcement is ineffective with one shield "
    "down and is countered by minimal damage on another shield. The best defence is to "
    "PRESENT ANOTHER SHIELD — usually a turn, sometimes just a speed increase or sideslip if "
    "the approach is oblique. Note hellbores, enveloping plasmas and plasmatic pulsars can "
    "still hit a non-facing down shield, and Hydrans put fighters out to BOTH flanks to "
    "guarantee a hit whichever way you turn. (p11)"
)

MIZIA_SPONGE = (
    "The empty-weapon sponge: if you expect a Mizia attack, fire your own mini-volley of one "
    "torpedo-hit weapon, two phasers and one drone-hit weapon so incoming damage falls on "
    "EMPTY weapons; repeat after each enemy mini-volley. This lets you keep closing while "
    "your firepower grows relatively stronger. (p11)"
)

# --------------------------------------------------------------------------
# SHIELD DOCTRINE — the manual is emphatic; the engine was silent on this
# --------------------------------------------------------------------------
SHIELD_DOCTRINE = {
    "first_volley": "NEVER take the first enemy salvo on the #1 shield. Maneuver so the "
                    "opening salvo lands on #2 or #6, preferably where it cannot penetrate. "
                    "You need #1 (with #2 or #6) intact to approach with weapons bearing. "
                    "(p16; Kosnett post-mortem p113: 'Taking the initial Klingon volley on the "
                    "#1 shield is never a good idea.')",
    "no_bonus": "There are no bonus points for winning with undamaged shields. If a drone will "
                "hit a non-facing shield strong enough to absorb it, and your defence phaser "
                "could instead hit a DOWN enemy shield — let the shield eat it. (p16)",
    "seventh_shield": "Hull boxes and expendable soft systems are the 'seventh shield'. Let a "
                      "drone penetrate a weak shield and fire the phaser at the enemy instead, "
                      "IF the ship's 'fat' can absorb the internals. (p16)",
    "drop_weak": "A shield down to a couple of points may be worth dropping entirely — best "
                 "when the enemy has fired most weapons and you are undamaged enough to absorb "
                 "internals. Lets you raise it next turn with reinforcement, and frees "
                 "transporters. (p16)",
    "repair_futile": "In-scenario shield repair is nearly futile. Only worth it to create a "
                     "couple of points that can then be reinforced, or to BALANCE two weak "
                     "shields so hellbore/enveloping-plasma damage spreads less "
                     "destructively. (p16)",
}

# --------------------------------------------------------------------------
# KAUFMAN RETROGRADE — the outnumbered-in-open-space answer (pp43-44)
# --------------------------------------------------------------------------
KAUFMAN_RETROGRADE = {
    "premise": "A fleet is strongest moving BACKWARDS at moderate speed. The enemy pursues, so "
               "both fleets' weapons face each other in a static battle at YOUR chosen range. "
               "So effective that a force up to TWICE your strength can still be defeated. (p43)",
    "preconditions": [
        "Open space ONLY — no base, planet, convoy or FRD to defend. A fixed asset anchors you "
        "and makes the retrograde unavailable. (p46)",
        "An enemy who wants to DESTROY your fleet, not merely push you out of the area. (p43)",
    ],
    "why": [
        "You control the range and never become decisively engaged; you need not enter overload "
        "range, let alone tractor range.",
        "Enemy seekers become near-worthless: at speed ~20 and range ~15, plasma is worthless and "
        "only high-speed drones are effective — and even those sit in the 2-3 hex kill range for "
        "several impulses.",
        "YOUR seekers become tremendously more effective: speed-8 drones become effectively "
        "speed-28 for no extra cost (employ in CONVERGING waves or he sideslips around them); "
        "plasma fired at range 15 strikes after travelling only 9 hexes.",
        "Enemy fighters are ineffective; you can leave mines behind you.",
        "Especially effective against the ISC — keeps their gunline in your range while keeping "
        "you outside the PPD's deadly 10-hex bracket.",
    ],
    "shield_rotation": "Do NOT move straight backwards. Keep #2 facing the enemy while "
                       "sideslipping away or moving at an angle; once #2 is damaged, turn to "
                       "bring #1 to bear while moving directly to the rear; once #1 is damaged, "
                       "turn again to bring #6 into action. Run damage control on down shields "
                       "throughout. If he is GAINING on you while you move at an angle, drop the "
                       "sideslips and move in a straight line. (p43)",
    "targets": ["crippled ships", "ships that will explode near other enemy units",
                "carriers re-arming fighters", "scouts", "ships with down shields"],
    "reversal": "At the decisive point, when the enemy is severely weakened, reduce speed "
                "drastically (or stop for part of a turn and reverse direction) and spend ALL "
                "energy on overloads and reinforced shields. He will coast into overload range "
                "before he realises what you are doing. Turns a retreat into a victory. (p43)",
    "weakness": "A badly crippled ship cannot keep formation AND cannot disengage, because it is "
                "moving in reverse. A moderately crippled ship should cease fire and spend all "
                "remaining power holding formation speed — its mission is to survive until the "
                "enemy gives up and goes away. (p43)",
    "counters": [
        "Attrition: follow spread on a wide front, use your seekers primarily against HIS "
        "seekers, mass fire on selected units.",
        "Charge, spending whatever energy it takes, using Erratic Maneuvers during the approach.",
        "Outflank at high speed outside his effective range — but you must outflank BOTH sides "
        "at once (or he simply sideslips away), and each flanking force must be strong enough to "
        "fight alone.",
        "BEST: attrition + outflanking simultaneously, charging to close range only at the "
        "decisive moment — and begin immediately. (p44)",
    ],
}

# --------------------------------------------------------------------------
# FIXED-ASSET DEFENCE — base / planet / convoy / FRD (pp45-47)
# --------------------------------------------------------------------------
FIXED_ASSET = {
    "anchor": "A planet is an anchor on your position: you CANNOT retrograde or move around. "
              "You must defend it from the attacker. (p46) — this is precisely why the Kaufman "
              "Retrograde is unavailable in fixed-asset defence.",
    "inhabited": "If the planet is INHABITED, using it as a shield costs the population you are "
                 "defending, and your own weapons can be diverted by enemy maneuvers into doing "
                 "the attacker's work for him. If UNINHABITED, use it freely as a shield and to "
                 "distract self-guiding seekers, with no victory-point loss. (p46)",
    "far_side": "BOTH sides must prevent the enemy slipping to the far side of the planet and "
                "causing self-guiding seeking weapons to accept the PLANET as their target. (p46)",
    "blind_side": "Bases on planets are blind on one side — they cannot be hit from that side but "
                  "cannot support it either. This enables divide-and-conquer: fighting on one "
                  "side leaves the other side's bases out of the battle but unable to escape. It "
                  "also tempts the defender to split forces. (p46)",
    "haven": "An attacker should destroy enough bases to create a 'haven' where his forces are "
             "safe from ground fire, then pick off the rest with fleets that expose themselves "
             "over the horizon simultaneously, just long enough to fire. (p46)",
    "atmosphere": "Entering atmosphere protects from radiation, explosions outside your hex, SFGs "
                  "and ESGs, and gives an ECM benefit. An outnumbered force can hide there and "
                  "send weapons and fighters out, or 'porpoise' — rise to fire, dive back for "
                  "protection. COST: speed limited to 1 hex per turn, and your own weapons fire "
                  "is degraded. (p46)",
    "assault": "Attacking a fixed installation: approach OBLIQUELY so a SIDE shield takes the "
               "damage; stop at the point that optimally trades your weapons' effectiveness "
               "against the base's; spend the movement energy you saved on shield REINFORCEMENT; "
               "TAC a fresh shield around as each is battered; eventually bring a rear shield "
               "around and depart to repair, returning to repeat. (p45)",
    "nebula_base": "Bases do well in nebula — a base with a couple of power boost pods can rip "
                   "ships to pieces, due to its phaser-4s. (p47)",
    "mauler": "Maulers vs bases: 'It's easy to get a base in your firing arc with a Mauler; just "
              "drive over there and blast it.' Romulans with CLOAKABLE maulers are especially "
              "useful — draw the base's fire with another ship, then uncloak the mauler for the "
              "coup-de-grace. (p28)",
    "plasma": "Plasma can be employed more easily against bases, which don't move, allowing you "
              "to select the range. Use ENVELOPING torpedoes against targets with many wild "
              "weasels (such as a base) for the extra collateral damage. (pp29-30)",
    "frd": "Docking one or more ships to an FRD lets it move at speeds up to 16 instead of 1, and "
           "supplies power to complete repairs — provided you have other undocked ships still "
           "protecting it. (p33)",
    "defender_edge": [
        "No need to reserve power for movement — it all goes to weapons and reinforcement.",
        "Effectively unlimited arming time; weapons can be held armed.",
        "Repair facilities and fighter/PF support on station.",
    ],
}

# --------------------------------------------------------------------------
# FLEET ACTIONS (pp38-40)
# --------------------------------------------------------------------------
FLEET_PRINCIPLES = [
    "A fleet or squadron is not a collection of single ships, but an organism in and of itself. "
    "A fleet with a unified plan will always beat an equal fleet of individual ships. Because "
    "fleet actions are so complicated, THE SIMPLEST PLANS ARE USUALLY THE BEST. (p38)",
    "Casualties are far higher than in duels: in a single-ship action a crippled ship can often "
    "escape while the enemy reloads; in a fleet action it won't survive that long. (p38)",
    "Weapon-cycle synergy: a Federation fleet firing HALF its ships each turn keeps up a "
    "continuous photon barrage, covering the two-turn reload; or fire all photons on one turn "
    "for a massive firestorm. A disruptor fleet has a massive EVERY-turn punch. (p38)",
    "Division of labour: assign one ship boarding parties, another tractors, another drone "
    "defence. A single ship must find power for everything or forego options. (p38)",
    "You must concentrate your firepower IN SPACE AND TIME. (p39)",
]

FLEET_ROLES = {
    "FF": "Frigate: short-ranged. Primarily protects the fleet from fighters, PFs and seeking "
          "weapons. Station BEHIND the battle line — it can kill approaching drones, is far "
          "enough back to avoid being targeted, anything reaching its overload range has already "
          "eaten the fleet's overloads, an explosion won't damage the line's enemy-facing "
          "shields, and fire lanes stay clear for your outbound drones. EXCEPTION: if your "
          "frigates cannot kill drones at range 2 (ADDs + ph-1s), put them in the SAME hexes as "
          "the cruisers. (p39)",
    "DD": "Destroyer: supports the frigates' defensive mission but must supplement cruiser "
          "firepower — often, destroyers fire the FOLLOW-UP volley. (p38)",
    "CL": "Light cruiser: place where it won't take the first punch. (p38)",
    "CW": "War cruiser: the weapons of a heavy cruiser but lacking durability — can survive ONE "
          "solid punch, rather than two. (p38)",
    "CA": "Cruiser: primary combat unit; can take a volley on a shield and keep fighting. (p38)",
    "DN": "Dreadnought: heaviest punch and best defences, but it is often better to let a cruiser "
          "take the heat and allow the DN, in the second or third position, to deliver the "
          "killing blow. (p38)",
    "CV": "Carrier / PF tender / space control ship: should NOT be risked until the fighter group "
          "is no longer operational, unless it can score a decisive blow. (p39)",
    "SC": "Scout: very vulnerable — stay well behind the line of contact, often needing a DD or "
          "war cruiser as escort. SMALLER scouts need to be farther from the enemy; LARGER "
          "scouts (better at offensive EW) must be closer to the front line so they can reach "
          "the enemy ships. A scout which cannot reach the seeking weapons approaching your "
          "fleet isn't able to perform that mission. (p38)",
}

FORMATIONS = {
    "armed_mob": "The most basic (and worst) formation: ships out of range to cover each other, "
                 "facing different shield arcs of the key target, out of effective range at the "
                 "key impulse. On contact ALL formations close ranks and may degenerate into "
                 "this. (p39)",
    "fed_battle_line": "DD CA DN CA CL, with FF FF FF FF behind: good photon fields of fire, "
                       "protects vulnerable rear quarters, compensates for wallowing turn modes. "
                       "Scout and/or small carrier ~4 hexes to the rear of the DN. Wide spacing "
                       "risk: an enemy could be at point-blank range on the DD without being in "
                       "overload range of the CL's photons — many players concentrate into two "
                       "or three hexes. (p39)",
    "double_diamond": "Protects the flanks and keeps the DN out until one or two ships have "
                      "exchanged fire; frigates kept out of the way; scout/carrier to the "
                      "rear. (p39)",
    "wagon_wheel": "Protects a valuable central unit (carrier, FRD). It cannot block direct fire, "
                   "but stops drone and fighter attacks and ensures any unit reaching overload "
                   "range of the centre has already been hit by the surrounding ships' overloads. "
                   "VERY restrictive — all ships turn at the turn mode of the most sluggish unit. "
                   "When protecting a carrier you are really protecting the FIGHTERS: they return "
                   "damaged, without weapons or chaff, and depart unable to fire "
                   "immediately. (p39)",
    "klingon_wing": "Exploits the 60-degree hex rows where most weapons bear. Drone ships on the "
                    "flanks (D5 position) to avoid firing through the formation. The whole "
                    "formation turns toward or away at the option point; if turning away, "
                    "frigates move forward first to screen against pursuit. Late-war ADD ships: "
                    "ADJACENT hexes let one unit's ADDs cover the whole wing but risk "
                    "neighbouring explosions; TWO hexes still permits overlapping ADD coverage "
                    "while eliminating that risk. (p40)",
    "kzinti_inverted_v": "Protects the heavy units initially; flanking ships launch drones "
                         "without routing them through the fleet and screen the heavies from "
                         "enemy drones. Unwieldy — keep the heavy core intact and let flanking "
                         "frigates keep station as best they can. (p40)",
    "talons_of_the_eagle": "Romulan / Gorn & Kzinti 'Claw': surrounds the enemy and fires plasma "
                           "or drones from several directions so he cannot outrun them and must "
                           "accept damage or go under wild-weasel restrictions — which can be "
                           "fatal, because the ships follow the seekers in for a direct-fire "
                           "pass. Splitting the formation is normally dangerous, but the seeker "
                           "waves deter attacks on a single talon. (p40)",
    "lyran_tight_cluster": "Lyrans cluster TIGHTLY so one ESG protects several ships (especially "
                           "vs hellbores) — but must maintain IDENTICAL speed and maneuvers to "
                           "avoid their own ESGs. Damaged ships cannot turn away, and ships "
                           "slowed by damage cause major problems. 'This is one of the reasons "
                           "why the Lyrans were semi-dominated by the Klingons: their ships are "
                           "individually superior but collectively inferior.' (p40)",
    "romulan_open": "More open formations, so several cloaked ships aren't in one small area — "
                    "one ship destroyed exposes others within the explosion radius. (p40)",
    "tholian_pinwheel": "Pinwheels and webs when outnumbered. (p40)",
    "wyn_mob": "The armed mob — everyone races to get in range before the radiation-zone effects "
               "wear off. (p40)",
}

CONCENTRATION = {
    "for": "Concentration is a dangerous but necessary element. The closer your ships, the more "
           "likely a target is in range and arc of all of them at once, the more likely they hit "
           "THE SAME SHIELD, and the less likely an enemy can reach overload range of one of "
           "yours without being hit by the overloads of your entire fleet. A concentrated fleet "
           "is also less vulnerable to seeking weapons. (p38)",
    "against": "More vulnerable to mines and ship explosions — though Captain's-edition explosion "
               "strengths are only 10-33% of the old edition, 'the single most profound change in "
               "fleet tactics', allowing fleets to operate much closer together. (p38)",
    "warning": "Spreading ships into adjacent hexes (or every other hex) may seem an effective "
               "compromise, but due to the short range of some weapons may dilute firepower to "
               "the point of IMPOTENCE. (p38)",
    "flanking": "Flanking maneuvers in fleet battles are generally UNPRODUCTIVE — they split your "
                "force; the enemy will simply run toward one element, destroying it before the "
                "second group can engage, and it lets him take your two forces' fire on two "
                "OPPOSITE shields. EXCEPTION: a valuable, vulnerable enemy unit (scout, carrier "
                "without heavy weapons, FRD) in his rear area — send a PF squadron or a fast war "
                "destroyer. (p32)",
    "flying_wedge": "The most basic fleet maneuver — heavily armed ships seeking LOCAL firepower "
                    "superiority in an area a few hexes across. Four ships, firing together, can "
                    "destroy one enemy ship per volley. (p32)",
    "deny": "Deny the enemy concentration by crippling key units — cripples cannot maintain fleet "
            "speed and must turn away to bring new shields to bear. (p38)",
    "float": "Ships need not be in rigid lockstep — they must float back and forth within the "
             "moving group to where they can best accomplish their mission. Ships approaching "
             "over a distance can spread out (with sideslips and speed changes), then concentrate "
             "again at the firing point. (p39)",
}

TARGET_PRIORITY = {
    "procedure": "Attrition procedure: concentrated firepower to BREAK a shield, followed by two "
                 "or three Mizia volleys to STRIP the target of weapons. Then move on to another "
                 "target. (p38)",
    "anti_pattern": "Having each of your ships fire on one enemy ship works fine with wet-navy "
                    "battleships, but is INEFFECTIVE with starships — neither shell-splash "
                    "observation nor suppression of return fire applies. (p38)",
    "cripples": "Cripples are ticking time bombs — maneuver your OWN cripples out of action, or at "
                "least out of the fleet's hex, so their explosion doesn't land on you. (p38)",
    "escort_death": "Sudden Escort Death Syndrome: in fleet battles there is so much firepower "
                    "available that it is often possible to completely explode a small enemy ship "
                    "with one massed volley, creating an explosion inside his formation and no "
                    "end of trouble. Use it; defend against it. (p6)",
    "sacrifice": "The sacrifice: deliberately send a frigate into a key area early in the turn to "
                 "draw the enemy into unloading his weapons before your real attack is revealed "
                 "by speed and course changes. Pay great attention to tactical intelligence "
                 "levels to conceal your true purpose. Lose something that won't cost you nearly "
                 "as much as it costs your opponent. (p32)",
    "drones": "Drone target selection: CLOSER targets are better — drones hit sooner, freeing "
              "control channels, and don't fly through the enemy formation where bypassed ships' "
              "rear weapons and extra lab attempts get at them. Enemy ships SEPARATED from their "
              "formation are good choices. A target moving TOWARD you is preferable to one moving "
              "away — the closing speed gives him fewer firing opportunities and less reaction "
              "time. (p18)",
    "morale": "If you can identify a target as a sentimental favourite, use this to your "
              "advantage. (p38)",
}

# --------------------------------------------------------------------------
# TERRAIN — force multiplier for the weaker side (pp46-48)
# --------------------------------------------------------------------------
TERRAIN = {
    "asteroids": {
        "use": "Fight here if you are the SMALLER force and want to make the enemy come to you; "
               "seeded with mines this is very effective. Against medium/fast drones or plasma, "
               "keep rocks between you and him — his weapons take damage passing through. Fight "
               "in a GAP between asteroids if possible (a tractor beam can help). Nimble ships "
               "do well.",
        "cost": "Asteroid damage lands on your #1 shield (unless moving in reverse); at high "
                "speed your phasers must shoot rocks instead of the enemy; allocate ECCM to "
                "counteract at least the asteroids' natural EW; long-range bombardment won't "
                "work — accept that you must close. Asteroids also stop a ship trying to build "
                "speed to disengage.",
        "cite": "p46",
        "lyran_warning": "LYRANS are at a particular disadvantage in asteroids — constant ESG "
                         "damage. (p46)",
    },
    "black_hole": {
        "use": "Blocks fire passing within 2 hexes and imposes EW penalties. An outnumbered force "
               "can stay on the opposite side — the enemy risks destruction to approach directly.",
        "cost": "A ship passing within 5 hexes must have speed >=13 or be pulled in and destroyed.",
        "cite": "p47",
    },
    "variable_pulsar": {
        "use": "An outnumbered force can stay on the opposite side. Damage usually destroys drones "
               "and most fighters and cripples PFs; pulsar damage CANNOT be negated by electronic "
               "warfare; hellbores, plasmatic pulsars and enveloping plasmas exploit the weak "
               "shield it creates. Trick: drop a web anchor and lay web three or four hexes "
               "directly away from the pulsar, and concentrate the fleet in the last web hex, "
               "where the effects will be nil after penetrating a hundred-odd points of web.",
        "cost": "It also BLOCKS attempts to disengage.",
        "cite": "p47",
    },
    "nebula": {
        "use": "The true equaliser between smaller and bigger ships — all shields are the same and "
               "weapons can quickly punch through.",
        "cost": "Smaller ships have fewer internals; larger ships use ECCM better. The nebula "
                "generates 9 points of ECM; you may raise your ECM to 15 OR cut his to 3, BUT NOT "
                "BOTH — a desperate guessing game. NON-FUNCTIONAL: tractors, transporters, webs, "
                "cloaks, ESGs, SFGs, DDs, scout functions, mines, ATG and type-VI drones. NO "
                "shuttles at all — no wild weasels, but also no scatter-packs or suicide "
                "shuttles. FIGHTERS CANNOT BE USED AT ALL.",
        "cite": "p47",
    },
    "radiation": {
        "use": "Radiation reduces maximum range for all combat operations to 25 hexes — "
               "eliminating long-range sniping but making DISENGAGEMENT BY DISTANCE far easier. "
               "Keep one ship out of the action to own the last survivors and capture crewless "
               "enemy vessels afterwards.",
        "cost": "Smaller ships are in trouble once shields are penetrated — crewless in 3-5 turns. "
                "Every effort must be made to maintain at least a box in EVERY shield; general "
                "reinforcement will NOT protect the crew.",
        "cite": "p48",
    },
    "heat_zone": {
        "use": "Tends to avoid decisive battles — once shields are down, ships take zone damage "
               "and quickly disengage rather than fighting to the death. Ideal terrain to choose "
               "against an ANDROMEDAN (he cannot clear his panels).",
        "cite": "p48",
    },
    "sunspots": {
        "use": "You cannot communicate or coordinate — disrupts carrier groups, PF flotillas and "
               "fighter groups; SCOUTS ARE NEUTRALISED since no EW can be loaned. Drone and "
               "plasma races gain, but overkill becomes more likely. If you can maneuver yourself "
               "AND a scout into the shadow zone, you receive ECCM and gain a firepower "
               "advantage.",
        "cite": "p48",
    },
    "dust_cloud": {
        "use": "Mostly a nuisance; limited EW benefit, easily countered. Decisive only against a "
               "ship with a down FRONT shield trying to disengage.",
        "cost": "An EM ship takes much more damage; the major problem is for CLOAKED ships, which "
                "are easily tracked in a dust cloud.",
        "cite": "p48",
    },
    "combined": "Mines inside asteroid groups or in the gaps force slow movement (use explosive "
                "rather than captor mines in a belt). Mines + heat/radiation zone: the mine drops "
                "the shield, the zone does the rest. At planets, mines around a high-sunspot star "
                "let the defender manoeuvre freely through the field while planetary bases fire. "
                "Cast web + asteroids is 'very powerful' — long-lasting web and no one "
                "leaves. (p48)",
}

# --------------------------------------------------------------------------
# DISENGAGEMENT
# NOTE ON SOURCE COVERAGE: the Captain's Tactics Manual has NO dedicated article
# on disengagement. The following is assembled from rules and advice scattered
# across the manual, each individually cited. Treat as thinner and less
# authoritative than the sections above.
# --------------------------------------------------------------------------
# --------------------------------------------------------------------------
# C7.0 DISENGAGEMENT — the HARD RULES (Master Rulebook), as distinct from the
# Tactics Manual's scattered advice below. These are testable preconditions:
# the engine must not advise disengaging when it is not actually legal.
# --------------------------------------------------------------------------
DISENGAGE_METHODS = {
    "separation": {
        "cite": "C7.2",
        "cost": "Instant — any impulse, at the Lock-On Stage. The cheapest method by far.",
        "gates": [
            (50, "must be MORE than 50 hexes from any enemy ship (C7.21)"),
            (75, "must be MORE than 75 hexes from an operating enemy scout (C7.23)"),
            (35, "must be MORE than 35 hexes from enemy PFs or manned shuttles (C7.26)"),
        ],
        "blocker": "Unresolved seeking weapons CAPABLE OF CATCHING YOU hard-block it (C7.22).",
        "note": "A cloak raises effective range, so a cloaked ship qualifies at shorter TRUE "
                "range.",
    },
    "acceleration": {
        "cite": "C7.1",
        "cost": "One and usually TWO full turns.",
        "gates": [],
        "requirement": "Needs >= 50% (or 15) warp power at end of turn. THE TRAP (C7.121): the "
                       "test uses power at the BEGINNING of the turn, so a ship shot down to its "
                       "current max speed must move another whole turn.",
        "restrictions": "Forward only; no blocking terrain ahead (C7.123).",
        "tractors": "Tractors do NOT stop it (C7.122) and are BROKEN by it (G7.28).",
        "firing": "The ship MAY still fire everything charged (C7.124) — unlike fighters, which "
                  "may not fire at all while disengaging (C7.131).",
    },
    "sublight_evasion": {
        "cite": "C7.3",
        "cost": "Once per turn; die roll of 3 or less.",
        "eligibility": "WARP-LESS ships only, and requires functioning impulse engines.",
        "modifiers": "Modifiers invert intuition: nearby FRIENDLIES help (-1 each within 35 "
                     "hexes); uncrippled ENEMIES within 15 hexes hurt (+1 each).",
    },
}

# The absolute bar — checked before any method is considered.
DISENGAGE_FORBIDDEN = {
    "bases": "C7.0: 'Bases and pinwheels (C14.14) cannot disengage.' A base has NO disengage "
             "option — remove it from the option set entirely.",
    "terrain": "A black hole or variable pulsar within 10 hexes — OR in the FA arc within 100 "
               "hexes — forbids disengagement entirely (P4.28, P5.351).",
}

DISENGAGE_SEPARATION_GATES = {"ship": 50, "scout": 75, "pf_shuttle": 35}


def disengage_check(rng_nearest_ship=None, rng_nearest_scout=None, rng_nearest_pf=None,
                    seekers_can_catch=False, is_base=False, terrain_veto=False,
                    warp_fraction=None, warpless=False, has_impulse=True,
                    friendlies_within_35=0, uncrippled_enemies_within_15=0,
                    warp_boxes=None, warp_max_boxes=None):
    """Which disengagement methods are actually LEGAL right now? (C7.0)

    Returns (available, blocked) — each a list of (method, explanation). The
    engine should never advise DISENGAGE when `available` is empty; it should say
    what is blocking instead.
    """
    available, blocked = [], []

    if is_base:
        blocked.append(("all", DISENGAGE_FORBIDDEN["bases"]))
        return available, blocked
    if terrain_veto:
        blocked.append(("all", DISENGAGE_FORBIDDEN["terrain"]))
        return available, blocked

    # --- Separation (C7.2): three simultaneous range gates + the seeker block ---
    sep_fail = []
    if rng_nearest_ship is not None and rng_nearest_ship <= 50:
        sep_fail.append(f"enemy ship at {rng_nearest_ship} (need >50, C7.21)")
    if rng_nearest_scout is not None and rng_nearest_scout <= 75:
        sep_fail.append(f"operating enemy scout at {rng_nearest_scout} (need >75, C7.23)")
    if rng_nearest_pf is not None and rng_nearest_pf <= 35:
        sep_fail.append(f"enemy PF/manned shuttle at {rng_nearest_pf} (need >35, C7.26)")
    if seekers_can_catch:
        sep_fail.append("unresolved seeking weapons capable of catching you (C7.22)")
    if sep_fail:
        blocked.append(("separation", "Separation blocked: " + "; ".join(sep_fail) + "."))
    else:
        available.append(("separation", "Separation AVAILABLE (C7.2) — instant, any impulse, at "
                                        "the Lock-On Stage. Take it."))

    # --- Acceleration (C7.1) ---
    # C7.11 requires 50% OF ORIGINAL WARP *OR* FIFTEEN BOXES, whichever is less.
    # Testing warp_fraction >= 0.5 alone is too strict for a high-warp hull: a DN
    # with 40 warp needs only 15 boxes (37.5%), not 20. When the box counts are
    # available, use the min(ceil(50%), 15) rule; fall back to the fraction only
    # when they are not.
    accel_note = ("Acceleration available (C7.1) — one and usually TWO full turns. "
                  "TRAP (C7.121): the test uses power at the BEGINNING of the turn. "
                  "Forward only, no blocking terrain (C7.123). Tractors do not stop it "
                  "and are broken by it (C7.122/G7.28). You MAY still fire everything "
                  "charged (C7.124).")
    if warp_boxes is not None and warp_max_boxes:
        need = min(-(-warp_max_boxes // 2), 15)      # ceil(50%) capped at 15
        if warp_boxes >= need:
            available.append(("acceleration", accel_note
                              + f" (have {warp_boxes} warp, need {need} = min(50% of "
                                f"{warp_max_boxes}, 15) per C7.11.)"))
        else:
            blocked.append(("acceleration", f"Acceleration blocked: {warp_boxes} warp "
                            f"boxes, need {need} (min of 50%-of-{warp_max_boxes} and 15, "
                            f"C7.11)."))
    elif warp_fraction is None:
        blocked.append(("acceleration", "Acceleration: warp power unknown — cannot "
                                        "verify the 50%-or-15 requirement (C7.11)."))
    elif warp_fraction >= 0.5:
        available.append(("acceleration", accel_note))
    else:
        blocked.append(("acceleration", f"Acceleration blocked: warp power at "
                                        f"{warp_fraction:.0%}, need >=50% or 15 (C7.1)."))

    # --- Sublight evasion (C7.3): warp-less ships only ---
    if not warpless:
        blocked.append(("sublight", "Sublight evasion not applicable — it is for WARP-LESS ships "
                                    "only (C7.3)."))
    elif not has_impulse:
        blocked.append(("sublight", "Sublight evasion blocked: requires functioning impulse "
                                    "engines (C7.3)."))
    else:
        mod = uncrippled_enemies_within_15 - friendlies_within_35
        need = 3 - mod
        available.append(("sublight",
                          f"Sublight evasion available (C7.3) — once per turn, die <= 3 before "
                          f"modifiers; here you need <= {need} (-1 per friendly within 35: "
                          f"{friendlies_within_35}; +1 per uncrippled enemy within 15: "
                          f"{uncrippled_enemies_within_15})."))
    return available, blocked


# --------------------------------------------------------------------------
# Base-defence advantages, rules-verified (as distinct from the Tactics Manual)
# --------------------------------------------------------------------------
BASE_DEFENCE_RULES = [
    "Ground bases IGNORE atmosphere when firing (P2.722), while attackers eat a +1 phaser die "
    "shift and -25%/hex on photons and hellbores (P2.541/P2.542).",
    "Surface shields are x3 on two facings, or x2 on three (P2.731).",
    "Enveloping weapons are HALVED against a ground base — half is lost to the landscape "
    "(P2.7331).",
    "Ballistic seeker attack is capped at 4 hexes (P2.713).",
    "Bases repair (G17.0) and dock 26 units per module x 6 (C13.33).",
]

BASE_ATTACKER_EDGE = (
    "The counterweight, which contradicts common belief: S4.0 explicitly gives the ATTACKER the "
    "readiness edge — the base 'has no particular reason to know that TODAY is THE DAY.'"
)

DISENGAGEMENT = {
    "when": [
        "Objective framing FIRST: campaign scenarios (a past and a future) mean the survival of "
        "the ship is more important than winning; pick-up battles have no reason to disengage. "
        "The AI must know which kind of scenario it is in before evaluating disengagement. (p5)",
        "Weapons stripped but power intact -> disengage; this is the tactically prudent option "
        "after a Mizia stripping. (p11)",
        "At the Oblique option point, historically the ship that received the most damage would "
        "turn away and disengage. (p41)",
        "Long-range duels lead to disengagements — 'a victory, but not a decisive one.' (p17)",
        "WARNING SIGN (Fleet Captain Tom Carroll, five-time National Champion): 'If you put "
        "yourself into a position where you must HET, Weasel, or Emer Decel, you more than "
        "likely are losing the battle and are just prolonging your death.' (p5)",
    ],
    "how": [
        "Preserve every point of speed: the ONLY time you do not empty and refill your batteries "
        "with warp power is when you need every point of speed to disengage or run down a "
        "fleeing enemy. (p9)",
        "Cover the withdrawal with SEEKING WEAPONS: drones cost no energy beyond active fire "
        "control, draw fire away from the disengaging ship, close faster because he is pursuing, "
        "and either cut his reaction time or force him to break off pursuit. (p18)",
        "Mine the retreat: drop T-bombs out the shuttle hatch to destroy pursuing drones and "
        "deter a ship trying to prevent disengagement. (pp26,34)",
        "The oblique T-bomb trap: fire and turn 60 degrees at the option point; while running "
        "straight to satisfy the turn mode, seed T-bombs; then turn 60 again so the pursuer ends "
        "on the far side of your minefield. (p27)",
        "The T-bomb self-screen: drop a forward shield, transport a bomb one hex out, then move "
        "or sideslip ON TOP of it — it stays unarmed until you clear the hex and then draws "
        "pursuing drones across it. (p27)",
        "Turn the whole fleet away behind a frigate screen — in the Klingon Wing, if turning "
        "away the frigates often move forward before turning, to screen against pursuit. (p40)",
        "HET to break contact: the simplest and riskiest way to escape pursuit. Do it at the END "
        "of the turn to give the pursuer minimum time to react. Feigning disengagement with one "
        "HET and reversing into firing position with a second is 'an overly risky maneuver and "
        "costs too much power'. (pp32,37)",
        "Speed floor after a stop: accelerate to speed 10 if possible, at least speed 6 — speed "
        "6-10 allows you to reach disengagement speed in two more turns. (p44)",
        "Fed ships' fast reload buys the option to stay: a Federation ship, with two-turn "
        "reloading, can afford to remain at knife-fighting range while he reloads. A plasma ship, "
        "with three-turn reloading, can't afford to hang around — launch, run/cloak, reload, then "
        "attack again. (p29)",
    ],
    "prevent": [
        "The Gorn Anchor and tractors are the primary tool. (pp33-34)",
        "Place T-bombs IN FRONT of a fleeing enemy to cut off his escape: place two, since the "
        "second is guaranteed to do internals if he can't turn away — and if he can turn, he will "
        "come back to where you want him. The BRACKET (bombs in front of and behind) means no "
        "matter how he turns, sideslips, or HETs, he'll have to hit one of them. (p27)",
        "Deliver the massive single volley, not Mizia, when you want the kill — it leaves a ship "
        "with weapons but no power to disengage. (p11)",
    ],
    "blockers": [
        "Crippled ships in a retrograde formation cannot disengage, because they are moving in "
        "reverse. Conversely, crippled ENEMY ships can disengage, and you can't stop them. (p43)",
        "Emergency deceleration hands the enemy the option to disengage unpursued. (p44)",
        "A variable pulsar blocks attempts to disengage (p47); asteroids can stop a ship building "
        "speed for disengagement (p46); a dust cloud is decisive against a ship with a down front "
        "shield trying to disengage (p48).",
        "Radiation zones cap all combat operations at 25 hexes, making disengagement by distance "
        "far EASIER. (p48)",
        "Fighters mostly disengage by distance (p92); PFs must actually disengage — landing on a "
        "ship does not count (Campaign Designer's Handbook).",
    ],
    "golden_bridge": "Build a golden bridge for a fleeing enemy. (Roman proverb, quoted "
                     "approvingly at p5)",
}

# --------------------------------------------------------------------------
# WILD WEASEL — the manual is notably hostile to it
# --------------------------------------------------------------------------
WILD_WEASEL_CAUTION = (
    "Fleet Captain Mark Schultz, 1985 National Champion: 'Any opponent who has used a wild "
    "weasel against me has lost the initiative and has been destroyed within two turns.' (p5) "
    "The Kosnett post-mortem lists using a WW when other solutions existed as an ERROR — T-bombs "
    "are the most obvious alternative, but tractors, maneuvering to turn another shield toward "
    "the drones, or holding some phasers out of the initial volley could also have been "
    "done. (p113) Nonetheless: charge two or more WWs when entering combat against ships with "
    "seeking weapons, and think at least two WWs ahead, especially in fleet battles. (p7)"
)

WW_COUNTER = "Wild weasels are countered by TRACTORING the target ship before it launches. (p22)"

# --------------------------------------------------------------------------
# ESCORT / CONVOY
# --------------------------------------------------------------------------
ESCORT = [
    "Escorts should be on the FLANKS or REAR of a drone-launching formation. (p22)",
    "'Act as escorts for it; protect it and it will protect you' — aegis escorts give phenomenal "
    "drone/fighter defence. (p56)",
    "T-bomb barrier for a retreating fleet: in fleet battles your escorts could drop a row of "
    "T-bombs set for drones. Your ships could then retreat through or behind the barrier, shaking "
    "off a massive drone attack for little cost. (p26)",
]

# --------------------------------------------------------------------------
# THE CONSUMPTION PRINCIPLE (p4)
# --------------------------------------------------------------------------
CONSUMPTION = (
    "Every irreversible resource spent is a resource unavailable later: making your first HET "
    "(the only one with a bonus) means you won't have it if you need it later; using ammunition "
    "(shuttles, drones, PPTs, probes) means you won't have it later — 'It can be just as bad to "
    "lose a scenario with empty drone racks as it is to lose it with full drone racks'; turning "
    "your ship means you won't be able to change direction again until you have satisfied your "
    "turn mode. BUT: it would be just as wrong to use your systems, ammunition and options "
    "indiscriminately as to never use them. (p4)"
)


# ==========================================================================
# SITUATIONAL SELECTOR
# ==========================================================================
_FORMATION_BY_RACE = {
    "LYRAN": "lyran_tight_cluster",
    "KZINTI": "kzinti_inverted_v",
    "KLINGON": "klingon_wing",
    "FEDERATION": "fed_battle_line",
    "ROMULAN": "romulan_open",
    "THOLIAN": "tholian_pinwheel",
    "WYN": "wyn_mob",
}


def situations(ctx):
    """Select the doctrine applicable to a concrete position.

    ctx keys (all optional; a missing key simply skips its tests):
        outnumbered   bool  we are the weaker force
        defending     str   'planet' | 'base' | 'convoy' | 'frd' | None
        attacking     str   'planet' | 'base' | None
        scenario      str   'campaign' | 'pickup'
        objective     str   'destroy' | 'control'
        terrain       str   key into TERRAIN
        fleet_size    int   friendly ships in the engagement
        shield_down   bool  the TARGET has a down shield facing us
        our_crippled  bool  we have a crippled ship in formation
        seekers_in    int   incoming seeking weapons
        race          str   our race
        impulse       int   current impulse

    Returns a list of (priority, tag, text), most urgent first. Priority 1 is
    an immediate call to action; 4 is background context.
    """
    out = []
    g = ctx.get

    # Scenario framing gates everything about disengagement
    scen = g("scenario")
    if scen in SCENARIO_KIND:
        out.append((3, "FRAME", SCENARIO_KIND[scen]))

    # Volley pattern is objective-dependent (p11)
    if g("shield_down"):
        pat, why = volley_pattern(g("objective") or scen or "control")
        out.append((1, "VOLLEY", f"{pat} — {why}"))
        out.append((2, "VOLLEY", MIZIA_REQUIREMENT))
        if pat == "MIZIA":
            out.append((3, "VOLLEY", MIZIA_FREE_VOLLEYS))

    # Fixed-asset defence OVERRIDES the retrograde
    if g("defending"):
        out.append((1, "DEFEND", FIXED_ASSET["anchor"]))
        out.append((2, "DEFEND", FIXED_ASSET["far_side"]))
        out.append((3, "DEFEND", FIXED_ASSET["blind_side"]))
        out.append((3, "DEFEND", FIXED_ASSET["atmosphere"]))
        if g("defending") == "planet":
            out.append((2, "DEFEND", FIXED_ASSET["inhabited"]))
        if g("defending") == "frd":
            out.append((2, "DEFEND", FIXED_ASSET["frd"]))
        out.append((4, "DEFEND", "Defender's edge: " + " ".join(FIXED_ASSET["defender_edge"])))
    elif g("outnumbered"):
        # The retrograde is available ONLY in open space with nothing to defend
        out.append((1, "RETROGRADE", KAUFMAN_RETROGRADE["premise"]))
        out.append((2, "RETROGRADE", KAUFMAN_RETROGRADE["shield_rotation"]))
        out.append((3, "RETROGRADE", "Target priority while retrograding: " +
                    ", ".join(KAUFMAN_RETROGRADE["targets"]) + ". (p43)"))
        out.append((3, "RETROGRADE", KAUFMAN_RETROGRADE["reversal"]))
        if g("our_crippled"):
            out.append((1, "RETROGRADE", "WARN " + KAUFMAN_RETROGRADE["weakness"]))

    if g("attacking") in ("base", "planet"):
        out.append((1, "ASSAULT", FIXED_ASSET["assault"]))
        out.append((3, "ASSAULT", FIXED_ASSET["plasma"]))
        out.append((4, "ASSAULT", FIXED_ASSET["mauler"]))
        if g("attacking") == "planet":
            out.append((2, "ASSAULT", FIXED_ASSET["haven"]))

    # Fleet-scale doctrine
    n = g("fleet_size") or 1
    if n >= 3:
        out.append((2, "FLEET", FLEET_PRINCIPLES[0]))
        out.append((3, "FLEET", CONCENTRATION["for"]))
        out.append((3, "FLEET", TARGET_PRIORITY["procedure"]))
        out.append((4, "FLEET", CONCENTRATION["warning"]))
        form = _FORMATION_BY_RACE.get((g("race") or "").upper())
        if form:
            out.append((2, "FORM", FORMATIONS[form]))
        if g("our_crippled"):
            out.append((2, "FLEET", TARGET_PRIORITY["cripples"]))

    # Seeker defence
    if g("seekers_in"):
        out.append((1, "SEEKERS", WILD_WEASEL_CAUTION))

    # Terrain
    t = g("terrain")
    if t in TERRAIN and isinstance(TERRAIN[t], dict):
        d = TERRAIN[t]
        out.append((2, "TERRAIN", f'{t}: {d["use"]} [{d["cite"]}]'))
        if "cost" in d:
            out.append((3, "TERRAIN", f'{t} COST: {d["cost"]}'))
        if "lyran_warning" in d and (g("race") or "").upper() == "LYRAN":
            out.append((1, "TERRAIN", "WARN " + d["lyran_warning"]))

    # Timing
    imp = g("impulse")
    if imp and imp > IMPULSE_OF_DECISION:
        out.append((2, "TIMING", f"Past Impulse #{IMPULSE_OF_DECISION}: weapons fired now will "
                                 f"NOT recycle in time to fire on Impulse #1 next turn. (p7)"))

    out.sort(key=lambda r: r[0])
    return out


if __name__ == "__main__":
    demo = {"outnumbered": True, "race": "Lyran", "fleet_size": 4, "scenario": "campaign",
            "shield_down": True, "impulse": 28, "terrain": "asteroids", "seekers_in": 3}
    for pri, tag, text in situations(demo):
        print(f"[{pri}] {tag}: {text[:110]}...")
