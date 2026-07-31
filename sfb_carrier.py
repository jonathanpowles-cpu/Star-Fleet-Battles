"""
Carrier and PF operations: launch timing, recovery, and the restrictions that
make a badly-timed launch worthless.

Every rule number here comes from Module J (fighters) and the Master Rulebook's
K-series (PFs), extracted into kb_fighters_pfs.md. The single most actionable
finding, and the reason this module exists:

    A fighter launched inside 8 impulses of contact is a PASSENGER.
    Launch must lead contact by >= 8 impulses to use guns (J1.342),
    and >= 16 impulses to use drones (J1.341).

That is a timing decision the engine can make for you, and getting it wrong
wastes the whole strike.
"""
from __future__ import annotations

# --------------------------------------------------------------------------
# Post-launch restrictions (J1.34) - a just-launched fighter cannot fight yet.
# --------------------------------------------------------------------------
LAUNCH_LOCKOUT = {
    "seeking":   (16, "J1.341", "cannot launch or guide seeking weapons"),
    "direct":    (8,  "J1.342", "cannot fire direct-fire weapons"),
    "ew_mines":  (8,  "J1.343", "cannot lend EW, lay/sweep mines, or gather information"),
}
SCATTERPACK_SEEKER_DELAY = 8      # J1.341 exception: scatter-packs release after 1/4 turn
LAUNCH_LAND_GAP = 8               # J1.52: min 1/4-turn between launching and landing
IMPULSES_PER_TURN = 32

# --------------------------------------------------------------------------
# Bay throughput (J1.50): one shuttle per bay per TWO impulses, launch XOR
# recover - a bay cannot do both within any two consecutive impulses.
# --------------------------------------------------------------------------
BAY_IMPULSES_PER_OP = 2

# Special systems that beat the baseline (J1.53-J1.58).
BAY_SYSTEMS = {
    "balcony": dict(cite="J1.53/J1.532",
                    note="Bay->balcony still limited by J1.50, but ANY NUMBER may launch "
                         "from or land on the balcony in a single impulse - this is how "
                         "strike groups go up fast.",
                    cost="J1.531: while on the balcony, each REAR HULL damage point "
                         "destroys one shuttle instead of one hull box. Balcony shuttles "
                         "cannot fire, rearm, repair, or be prepared as WW/SS/SP, and are "
                         "destroyed if the ship disengages by acceleration (J1.533)."),
    "tubes":   dict(cite="J1.54/J1.541/J1.542",
                    note="Launch several AND recover one per bay simultaneously; each tube "
                         "once per two impulses.",
                    cost="Tubes CANNOT recover. No admin shuttles and no heavy fighters (J10.2)."),
    "tunnel":  dict(cite="J1.58",
                    note="Doors at both ends; each hatch operates independently at the full "
                         "J1.50 rate - DOUBLE throughput.",
                    cost=""),
    "external": dict(cite="J1.55",
                     note="Tholian external bays launch/recover ALL fighters simultaneously; "
                          "cannot be overcrowded.",
                     cost="Still no launch+recover from the same bay within two impulses."),
}

# Hulls known to carry the special systems (from the Module J tables).
TUNNEL_DECK_HULLS = {"CV", "CVS", "CVL", "MCV", "CVE"}     # Fed CVS; Kzinti CV/CVS/CVL/MCV/CVE
TUNNEL_DECK_RACES = {"KZINTI", "FEDERATION"}

# --------------------------------------------------------------------------
# The carrier's own risk (D12.0)
# --------------------------------------------------------------------------
CHAIN_REACTION = (
    "D12.0: a flight deck crowded with armed fighters, fuel and ammunition can CHAIN "
    "REACT - loaded fighters parked wingtip-to-wingtip detonate each other. If NO armed "
    "fighters are aboard, D12.0 is ignored entirely: once the group is up, the carrier "
    "is materially safer than it was with them aboard."
)

