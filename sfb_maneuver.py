"""
High Energy Turns, Tactical Maneuvers and Erratic Maneuvers — the risk/benefit
layer the engine was missing.

Rules from the Master Rulebook (C6.0 HET, C6.5 breakdown, C10.0 EM, C5.0 TAC);
tactical judgement from the Captain's Tactics Manual pp35-37, 44-45 (cited
inline). The headline findings the engine must act on:

  * A HET is almost never the right answer. Two warp TACs plus a sublight TAC
    turn the ship 180 degrees — "the full effect of an HET at no risk of
    breakdown" (p45). The engine should offer the TAC alternative EVERY time it
    considers a HET.
  * Erratic Maneuvers are usually a bad trade. Any ship with movement cost >= 2/3
    gets the same 4 ECM for equal or less cost by simply allocating 4 points to
    ECM, with none of EM's restrictions (p36).
  * EM against a drone-armed enemy is close to suicidal: "The strongest counter
    to an EM ship is a mass drone launch — it is virtually defenceless" (p36).
    This matters directly when facing the Kzinti.

Breakdown ratings are per ship class and live in the Master Annex, not in the
tactical save. Where the rating is unknown this module says so rather than
guessing — an explicit gap beats a plausible invention.
"""
from __future__ import annotations

HET_COST_HEXES = 5          # warp power equal to five hexes of movement (C6.0, p36)
HET_BONUS = -2              # once-per-scenario breakdown bonus (p36)
EM_COST_HEXES = 6           # most ships; nimble/computer-controlled pay 3 (p35)
EM_COST_NIMBLE = 3
EM_ECM = 4                  # 4 points of naturally-produced ECM = +2 shift (p35)
EM_SEEKER_HALVING = 0.33    # 33% chance a seeking weapon that hits does 50% damage

WARP_TAC_PER_TURN = 4       # each costs one hex of warp movement (p45)
SUBLIGHT_TAC_PER_TURN = 1   # costs one impulse point regardless of ship size
TAC_FIRST_WARP_IMPULSE = 2  # earliest a warp TAC is available
TAC_MAX_GAP = 8             # two warp TACs may be up to 8 impulses apart

# --------------------------------------------------------------------------
# The only three tactically sound reasons to HET (p36)
# --------------------------------------------------------------------------
HET_REASONS = {
    "escape": "Retreating from a suddenly created problem (inbound R-torp, a wave of "
              "drones, a solidifying web wall, an Andromedan displacing in front of you). "
              "The object of the HET is to BUY TIME. (p36)",
    "shield": "A need for a new shield facing in a fight. (p36)",
    "surprise": "A surprise attack — best against an opponent trapped into following you: "
                "slow enough not to catch you, too fast to launch a wild weasel. Especially "
                "effective immediately after the opponent makes a turn. (p37)",
}

HET_BEST_MOVE = ("Best of all: avoid putting your ship into situations where an HET is your "
                 "only alternative. (p37)")

HET_WARNING_SIGN = ("Tom Carroll, five-time National Champion: 'If you put yourself into a "
                    "position where you must HET, Weasel, or Emer Decel, you more than likely "
                    "are losing the battle and are just prolonging your death.' (p5)")


