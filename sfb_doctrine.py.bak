"""
SFB tactical doctrine — structured expert knowledge distilled from the Captain's
Tactics Manual (ADB5703). Consumed by sfb_advanced.py (decisions) and
sfb_brain.py (advice). Pure data + light heuristics; no game state.

This is the "why" behind sophisticated play; sfb_advanced.py applies it to a
concrete position.
"""

from __future__ import annotations

# --------------------------------------------------------------------------
# Key timing landmarks (impulse chart, 32-impulse turn)
# --------------------------------------------------------------------------
IMPULSE_OF_DECISION = 25   # last impulse a weapon can fire and still recycle by #1 next turn
IMPULSE_OF_TRUTH = 1       # a high-speed ship can swap power to weapons in EA and fire
                           #   before the target can change shield facings
FIRE_RECYCLE_QUARTER = 8   # no weapon fires twice within 1/4 turn (8 impulses)

# --------------------------------------------------------------------------
# Energy doctrine ("POWER AND ENERGY")
# --------------------------------------------------------------------------
# Housekeeping (pre-plot every turn so you never forget):
HOUSEKEEPING = {
    "life_support": (0.5, 1.5),   # by size class
    "shields": (1, 4),            # full strength
    "fire_control": 1,            # active; 0 = cannot fire
}

ENERGY_PRINCIPLES = [
    "Pre-plot housekeeping (life support + shields + fire control) first, always.",
    "Never waste power. Leftover fractions -> batteries -> reserve warp power.",
    "Batteries are vital: empty them in EA, refill from WARP so reserve stays warp-qualified.",
    "Phasers are more efficient power->damage than heavy weapons, EXCEPT the last "
    "arming turn of a multi-turn weapon, or when range/EW is unfavorable.",
    "Charge phaser capacitors EARLY; unfired rear/side phasers power the forward ones next turn.",
    "Don't dump spare energy into shield reinforcement by reflex — HET, tractor, EW, "
    "transporters, or reserve are usually better. Reinforcement is the last refuge.",
    "Speed is not a dumping ground either: buy exactly the speed your plan needs.",
    "Arming two standard weapons beats one overload — more flexible.",
    "Track the enemy's power: what he spent bounds what he can do.",
]

# 1 pt reserve ECM can save ~10 pts of shield reinforcement if it forces a miss.
RESERVE_EFFICIENCY = "1 reserve ECM > 10 shield reinforcement (if it causes a miss)."

# --------------------------------------------------------------------------
# The Mizia Concept — damage concentration
# --------------------------------------------------------------------------
# Distribute the SAME weapons as several small volleys on ONE down shield.
# Effect vs a single massive volley:
MIZIA = {
    "principle": "Several small volleys on one down shield -> more WEAPON hits "
                 "(cripples the enemy's ability to fight). One massive volley -> "
                 "more power/hull hits (cripples his ability to run).",
    "requirement": "The shield must be DOWN and stay facing you across the volleys "
                   "(target that just turned and can't turn again for several impulses).",
    "separate_volleys": ["direct-fire (one volley)", "seeking weapons (separate)",
                         "mines/T-bomb (Movement Segment, separate)", "hellbore (before/after)",
                         "enveloping plasma (separate)", "each PPD (separate)"],
    "defense": ["present another (fresh) shield — turn/sideslip/speed change",
                "fire your own mini-volley onto your empty weapons so hits waste there",
                "EW (ECM drones / MRS / wild weasel) to blunt later volleys",
                "general shield reinforcement is INEFFECTIVE with a shield already down"],
    "wartime_caveat": "Mizia leaves the enemy alive-but-toothless; if the goal is to "
                      "DESTROY (campaign), a massive volley may be better.",
}

# --------------------------------------------------------------------------
# Firepower doctrine
# --------------------------------------------------------------------------
FIREPOWER = [
    "Goal: maximum firepower INSIDE the enemy — knock a shield down, then concentrate.",
    "A medium-range shot on a DOWN shield beats a short-range shot on a strong shield.",
    "Know your damage tables by heart (anticipated damage); don't overkill a target.",
    "It's fine to withhold an unfired phaser — it complicates the enemy's 'after you fire' plans.",
    "Alpha Strike = max single-impulse firepower (all weapons + overloads). Time it.",
]