# --------------------------------------------------------------------------
# PFs (K-series)
# --------------------------------------------------------------------------
PF_FACTS = {
    "turn_mode":  "AA at every speed - the most nimble class in the game (K1.21)",
    "move_cost":  "1/5 point per hex; interceptors 1/6 (K1.21)",
    "accel":      "triple current speed, or by fifteen, whichever is greater (K1.22)",
    "het":        "ONE per scenario with no breakdown chance; break down on a 6 thereafter. "
                  "PFs do NOT get the C11.22 nimble HET benefit (K1.23)",
    "shields":    "one point of energy keeps ALL shields at full strength; 1/2 for minimum (K1.41)",
    "flotilla":   "six PFs nominally - one leader, one scout, four combat (K0.30); may run "
                  "three to eight (K0.33)",
    "carried":    "PFs carry no shuttles, tractors or transporters - except the one PF "
                  "Leader per flotilla, which has all three (K0.121)",
}

# Lyrans have no fighters of their own (R11.0 / R11.F1) - they flew Klingon
# designs unmodified under licence. Same for the LDR and Seltorians.
NO_NATIVE_FIGHTERS = {"LYRAN", "LDR", "SELTORIAN"}
BORROWS_FROM = {"LYRAN": "Klingon", "LDR": "Klingon", "SELTORIAN": "Klingon"}


# --------------------------------------------------------------------------
# Fighter maximum speeds, from the Master Fighter Chart (Module G3).
#
# This matters far more than it looks. (J1.61): landing aboard under its own
# power requires "the ship is not moving faster than the shuttle". So a carrier
# that launches and then runs at speed 25 has not merely left its fighters
# behind - it has made recovery IMPOSSIBLE until it slows down.
# --------------------------------------------------------------------------
FIGHTER_SPEED = {
    ("KZINTI", "AAS"):   8,
    ("KZINTI", "AAS-E"): 8,
    ("KZINTI", "HAAS"):  11,
    ("KZINTI", "TAAS"):  12,
}
FIGHTER_SPEED_ASSUMED = 8       # first-generation fighters; flagged when used
FIGHTER_SPEED_ACE_MAX = 31      # J6.23: only an ace gets a speed-15 fighter to 31


def fighter_speed(race, ftype=None):
    """(speed, known). `known` False means this is the conservative assumption,
    not a charted value - say so rather than presenting it as fact."""
    r = (race or "").upper()
    if ftype:
        v = FIGHTER_SPEED.get((r, ftype.upper()))
        if v:
            return v, True
    # Fall back to the slowest charted fighter for that race, which is the safe
    # assumption for a mixed or unknown group.
    speeds = [v for (rr, _t), v in FIGHTER_SPEED.items() if rr == r]
    if speeds:
        return min(speeds), True
    return FIGHTER_SPEED_ASSUMED, False


def carrier_speed_limit(carrier, fighters_out, ftype=None, state=None):
    """The speed a carrier may run at without abandoning its group.

    `fighters_out` must mean FIGHTERS ACTUALLY AIRBORNE. Callers used to pass
    bool(fighter boxes), which is true whenever the ship has surviving boxes -
    so a carrier was capped at fighter speed permanently, group in the bay or
    not, and its escorts were told to match it. That single inversion was
    costing the whole squadron its approach speed.

    When `state` is supplied the limit comes from the SLOWEST airframe actually
    on the board (J1.61), which a per-race constant cannot express: a group of
    speed-8 AAS with one speed-6 BMR in it recovers at 6.

    Returns (limit, note) or (None, note) when unconstrained.
    """
    if state is not None:
        try:
            import sfb_airborne as AB
            spd, note = AB.recovery_speed(state, carrier)
            if spd is not None:
                return spd, note
            return None, ""
        except Exception:
            pass
    if not fighters_out:
        return None, ""
    spd, known = fighter_speed(carrier.get("race"), ftype)
    qual = "" if known else " (assumed - fighter type not in the save)"
    return spd, (f"J1.61: fighters cannot land while the ship moves faster than they do. "
                 f"With the group out, speed must not exceed {spd}{qual} or they are "
                 f"stranded - not just trailing, but unable to recover at all.")


def bay_system_for(race, ship_type):
    """Which special bay system this hull has, if any."""
    t = (ship_type or "").upper().replace("-", "").strip()
    if t in TUNNEL_DECK_HULLS and (race or "").upper() in TUNNEL_DECK_RACES:
        return "tunnel"
    return None


