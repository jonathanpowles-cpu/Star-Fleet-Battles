"""
Shuttle warfare: scatter-packs, suicide shuttles, wild weasels.

Distinct from fighters (sfb_carrier). These are the *expendable* shuttle uses,
and they matter enormously for the Kzinti - the designers' own doctrine note
(FD1.63) names scatter-packs as the way to beat drone defences:

    "To consistently score damage with a drone attack requires concentration...
     More drones must arrive during a given period of time than the target's
     defenses can deal with. Scatter-packs can be used to increase the effective
     rate of fire."

Each option has a different arming clock, and that is what decides when you must
commit power - all three compete for the same shuttle boxes.

    Scatter-pack   FD7.111  NO energy to launch; loaded with drones in advance
    Suicide shuttle J2.2211 THREE turns arming, 1-3 pts/turn, warhead = 2x total
    Wild weasel    J3.12    ONE point on each of TWO consecutive turns
"""
from __future__ import annotations

# ---------------------------------------------------------------- scatter-pack
SP_CAPACITY_ADMIN = 6        # FD7.21: an admin shuttle SP carries 6 "spaces" of drones
SP_CAPACITY_MRS = 8          # FD7.21: some MRS shuttles carry 8
SP_ENERGY = 0                # FD7.111: unmanned, costs NO energy to launch
SP_PROHIBITED = ("type-III (FD5.253)", "multi-warhead (FD8.31)",
                 "Starfish (FD15.253)", "Stingray (FD16.253)")

# ------------------------------------------------------------- suicide shuttle
SS_ARM_TURNS = 3             # J2.2211
SS_MIN_PER_TURN, SS_MAX_PER_TURN = 1.0, 3.0
SS_WARHEAD_PER_POINT = 2     # warhead = 2 x total energy applied
SS_WARHEAD_MIN, SS_WARHEAD_MAX = 6, 18
SS_HOLD_COST = 1             # J2.2212: 1/turn from the 4th turn onward

# ------------------------------------------------------------------ wild weasel
WW_ARM_TURNS = 2             # J3.12: one point on each of two consecutive turns
WW_HOLD_COST = 1             # J3.121
WW_MIN_IMPULSES = 32         # J3.122


def suicide_warhead(total_energy):
    """Warhead strength for the energy applied (J2.2211)."""
    return max(SS_WARHEAD_MIN, min(SS_WARHEAD_MAX,
                                   int(SS_WARHEAD_PER_POINT * total_energy)))


# EAF rows that are SOURCES or computed/display, not power CONSUMED. Everything
# else in a row is real spend, so summing the rest gives what was actually used.
_EAF_NON_SPEND = {
    "Warp Power", "Impulse", "APR", "AWR", "Total Power", "Total Power Used",
    "Total Power Available", "Battery Power", "Battery Discharged",
    "Start Phaser Capacitor", "Phaser Capacitor Added", "Phaser Capacitor Used",
    "End Phaser Capacitor", "Reserve Power Added", "Reserve Power Used",
    "Reserve Power Available", "Speed Plot (Start)", "Speed Plot (End)", "Notes",
}


def _spare_from_eaf(ship):
    """Unused power from the ACTUAL client EAF (available - allocated), or None.

    Exact where the estimate was not: the Feral OVERLOADS its disruptors and is
    fully committed, so its real spare is ~0 - but the old estimate (assuming
    standard arming) wrongly found 5.5 and offered a suicide shuttle it cannot
    fund without cutting something else.
    """
    turns = [t for t in (ship.get("eaf") or []) if t]
    if not turns:
        return None
    row = turns[-1]

    def num(k):
        try:
            return float(row.get(k, 0) or 0)
        except (TypeError, ValueError):
            return 0.0
    available = num("Warp Power") + num("Impulse") + num("APR") + num("AWR")
    used = sum(num(k) for k in row if k not in _EAF_NON_SPEND)
    return max(0.0, round(available - used, 2))


def _spare_power(ship):
    """Power left after everything the EAF actually spends. Prefers the real
    client allocation; falls back to a rough estimate only without EAF data."""
    live = _spare_from_eaf(ship)
    if live is not None:
        return live
    pwr = (ship.get("power") or {}).get("total", 0)
    sc = ship.get("size_class") or 3
    hk = {1: 6, 2: 5, 3: 4, 4: 2.5}.get(sc, 4)          # D3.32 + B3.3 + fire control
    w = ship.get("weapons") or {}
    arm = (w.get("disruptor", [0, 0])[0] * 2 + w.get("photon", [0, 0])[0] * 2
           + w.get("esg", [0, 0])[0] * 3 + w.get("hellbore", [0, 0])[0] * 3)
    try:
        mc = float(ship.get("move_cost") or 1.0)
    except (TypeError, ValueError):
        mc = 1.0
    move = int(ship.get("speed") or 0) * mc
    return max(0.0, round(pwr - hk - arm - move, 1))