def het_assessment(ship, reason=None, breakdown_rating=None, bonus_used=False,
                   impulse=1, expected_damage_if_not=None, expected_breakdown_cost=None):
    """Should this ship HET? Returns a list of (priority, text) notes.

    breakdown_rating: the ship's C6.5 rating (lower is safer). None = unknown,
        which is reported honestly rather than assumed.
    expected_damage_if_not / expected_breakdown_cost: if both are supplied the
        risk gate is evaluated numerically; otherwise it is stated as a rule.
    """
    out = []

    # Hard legality first. C6.37: "No unit may make an HET on the first impulse
    # of the turn" (also while docked/docking/undocking, in a pinwheel, or
    # uncontrolled; the sole exception F2.135 is for seeking weapons, not ships).
    # The audit flagged this as a misplaced restriction, but the rule is real and
    # the code was right - only the citation was vague ("p13").
    if impulse == 1:
        out.append((1, "You cannot HET on the first impulse of the turn (C6.37)."))
        return out

    if reason and reason not in HET_REASONS:
        out.append((2, f"'{reason}' is not one of the three tactically sound reasons to HET. "
                       f"Sound reasons: escape / shield / surprise. (p36)"))
    elif reason:
        out.append((3, f"HET reason ({reason}): {HET_REASONS[reason]}"))

    # Cost
    out.append((3, f"HET costs warp power equal to {HET_COST_HEXES} hexes of movement; unused, "
                   f"it is wasted — ships cannot afford to waste ~14% of their power. Standard "
                   f"solution: allocate part and rely on batteries for the balance. Using "
                   f"reserve warp for anything else means giving up the ability to HET. (p36)"))

    # Breakdown risk
    if breakdown_rating is None:
        out.append((2, "Breakdown rating UNKNOWN for this class (it lives in the Master Annex, "
                       "not the tactical save) — cannot compute HET risk. Treat as unsafe "
                       "unless you know the rating."))
    else:
        # Rating = the die roll on which the ship breaks down, so a LOWER rating is
        # MORE dangerous. Rating 5-6 with the -2 bonus becomes 7-8 (impossible) =
        # risk-free; rating 4 becomes 6, still a 1-in-6 chance.
        #
        # AMBIGUITY FLAGGED: the Tactics Manual extraction reads both "ships with
        # breakdown rating 5-6 can make their first HET risk-free using the bonus"
        # AND "ships rated 4-6 or worse can never make a safe HET even with the
        # bonus" (p36). Those are contradictory as written. The reading used here —
        # 5-6 safe with bonus, 4 or worse never safe — is the only one consistent
        # with the -2 arithmetic, but VERIFY against C6.5 in the Master Rulebook
        # before relying on it for a marginal call.
        eff = breakdown_rating + (0 if bonus_used else HET_BONUS)
        if breakdown_rating >= 5 and not bonus_used:
            out.append((2, f"Rating {breakdown_rating}: your FIRST HET is RISK-FREE using the "
                           f"once-per-scenario -2 bonus (effective {eff} — cannot be rolled). "
                           f"(p36) [reading of an ambiguous source; see C6.5]"))
        elif breakdown_rating >= 5 and bonus_used:
            out.append((1, f"WARN Rating {breakdown_rating} with the bonus already spent: this "
                           f"HET carries real breakdown risk. (p36)"))
        else:
            out.append((1, f"WARN Rating {breakdown_rating}: this ship can NEVER make a safe HET, "
                           f"even with the bonus (effective {eff} still breaks down). For it, the "
                           f"HET is a desperate maneuver. (p36)"))
        if not bonus_used:
            out.append((3, "Nimble ships (except PFs) and Orion warships get TWO bonuses. Bonus "
                           "policy: traditional is to hold it until you must avoid great damage "
                           "(guarantees the desperation HET works, and keeps you unpredictable); "
                           "aggressive is to plan the offensive HET before the scenario starts — "
                           "but then it won't cover an escape later. (pp36-37)"))
        out.append((4, "High-value low-breakdown units (e.g. the B10) should take the damage and "
                       "limp away crippled rather than HET. (p36)"))
        out.append((3, "A breakdown is tantamount to being crippled. In a single-ship duel it "
                       "will generally cost you the game; in a fleet action a ship which breaks "
                       "down is quickly singled out for destruction. (p36)"))

    # The risk gate
    if expected_damage_if_not is not None and expected_breakdown_cost is not None:
        if expected_damage_if_not > expected_breakdown_cost:
            out.append((1, f"RISK GATE PASSED: not HETing costs ~{expected_damage_if_not} damage "
                           f"vs ~{expected_breakdown_cost} from a breakdown. The risk is worth "
                           f"taking. (p36)"))
        else:
            out.append((1, f"RISK GATE FAILED: not HETing costs ~{expected_damage_if_not} damage, "
                           f"less than the ~{expected_breakdown_cost} a breakdown would cost. "
                           f"Do NOT HET. (p36)"))
    else:
        out.append((2, "Risk gate: the risk is only worth taking when NOT making the HET will "
                       "result in more damage than the breakdown would cause. Note that if you "
                       "HET to avoid an attack and FAIL, you take the breakdown damage AND the "
                       "attack. (p36)"))

    # The alternative the engine must always surface
    out.append((1, "ALTERNATIVE: two warp TACs (up to 8 impulses apart) plus a sublight TAC "
                   "turns the ship 180 degrees — the full effect of an HET at NO risk of "
                   "breakdown. Consider this first. (p45)"))
    out.append((4, HET_BEST_MOVE))
    return out