def launch_window(bays, group_size, system=None):
    """Impulses needed to put `group_size` fighters up through `bays` bays."""
    bays = max(1, bays)
    per_op = BAY_IMPULSES_PER_OP
    if system == "tunnel":
        bays *= 2                    # each hatch runs at the full rate (J1.58)
    if system in ("balcony", "external"):
        return 2                     # effectively one impulse plus the bay->balcony feed
    ops = (group_size + bays - 1) // bays
    return ops * per_op


def launch_advice(carrier, rng_to_enemy, closing_speed, impulse, group_size=None,
                  bays=1, has_armed_fighters=True):
    """When should this carrier launch? Returns (priority, text) notes.

    The decision is a race between two clocks: how long the group takes to get
    off the deck, and how long until contact. Launch too late and the fighters
    are passengers under J1.341/J1.342.
    """
    out = []
    race = (carrier.get("race") or "").upper()
    ctype = carrier.get("type") or ""
    system = bay_system_for(race, ctype)
    group = group_size if group_size is not None else 6

    # How many impulses until the FIGHTERS are in the fight? It is their speed
    # that decides, not the carrier's - the carrier is not going with them.
    fspd, fknown = fighter_speed(race)
    enemy_closing = max(0, closing_speed - int(carrier.get("speed") or 0))
    closing = max(1, fspd + enemy_closing)
    imp_to_contact = int(round((rng_to_enemy / closing) * IMPULSES_PER_TURN))
    out.append((3, f"fighter speed {fspd}{'' if fknown else ' (assumed)'}; with the enemy "
                   f"closing at {enemy_closing} the strike arrives in ~{imp_to_contact} "
                   f"impulses"))

    # The carrier's own ordered speed decides whether it keeps its group.
    carrier_spd = int(carrier.get("speed") or 0)
    if carrier_spd > fspd:
        out.append((1, f"WARN SPEED CONFLICT: the ship is at speed {carrier_spd} but the "
                       f"fighters make {fspd}. Launch now and they fall {carrier_spd - fspd} "
                       f"hexes behind every turn, and under J1.61 they CANNOT LAND while you "
                       f"outrun them. Either slow to {fspd} to fight with them, hold them "
                       f"aboard, or tractor-drag them (J1.62)."))

    deck = launch_window(bays, group, system)
    need_guns = LAUNCH_LOCKOUT["direct"][0]
    need_drones = LAUNCH_LOCKOUT["seeking"][0]

    out.append((3, f"deck: {bays} bay(s)"
                   + (f" + {system} ({BAY_SYSTEMS[system]['cite']})" if system else "")
                   + f"; ~{deck} impulses to put {group} fighters up"
                   + (f" - {BAY_SYSTEMS[system]['note']}" if system else "")))

    lead_guns = deck + need_guns
    lead_drones = deck + need_drones
    out.append((3, f"contact in ~{imp_to_contact} impulses at range {rng_to_enemy}, "
                   f"closing {closing}/turn"))

    if imp_to_contact >= lead_drones:
        out.append((1, f"LAUNCH NOW - {imp_to_contact} impulses to contact clears the "
                       f"{lead_drones} needed ({deck} on the deck + {need_drones} lockout), "
                       f"so the group arrives able to use DRONES as well as guns (J1.341)."))
    elif imp_to_contact >= lead_guns:
        out.append((1, f"LAUNCH NOW for a GUN strike - {imp_to_contact} impulses clears the "
                       f"{lead_guns} needed, but NOT the {lead_drones} for drones. Fighters "
                       f"will arrive with direct-fire only (J1.341/J1.342)."))
    else:
        short = lead_guns - imp_to_contact
        out.append((1, f"WARN TOO LATE to launch usefully - contact in {imp_to_contact} "
                       f"impulses but a fighter needs {lead_guns} ({deck} deck + "
                       f"{need_guns} lockout). Launching now puts PASSENGERS in the fight, "
                       f"{short} impulses short. Either hold them aboard or open the range."))

    if has_armed_fighters:
        out.append((2, "WARN " + CHAIN_REACTION))
    else:
        out.append((3, "No armed fighters aboard - D12.0 chain reaction does not apply."))

    if system == "balcony":
        out.append((2, "WARN " + BAY_SYSTEMS["balcony"]["cost"]))

    if race in NO_NATIVE_FIGHTERS:
        out.append((3, f"{race.title()} have no fighters of their own (R11.0/R11.F1) - they "
                       f"fly {BORROWS_FROM[race]} designs unmodified; resolve loadouts from "
                       f"the {BORROWS_FROM[race]} table."))
    return out