# --------------------------------------------------------------------------
# Firing-arc / maneuver doctrine
# --------------------------------------------------------------------------
ARC_DOCTRINE = {
    "forward_centerline": "Max firepower only with the target in the forward hex row "
                          "(Gorn BC, Hydran, unrefitted Kzinti). Closes range fast; "
                          "avoid that hex row when fighting one.",
    "FA_firepower": "Even firepower across the whole FA arc (Klingon, Lyran, Romulan). "
                    "Can use the Oblique Attack (60-deg-off rows).",
    "close_and_turn": "Close, fire, then turn 60 across the enemy bow -> fresh shield + "
                      "unfired weapons for a second salvo. But turning telegraphs your "
                      "next several impulses (unless you HET).",
    "sideslip": "Sideslips line up the forward hex row without a full turn.",
}

# --------------------------------------------------------------------------
# Combat checklist (Ardak Kumerian) — standard doctrine, violate when warranted
# --------------------------------------------------------------------------
COMBAT_CHECKLIST = [
    "Transporters ready for hit-and-run / T-bombs on downed shields.",
    "Post guards on critical systems (bridge, security, sensors, cloak, weapons).",
    "Labs identify drones in range.",
    "Charge batteries; empty in EA and refill from warp for reserve.",
    "Charge phasers early; unfired side/rear phasers feed forward ones next turn.",
    "Fire no later than Impulse #25 so weapons recycle by Impulse #1.",
    "Kill enemy shuttles when convenient — could be WW/SP/SS.",
    "Charge >=2 wild weasels vs seeking-weapon enemies; think two WWs ahead.",
    "Keep a point or two for tractor/negative-tractor auctions.",
    "Rule #14: ruthlessly violate any principle when the situation warrants.",
]

# --------------------------------------------------------------------------
# Named tactical concepts (glossary the advisor can invoke by name)
# --------------------------------------------------------------------------
GLOSSARY = {
    "alpha_strike": "Max single-impulse firepower (all weapons, overloads).",
    "battle_pass": "Move past the target so side/rear weapons fire and you reach its rear.",
    "battle_run": "Close, turn away at short range, bring rear weapons to bear.",
    "glory_zone": "A range bracket where you dominate (e.g. plasma bolts at r9-10, "
                  "outside enemy overload range).",
    "gorn_anchor": "Tractor the enemy to deny his wild-weasel during seeking-weapon movement.",
    "power_curve": "Surplus-power measure; a good power curve = more effective in combat.",
    "sicilian_knife_fight": "Very short range, low speed: overloads, tractor auctions, "
                            "reinforced shields.",
    "klingon_saber_dance": "Attrition via repeated attacks from beyond overload range.",
    "oblique_attack": "Move to one side so FA weapons bear without pointing dead-on.",
    "overrun": "Move through the target's hex — point-blank fire, rear weapons, mine drop.",
}

# --------------------------------------------------------------------------
# Per-race tactical fingerprints (weapon-driven doctrine)
# --------------------------------------------------------------------------
RACE_DOCTRINE = {
    "FEDERATION": "Photon alpha strike; can't afford full overload + speed same turn "
                  "(6/tube). Durable, balanced. Pick the decisive volley.",
    "KLINGON": "Disruptors + Oblique Attack; saber-dance at 13-15 hexes where disruptors "
               "beat photons; wing phasers trade arc for reach. Close for the kill on a down shield.",
    "ROMULAN": "Cloak + plasma + mines. Fight in the plasma glory zone (bolt r9-10). "
               "Sub-hunt cloaked enemies; use the ECM/speed yo-yo.",
    "GORN": "Heavy plasma, slow. Close to medium range; the Gorn Anchor (tractor to deny WW).",
    "KZINTI": "Drone swarms + scatter-packs; 360 drone fire. Overwhelm drone defenses.",
    "HYDRAN": "Hellbores (hit the down shield regardless of facing) + fighters on both "
              "flanks -> premier Mizia users. Close-combat maneuvering.",
    "LYRAN": "ESG at very close range forces forward-centerline play; charge ESGs before "
             "engaging, recharge some. Fusion knife-fight.",
    "THOLIAN": "Web control; defensive positioning; patient.",
    "ORION": "ECM + engine-doubling burst speed (DAC risk); hit the weakest enemy; hit-and-run.",
}