def tac_assessment(impulse, warp_tacs_used=0, sublight_used=False, decelerated_ago=None):
    """Tactical Maneuver availability and the 180-degree recipe (p45)."""
    out = []
    if impulse < TAC_FIRST_WARP_IMPULSE:
        out.append((2, f"Warp TACs are first available at impulse #{TAC_FIRST_WARP_IMPULSE}; "
                       f"a sublight TAC is available any time after impulse #1. (p45)"))
    remaining = WARP_TAC_PER_TURN - warp_tacs_used
    out.append((3, f"Warp TACs remaining this turn: {max(0, remaining)}/{WARP_TAC_PER_TURN} "
                   f"(one hex of warp movement each). Sublight TAC "
                   f"{'USED' if sublight_used else 'available'} (one impulse point, "
                   f"regardless of ship size — cheapest for DNs, expensive for small "
                   f"ships). (p45)"))
    out.append((2, "Warp TACs CANNOT be accumulated — earning the second loses the unused "
                   "first. (p45)"))
    if remaining >= 2 and not sublight_used:
        out.append((1, "180-degree turn available with NO breakdown risk: two warp TACs (up to "
                       "8 impulses apart) plus the sublight TAC. Warp and sublight TACs may be "
                       "combined in a turn but not on the same impulse — 120 degrees in two "
                       "impulses. (p45)"))
    if decelerated_ago is not None:
        if decelerated_ago < 4:
            out.append((1, f"WARN Emergency-decelerated {decelerated_ago} impulses ago: warp TACs "
                           f"require reserve warp power and a 4-impulse wait; a SUBLIGHT TAC "
                           f"needs only a 2-impulse wait"
                           f"{' — available now' if decelerated_ago >= 2 else ''}. Allocate one "
                           f"point of reserve IMPULSE power if you anticipate decelerating, so "
                           f"you can use both. (p45)"))
    return out


TAC_SITUATIONS = [
    "Fighting behind fixed terrain (web, minefield, friendly base) — a TACing Tholian pinwheel "
    "inside web is the 'Star Castle'.",
    "Attacking a fixed installation: come in on a side shield, stop at the optimum range "
    "trade-off, spend the movement energy on shield reinforcement, and TAC a fresh shield around "
    "as each is battered.",
    "As a defensive posture when outgunned and unable to outrun — best when you have the volume "
    "to take hits but lack firepower and maneuverability.",
    "To get an immediate decisive shot through a down shield at the start of a game-turn.",
    "After stopping mid-turn.",
    "To counter tractor rotations.",
]

TAC_CAUTION = ("Stopping gives far too much initiative to the enemy. Speed zero makes your next "
               "turn's position highly predictable. (p45)")

ZERO_ENERGY_TURN = ("Zero-energy turn: one 60-degree change on impulse #32 if you made no other "
                    "movement all turn. Costs nothing; only useful to badly damaged ships or as "
                    "elaborate subterfuge. (p45)")


# --------------------------------------------------------------------------
# Erratic Maneuvers
# --------------------------------------------------------------------------
EM_PROHIBITS = [
    "launching or guiding seeking weapons, shuttles, PFs or probes", "ESGs", "scout functions",
    "SWAC", "MRS-EW", "web generators", "SFGs", "maulers", "transporters", "tractors", "labs",
    "web laying", "docking", "separation", "pods", "mines",
]

EM_PENALTIES = ("Your own fire is equally penalised (unless computer-controlled); HET breakdown "
                "chance up; turn mode +1; mines auto-trigger; WILD WEASELS VOIDED; ADDs less "
                "effective. (p35)")


def em_assessment(ship, enemy_has_seekers=False, movement_cost=None, nimble=False,
                  purpose=None, at_max_ecm=False):
    """Should this ship use Erratic Maneuvers? Usually not — and the engine
    should say why, because the ECM alternative is strictly better for most ships.

    movement_cost: the ship's movement cost (e.g. 1.0, 2/3, 0.5). The p36 finding
        turns on whether it is >= 2/3.
    """
    out = []
    cost = EM_COST_NIMBLE if nimble else EM_COST_HEXES
    out.append((3, f"EM gives {EM_ECM} points of naturally-produced ECM -> a +2 shift against "
                   f"enemy direct fire, and a {int(EM_SEEKER_HALVING * 100)}% chance any seeking "
                   f"weapon that hits does only 50% damage. Greater effect on direct fire than "
                   f"on seekers. Cost: {cost} movement points"
                   f"{' (nimble)' if nimble else ''}. (p35)"))

    # The decisive objection for most ships
    if movement_cost is not None and movement_cost >= 2 / 3:
        if not at_max_ecm:
            out.append((1, f"BETTER OPTION: with movement cost {movement_cost:.2f} (>= 2/3), this "
                           f"ship gets the SAME 4 ECM for equal or less cost by simply allocating "
                           f"4 points to ECM — with NONE of EM's restrictions. (p36)"))
        else:
            out.append((2, "Already at max ECM, so the usual 'just allocate 4 ECM instead' "
                           "objection does not apply here. (p36)"))
    elif movement_cost is None:
        out.append((2, "Movement cost unknown — cannot test the p36 rule that any ship with "
                       "movement cost >= 2/3 should allocate 4 ECM instead of using EM."))

    # The hard veto
    if enemy_has_seekers:
        out.append((1, "WARN DO NOT USE EM: the enemy has seeking weapons. The strongest counter "
                       "to an EM ship is a mass drone launch — it is virtually defenceless, and "
                       "EM VOIDS wild weasels. On an approach spend the power on ECM instead so "
                       "you can shoot down the drones; on an escape spend it on speed. (pp35-36)"))

    if purpose == "approach" or purpose == "escape":
        out.append((3, "Use EM when you do not need to fire and your enemy does — usually an "
                       "approach or an escape. Fighters traditionally use EM to close to "
                       "point-blank for a ship assault. (p35)"))

    out.append((3, "EM restrictions: no " + ", ".join(EM_PROHIBITS[:6]) + ", and more. " +
                EM_PENALTIES))
    out.append((3, "EM may be started and stopped only ONCE per turn, and takes effect the "
                   "impulse AFTER announcement. Turn-break workaround: drop EM on impulse #31, "
                   "fire and announce on #32, resume on #1. (p35)"))
    out.append((4, "EM is fairly useful to fragile units (frigates, PFs, fighters) but of little "
                   "use for cruisers. Bases and FRDs cannot use EM. It CAN be used at speed zero "
                   "in a defensive stand. (p35)"))
    return out