def launch_schedule(carrier, group_size, bays=1, start_impulse=1, ftype=None):
    """A concrete impulse-by-impulse launch order, not just 'launch now'.

    Each fighter's lockout runs from ITS OWN launch impulse (J1.34), so the
    group does not become combat-capable all at once - the last one off the
    deck is the one that decides when the strike is really ready.
    """
    system = bay_system_for(carrier.get("race"), carrier.get("type"))
    per_impulse = (2 if system == "tunnel" else 1) * max(1, bays)
    if system in ("balcony", "external"):
        per_impulse = group_size
    spd, known = fighter_speed(carrier.get("race"), ftype)

    rows, imp, launched = [], start_impulse, 0
    while launched < group_size:
        n = min(per_impulse, group_size - launched)
        rows.append((imp, n, launched + 1, launched + n))
        launched += n
        imp += BAY_IMPULSES_PER_OP if per_impulse <= (2 if system == "tunnel" else 1) * bays else 1
    last_out = rows[-1][0] if rows else start_impulse

    out = [
        f"LAUNCH ORDER - {group_size} fighters, "
        + (f"{system} decks ({BAY_SYSTEMS[system]['cite']}), " if system else "")
        + f"{per_impulse} per launch cycle, one cycle every {BAY_IMPULSES_PER_OP} impulses "
          f"(J1.50). Fighter speed {spd}{'' if known else ' (assumed)'}.",
    ]
    for impn, n, a, b in rows:
        out.append(f"  imp {impn:>2}: launch {n} (#{a}" + (f"-#{b}" if b > a else "") + ")")
    out += [
        f"  imp {last_out:>2}: deck clear - D12.0 chain-reaction risk GONE once no armed "
        f"fighters remain aboard",
        f"  imp {last_out + LAUNCH_LOCKOUT['direct'][0]:>2}: whole group may fire "
        f"DIRECT-FIRE weapons (J1.342, 8 imp after the LAST launch)",
        f"  imp {last_out + LAUNCH_LOCKOUT['seeking'][0]:>2}: whole group may launch or "
        f"guide DRONES (J1.341, 16 imp after the LAST launch)",
        f"  NOTE each fighter's lockout runs from its OWN launch impulse - #1 can shoot "
        f"{LAUNCH_LOCKOUT['direct'][0]} impulses after it leaves, not after the last one does.",
        f"  RECOVERY: slow to {spd} or less to land them (J1.61); a bay cannot launch and "
        f"recover within two consecutive impulses (J1.50).",
    ]
    return out


def recovery_advice(carrier, threat_range, impulse):
    """Recovering fighters mid-battle - when it is safe and what it costs."""
    out = []
    out.append((2, f"recovery runs at the same J1.50 rate - one per bay per two impulses, "
                   f"and a bay CANNOT launch and recover within two consecutive impulses."))
    out.append((3, f"J1.52: a shuttle may launch once and land once per turn, either order, "
                   f"with a minimum {LAUNCH_LAND_GAP}-impulse gap between the two."))
    if threat_range is not None and threat_range <= 8:
        out.append((1, f"WARN enemy at range {threat_range} - recovering now re-arms the "
                       f"D12.0 chain-reaction risk just as you are being shot at. Recover "
                       f"outside knife range or accept the risk deliberately."))
    out.append((3, "J1.62 tractor recovery is the alternative: needs a powered, otherwise "
                   "unused tractor and an empty shuttle box; while held, the shuttle cannot "
                   "fire, launch or guide, but keeps its own EW."))
    return out


def pf_notes(ship):
    """PF-specific handling facts an AI needs when flying one."""
    return [(3, f"PF {k}: {v}") for k, v in PF_FACTS.items()]