def weasel_standing_note(ship):
    """A permanent hull fact about wild weasels, or None.

    Kept OUT of the per-impulse order stream: an ESG ship can never use a weasel
    (G23.48), so repeating it every impulse is noise that buries the advice which
    actually changes turn to turn. The UI shows this once, as ship data.
    """
    esg = (ship.get("weapons") or {}).get("esg", [0, 0])[0]
    if not esg:
        return None
    return (f"No wild weasel on this hull: G23.48 - raising an ESG VOIDS it, and "
            f"with {esg} ESG generator(s) the ESG is the renewable anti-drone "
            f"answer. T-bombs are the emergency alternative (Kosnett, p113).")


def shuttle_actions(ship, enemies, tgt, rng, impulse, turn, log, cmd):
    """What to do with this ship's shuttle boxes, given the tactical picture."""
    out = []
    sysd = ship.get("systems") or {}
    shuttles = sysd.get("shuttle", [0, 0])[0]
    if not shuttles or not enemies:
        return out

    w = ship.get("weapons") or {}
    esg = w.get("esg", [0, 0])[0]
    drones = w.get("drone", [0, 0])[0]
    enemy_seekers = sum((e.get("weapons") or {}).get("drone", [0, 0])[0] for e in enemies)

    # ---- SCATTER-PACK: a THREE-TURN commitment, not an impulse action.
    # FD7.22/FD7.26: the drones must be loaded by deck crews at normal rates -
    # six spaces is six crew actions, which is three turns with two crews. Saying
    # "load one" as if it were instant would promise a weapon that cannot exist
    # for three turns.
    if drones:
        try:
            import sfb_deckcrew as DC
            adv = DC.scatterpack_advice(ship, shuttles, rng, turn, log)
            if adv:
                head, why = adv
                why = why + [
                    "FD1.63 (designers' doctrine): 'more drones must arrive during a given "
                    "period than the target's defenses can deal with. Scatter-packs can be "
                    "used to increase the effective rate of fire'",
                    f"FD7.111: unmanned and costs NO ENERGY to launch - the cost is crew "
                    f"time, not power",
                    f"cannot carry: {', '.join(SP_PROHIBITED)}",
                ]
                out.append((head, why))
        except Exception:
            pass

    # ---- SUICIDE SHUTTLE: decide, do not offer.
    # 3 turns x up to 3 pts = 9 points for an 18-point warhead (J2.2211). That is
    # excellent damage per point, but only if the ship can actually spare it for
    # three consecutive turns - miss one and the whole investment is lost.
    spare = _spare_power(ship)
    if rng > 8:
        if spare >= SS_MAX_PER_TURN:
            out.append((f"ARM SUICIDE SHUTTLE - {SS_MAX_PER_TURN:g} points this turn",
                        [f"~{spare:g} spare after allocation, so the "
                         f"{SS_MAX_PER_TURN:g}/turn is affordable",
                         f"J2.2211: {SS_ARM_TURNS} turns at {SS_MAX_PER_TURN:g}/turn = 9 "
                         f"points = the {SS_WARHEAD_MAX}-point maximum warhead",
                         "WARN miss a turn's energy and arming ABORTS - all spent energy is "
                         "lost (J2.2211). Commit for three turns or not at all",
                         f"J2.2212: holding past the third turn costs {SS_HOLD_COST}/turn"]))
        elif spare >= SS_MIN_PER_TURN:
            warhead = suicide_warhead(spare * SS_ARM_TURNS)
            out.append((f"ARM SUICIDE SHUTTLE - {spare:g} point(s)/turn "
                        f"(warhead ~{warhead})",
                        [f"only ~{spare:g} spare, so this buys a {warhead}-point warhead "
                         f"rather than the {SS_WARHEAD_MAX}-point maximum",
                         "WARN arming ABORTS if a turn's energy is missed (J2.2211)"]))
        else:
            out.append(("NO suicide shuttle - cannot spare the power",
                        [f"J2.2211 needs {SS_MIN_PER_TURN:g}-{SS_MAX_PER_TURN:g} points a "
                         f"turn for {SS_ARM_TURNS} consecutive turns; this ship has ~"
                         f"{spare:g} spare and is already trimming speed to fit",
                         "arming and then missing a turn loses everything spent"]))

    # ---- WILD WEASEL: only when it is not voided by our own ESG.
    #
    # An ESG ship can NEVER use a weasel (G23.48), so for a Lyran this is a
    # standing fact about the hull, not a decision to be re-made every turn.
    # Repeating "WILD WEASEL: NO" in the orders each impulse was pure noise and
    # crowded out advice that does change. It is stated once, as a static note
    # the UI can file away, rather than issued as a recurring order.
    if enemy_seekers:
        if esg:
            pass        # see weasel_standing_note() - not an per-impulse order
        else:
            out.append((f"WILD WEASEL: charge one ({enemy_seekers} enemy drone rack(s))",
                        [f"J3.12: 1 point on each of {WW_ARM_TURNS} consecutive turns, then "
                         f"{WW_HOLD_COST}/turn to hold (J3.121)",
                         f"J3.122: cannot launch within {WW_MIN_IMPULSES} impulses of "
                         f"starting - a weasel begun now is a NEXT-turn asset",
                         "J3.202: protects ONLY the launching ship; J3.116: only one active "
                         "at a time; J3.24: held in the bay it does nothing"]))
    return out