EM_ISC_FALLACY = ("The common belief that EM lets you penetrate an ISC gunline into the PPD's "
                  "no-fire zone is a FALLACY. An EM ship is very vulnerable to seeking weapons, "
                  "something ISC echelons have in quantity. (p35)")

# --------------------------------------------------------------------------
# Emergency Deceleration (p44)
# --------------------------------------------------------------------------
EMER_DECEL = {
    "effect": "Leaves you at SPEED ZERO for 16 impulses, handing the enemy initiative, range "
              "control, the ability to reload, and even the ability to disengage unpursued. "
              "'Once your last weasel is burned and the explosion period ends, you are a sitting "
              "duck for the Gorn Anchor.' (p44)",
    "legitimate_use": "Most common legitimate use: to launch a WILD WEASEL (it also reinforces "
                      "the facing shields). Have at least TWO weasels ready in case the first is "
                      "phasered. (p44)",
    "cannot_save_power": "It CANNOT save power — you must have allocated a full turn of legal "
                         "movement, and on declaration the power goes into the shields and "
                         "nowhere else. It cannot be pre-plotted. (p44)",
    "other_uses": "Substituting TACs to effectively lower your turn mode; delaying an encounter "
                  "when the enemy closes faster than expected; escaping suddenly-appearing "
                  "terrain; ISC use to slow closure and fire remaining PPD pulses before entering "
                  "the myopic zone; targets of a Mizia attack use it to pour power into forward "
                  "shields. (p44)",
    "recovery": "Recovery plan: accelerate to speed 10 if possible, at minimum speed 6 — speed "
                "6-10 allows you to reach disengagement speed in two more turns. (p44)",
    "usually_wait": "Usually it is faster to just wait for the turn break and plot eight impulses "
                    "of zero speed than to use decel as a prelude to reversing. (p44)",
}


def maneuver_advice(ship, enemy_has_seekers=False, impulse=1, considering=None,
                    breakdown_rating=None, bonus_used=False, movement_cost=None,
                    nimble=False, warp_tacs_used=0, sublight_used=False):
    """Top-level entry: what should this ship do about changing facing?

    considering: 'het' | 'em' | 'tac' | None (None = survey the options)
    Returns (priority, text) notes sorted most urgent first.
    """
    out = []
    if considering == "het" or considering is None:
        out += het_assessment(ship, breakdown_rating=breakdown_rating, bonus_used=bonus_used,
                              impulse=impulse)
    if considering == "tac" or considering is None:
        out += tac_assessment(impulse, warp_tacs_used, sublight_used)
    if considering == "em" or considering is None:
        out += em_assessment(ship, enemy_has_seekers=enemy_has_seekers,
                             movement_cost=movement_cost, nimble=nimble)
    out.sort(key=lambda r: r[0])
    return out


if __name__ == "__main__":
    print("--- Lyran CA considering a HET at impulse 14, facing Kzinti drones ---")
    for pri, txt in maneuver_advice({"label": "Kharg"}, enemy_has_seekers=True, impulse=14,
                                    breakdown_rating=5, movement_cost=1.0)[:8]:
        print(f"[{pri}] {txt[:120]}")
