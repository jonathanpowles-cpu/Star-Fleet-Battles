"""
SFB fleet command engine - the "human as hands" model.

You (the human) control every ship in the client. This engine reads the full
battle state and, for each side, tells you what to do:

  * LYRAN ships -> ADVICE  (suggestions; take them or not)
  * KZINTI ships -> ORDERS (the AI is playing this side; execute them faithfully)

State comes from the client's live tactical autosave (game#SFB_Game1), read via the
StateDump Java helper, giving full shields-per-facing, weapons and damage. Output is
posted to the game chat (proven to apply live) and/or printed.

    SFB_PASSWORD=x python sfb_command.py --ai Kzinti --advise Lyran [--post]

This is deterministic doctrine over a real engine; an optional LLM pass voices it.
"""
from __future__ import annotations
import os, sys, json, subprocess, argparse, time, re
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import sfb_hex as H
import sfb_rules as R
import sfb_log
import sfb_situations as SIT

TURN_CAT = {"AA": 0, "A": 1, "B": 2, "C": 3, "D": 4, "E": 5, "F": 6}


def turn_category(ship):
    """Turn Mode CATEGORY index for a ship (C3.3), read from its SSD attribute.

    Returns (index, ok). ok is False when the save's value was unrecognised and
    we fell back to C — the caller should say so rather than quietly present a
    guessed turn mode as fact. Categories are letters AA-F; a Federation CA is
    category D (C3.23).
    """
    raw = ship.get("turn_mode")
    if raw is None:
        return 3, False
    key = str(raw).strip().upper()
    if key in TURN_CAT:
        return TURN_CAT[key], True
    # Some saves store the category as a bare index rather than a letter.
    if key.isdigit() and 0 <= int(key) <= 6:
        return int(key), True
    return 3, False


def turn_mode_of(ship):
    """(turn_mode_hexes, category_letter, ok) for this ship at its current speed."""
    cat, ok = turn_category(ship)
    letter = [k for k, v in TURN_CAT.items() if v == cat]
    tm = R.turn_mode(int(ship.get("speed") or 0), cat)
    return tm, (letter[0] if letter else "?"), ok
# size class by hull type (life-support cost keys off this)
SIZE3 = {"DN", "BC", "BCH", "CA", "CC", "CV", "CVL", "CVE", "CVD", "CS", "SCS", "TGC", "TGT", "BATS"}


def size_class(t):
    base = "".join(ch for ch in t if ch.isalpha()).upper()
    return 3 if base in SIZE3 else 4


ECM_USEFUL_CAP = 4      # EW saturates; past ~4 points the shift stops paying


def _pt(v):
    """Format an energy figure without fake precision: 5.33 not 5.333333."""
    return f"{v:g}" if float(v).is_integer() else f"{v:.2f}".rstrip("0")


def _battery_discharge(ship):
    """Battery energy spent, read from the client EAF (was a hand-kept table).

    The client stores 'Reserve Power Used' per turn in the ship's EAF; that IS
    the battery draw. Reading it live means the recharge advice updates itself
    every turn instead of being maintained by hand.
    """
    try:
        import sfb_actions as ACT
        return ACT.battery_discharge_from_eaf(ship)
    except Exception:
        return 0.0


def compute_eaf(ship, want_speed, closing, threatened, fired_phasers=False,
                enemies=None, rng=None):
    """Doctrine energy allocation for one ship, given its real power budget."""
    pwr = ship["power"]["total"]
    batt = ship["power"]["battery"]
    sc = ship.get("size_class") or size_class(ship["type"])   # real value, fallback to guess
    w = ship.get("weapons", {})
    disr = w.get("disruptor", [0, 0])[0]
    phot = w.get("photon", [0, 0])[0]
    plas = w.get("plasma", [0, 0])[0]
    esg = w.get("esg", [0, 0])[0]
    has_ph = w.get("phaser", [0, 0])[0] > 0

    # Housekeeping (mandatory), all authentic + cited:
    #   shield operation cost by size class (D3.32): SC3 cruiser = 2 (measured)
    #   life support by size class (B3.3): SC3 = 1
    #   active fire control (D6.6) = 1  [low-power fire control (D6.7) = 0.5]
    shield_hk = R.SHIELD_COST.get(sc, 2)
    ls = R.LIFE_SUPPORT_COST.get(sc, 1)
    afc = R.FIRE_CONTROL_COST
    hk = shield_hk + ls + afc
    def fmt(n):
        return str(int(n)) if float(n).is_integer() else str(n)
    hk = int(hk) if float(hk).is_integer() else hk
    hk_detail = f"{fmt(shield_hk)}sh+{fmt(ls)}LS+{fmt(afc)}AFC"
    nph = w.get("phaser", [0, 0])[0]        # phasers fire at 1 pt each from capacitor/reserve
    try:
        mc = float(ship.get("move_cost") or 1.0)        # authentic per-point movement cost (C2)
    except (TypeError, ValueError):
        mc = 1.0
    spd_want = int(want_speed)
    # Energy for movement = speed x move-cost. Do NOT round: a 1/3-cost frigate
    # wanting 16 hexes needs 5.33 points, and rounding that to 5 both undercharges
    # the budget and reports a speed those 5 points cannot buy. Fractional
    # accounting (B3.2) allows the thirds and halves these costs produce.
    move = spd_want * mc
    arm = 0
    notes = []
    # per-weapon arming energy (SFB): standard / overloaded
    ARM_STD = {"disruptor": 2, "photon": 2, "fusion": 2, "hellbore": 3, "esg": 3, "plasma": 0}
    ARM_OVL = {"disruptor": 4, "photon": 8, "fusion": 4, "hellbore": 6}
    if disr:
        each = ARM_OVL["disruptor"] if closing else ARM_STD["disruptor"]
        cost = disr * each
        arm += cost
        notes.append(f"{disr} disr {'OVL' if closing else 'std'}={cost}")
    if phot:
        cost = phot * ARM_STD["photon"]
        arm += cost
        notes.append(f"{phot} phot std={cost}")
    if plas:
        notes.append(f"{plas} plasma arming (multi-turn)")
    if esg:
        cost = esg * ARM_STD["esg"]
        arm += cost
        notes.append(f"ESG x{esg}={cost}")
    # ELECTRONIC WARFARE. This was a flat "2 if threatened", which is wrong twice
    # over: it never looked at what the enemy is actually running, and 2 is not a
    # threshold value - EW pays only at perfect squares (D6), so 2 buys the same
    # +1 shift as 1 and the second point is simply burnt, every turn.
    #
    # recommend_ew sizes ECCM to cancel his ECM first (1-for-1, BEFORE the
    # square-law, so it is far cheaper than out-jamming) and then spends what is
    # left on ECM at the next threshold down.
    ew_note = ""
    try:
        import sfb_ew as EW
        _ecm, _eccm, _why = EW.recommend_ew(ship, enemies or [])
        ew = _ecm + _eccm
        if ew:
            bits = []
            if _eccm:
                bits.append(f"ECCM {_pt(_eccm)}")
            if _ecm:
                bits.append(f"ECM {_pt(_ecm)}")
            ew_note = " + ".join(bits)
    except Exception:
        ew = 2 if threatened else 0
    fixed = hk + arm + ew                     # mandatory + weapon arming (not movement/phasers)
    spent = fixed + move
    reserve = pwr - spent
    if reserve < 0:                          # over-budget: trim SPEED (integer) to fit
        avail_move = max(0, pwr - fixed)
        # Speed is whole hexes, so take the most the remaining points actually buy
        # and charge exactly that - keeping move and spd consistent with each other.
        spd_want = int(avail_move / mc) if mc else 0
        move = spd_want * mc
        spent = fixed + move
        reserve = pwr - spent
        notes.append("(speed trimmed to fit power)")
    seg = [f"hk {hk} ({hk_detail})",
           f"move {_pt(move)} (spd {spd_want} @ {_pt(mc)}/hex)"]
    if notes:
        seg.append("arm " + ", ".join(notes))
    if ew:
        # Name ECM and ECCM separately: they are separate lines on the client's
        # own EA form, so a lump "ECM 4" cannot be transcribed into it.
        seg.append(ew_note or f"ECM {_pt(ew)}")
    # ---- Phaser capacitor (H6.0). This is NOT a per-turn cost.
    #
    # H6.1: energy held in the capacitor carries from one turn to the next, free.
    # H6.22/E2.22: "If the capacitors are still full from the previous turn, no
    # power can be allocated to phasers" - so budgeting to 'hold' a full
    # capacitor is not merely wasteful, it is illegal. You allocate only to
    # REFILL what you actually fired.
    #
    # H6.21: capacity is the total to fire each phaser once - ph-1/2/4 cost 1,
    # ph-3 costs 0.5. (The rulebook's own example: a Kzinti CV with 5 ph-1s and
    # 11 ph-3s has a capacitor of 5x1 + 11x0.5 = 10.5, rounded to 11.)
    cap = phaser_capacitor(ship)
    # ---- Surplus power. There is NO "reserve" line on the Energy Allocation form.
    #
    # H7.0 is explicit: "Ships may use their BATTERIES as a source of reserve
    # power. Note specifically that unallocated power from engines or reactors is
    # NOT treated as reserve power; it was simply never produced (B3.4)."
    # H7.113 caps total reserve at battery capacity.
    #
    # So surplus cannot be "held". It goes to a real EA line (ECM, reinforcement,
    # damage control, recharging DISCHARGED batteries) or it is never generated.
    # Batteries that are already full cannot absorb anything.
    # batt_room = capacity - current charge. The save exposes only battery box
    # COUNT, not charge, so charge comes from BATTERY_DISCHARGE - values the
    # player reports (the FF spent 0.87 and the CW ~2 on ESG boosts). A ship not
    # listed is assumed full (batt_room 0), correct at scenario start.
    batt_room = _battery_discharge(ship)
    to_batt = min(max(0.0, reserve), batt_room)
    unspent = max(0.0, reserve) - to_batt
    bits = []
    if batt and batt_room <= 0:
        bits.append(f"batteries {batt} (full - reserve power comes from these, "
                    f"H7.0; nothing to allocate)")
    elif batt:
        bits.append(f"batteries {batt} boxes, ~{_pt(batt_room)} discharged (spent on "
                    f"ESG last turn) - reserve draws on what remains (H7.0)")
    if to_batt:
        bits.append(f"RECHARGE batteries {_pt(to_batt)} (H7.34) - restores the "
                    f"reserve you spent boosting the ESG")
    if unspent > 0:
        # Put it somewhere legal rather than leaving the player to guess. ECM is
        # the one sink that always pays at long range: it lasts the whole turn
        # (H7.12), needs no target, and the manual rates a point of EW above ten
        # points of shield reinforcement. Past a few points it saturates, and the
        # honest answer for the rest is that a ship simply does not generate it.
        ecm_extra = min(unspent, max(0, ECM_USEFUL_CAP - ew))
        left = unspent - ecm_extra
        if ecm_extra > 0:
            bits.append(f"ALLOCATE {_pt(ecm_extra)} to ECM (total ECM {_pt(ew + ecm_extra)}) "
                        f"- lasts the whole turn (H7.12) and needs no target")
        # Surplus beyond ECM goes to SPECIFIC shield reinforcement, sized to the
        # damage he can actually land at the range he will fight at - capped by
        # what is left in the budget. Buying more boxes than he can shoot off is
        # as wasted as buying none.
        plan = None
        if left > 0 and enemies and rng is not None:
            try:
                plan = reinforce_plan(ship, enemies, rng, left)
            except Exception:
                plan = None
        if plan:
            if plan.get("down_facing"):
                # D3.343: the threatened facing is DOWN and cannot take specific
                # reinforcement at all. General is the only legal answer, at the
                # D3.341 rate of 2 energy per point.
                bits.append(f"REINFORCE GENERAL +{plan['general']} "
                            f"(D3.341: 2 energy = 1 point, all facings) - the "
                            f"{SHIELD[plan['primary_idx']]} is DOWN and cannot be "
                            f"specifically reinforced (D3.343), but general still blocks "
                            f"fire from that bearing - sized to the ~{plan['threat']:.0f} "
                            f"he can land at range {plan['at_rng']}")
            else:
                where = f"REINFORCE {SHIELD[plan['primary_idx']]} +{plan['primary']}"
                if plan.get("second_idx") is not None and plan.get("second"):
                    where += f" and {SHIELD[plan['second_idx']]} +{plan['second']}"
                bits.append(f"{where} (specific, D3.342: 1 energy = 1 box) - sized to the "
                            f"~{plan['threat']:.0f} he can land at range {plan['at_rng']}")
            left -= plan["total"]
        # LAST OPTION: surplus beyond ECM and threat-sized reinforcement would
        # otherwise be UNPRODUCED (H7.0). Put it into SPECIFIC reinforcement on
        # the shields he is most likely to hit - 1 energy = 1 box (D3.342), TWICE
        # as efficient as spreading it over all six with general (2 energy = 1
        # box, D3.341). Best guess is the facing he bears on, then its neighbours
        # as the bearing drifts while he closes.
        if left >= 1 and enemies:
            try:
                ox = sum(e["x"] for e in enemies) / len(enemies)
                oy = sum(e["y"] for e in enemies) / len(enemies)
                idx = H.shield_hit((ship["x"], ship["y"]), ship.get("facing", 0),
                                   (round(ox), round(oy)))
                order = [idx, (idx + 1) % 6, (idx - 1) % 6]     # facing, then drift
                pts = int(left)
                spread = []
                for k, f in enumerate(order):
                    give = pts // len(order) + (1 if k < pts % len(order) else 0)
                    if give:
                        spread.append(f"{SHIELD[f]} +{give}")
                if spread:
                    bits.append("REINFORCE " + ", ".join(spread)
                                + f" (LAST OPTION, specific D3.342: 1 energy = 1 box) - "
                                f"the facings he bears on, soaking {pts} surplus that "
                                f"would otherwise not be produced (H7.0)")
                    left -= pts
            except Exception:
                pass
        if left >= 2:
            # No enemy to aim at (or the specific step failed): general is the
            # only remaining sink, at the less-efficient 2-for-1 rate.
            gen_pts = int(left // 2)
            bits.append(f"REINFORCE GENERAL +{gen_pts} (no target to aim at; D3.341: "
                        f"2 energy = 1 point, all facings) - soaks {gen_pts * 2} surplus")
            left -= gen_pts * 2
        if left > 0.01:
            bits.append(f"{_pt(left)} not allocatable this turn - not produced (H7.0/B3.4)")
    resv = " | ".join(bits) if bits else "all power allocated"
    if cap > 0:
        # Prefer the ACTUAL capacitor charge from the client EAF (it knows what
        # was fired); fall back to the fired_phasers boolean only when there is
        # no EAF data.
        cur, used, cap_note = phaser_capacitor_state(ship)
        if cur is not None:
            resv += " | " + cap_note
        else:
            resv += (f" | phaser cap {phaser_capacitor_text(ship)} pt"
                     + (f" - CARRIED OVER FULL, allocate 0 (H6.22)" if not fired_phasers
                        else f" - refill only what you fired (H6.1: holding is free)"))
    seg.append(resv)
    return f'EAF (pwr {pwr}): ' + " | ".join(seg)


# Firing cost per phaser type, and therefore capacitor capacity (H6.21).
PHASER_COST = {"phaser-1": 1.0, "phaser-2": 1.0, "phaser-4": 1.0, "phaser-3": 0.5,
               "phaser": 1.0}


def phaser_capacitor(ship):
    """Capacitor capacity: the power to fire every phaser once (H6.21).

    ph-3s are HALF a point, not one - a detail that materially changes the
    number on a phaser-heavy hull.
    """
    w = ship.get("weapons") or {}
    total = 0.0
    for fam, cost in PHASER_COST.items():
        total += (w.get(fam, [0, 0])[0] or 0) * cost
    return total


def phaser_capacitor_text(ship):
    """Capacity as the rules state it: exact under Fractional Accounting (B3.2),
    otherwise rounded UP to the next whole number (H6.21)."""
    exact = phaser_capacitor(ship)
    if exact <= 0:
        return ""
    rounded = int(exact) if exact == int(exact) else int(exact) + 1
    return f"{exact:g}" if exact == rounded else f"{exact:g} (={rounded} rounded, H6.21)"


def phaser_capacitor_state(ship):
    """(current_charge, used_last_turn, note) from the client EAF, or (None, ..).

    The capacitor holds charge across turns (H6.1); firing draws it down. The
    client tracks it exactly - 'Start Phaser Capacitor' and 'Phaser Capacitor
    Used' per turn - so the current charge is the LATEST turn's Start (which is
    the previous turn's End). This is why 'the Sorcerer fired 1 point' has to be
    read from the EAF, not assumed: the engine's capacity is the FULL value, but
    the live charge is one point lower.
    """
    turns = ship.get("eaf") or []
    if not turns:
        return None, 0.0, ""
    def num(row, key):
        try:
            return float(row.get(key, 0) or 0)
        except (TypeError, ValueError):
            return 0.0
    current = num(turns[-1], "Start Phaser Capacitor")
    # If the latest turn recorded no Start (mid-turn), fall back to Start-Used of
    # the previous turn.
    if not current and len(turns) >= 1:
        prev = turns[-1]
        current = num(prev, "Start Phaser Capacitor") - num(prev, "Phaser Capacitor Used")
    used = num(turns[-1], "Phaser Capacitor Used") or (
        num(turns[-2], "Phaser Capacitor Used") if len(turns) >= 2 else 0.0)
    cap = phaser_capacitor(ship)
    if current and current < cap:
        note = (f"phaser capacitor {current:g}/{cap:g} (fired {used:g} last turn, H6.1) "
                f"- REFILL {cap - current:g} to top it back up")
    else:
        note = f"phaser capacitor {current or cap:g}/{cap:g} - full, allocate 0 (H6.22)"
    return current, used, note


def impulse_note(ship, rng, impulse):
    """Per-impulse movement/turn window for the current impulse."""
    spd = ship["speed"]
    if spd <= 0:
        return "this impulse: stationary (speed 0 - set speed in EAF)"
    moves = R.moves_this_impulse(spd, impulse) if 1 <= impulse <= 32 else False
    tm, letter, ok = turn_mode_of(ship)
    tmtxt = "turn now OK" if tm <= 1 else f"turn after {tm} straight hexes (cat {letter})"
    if not ok:
        tmtxt += " [turn category UNREADABLE in save - assumed C]"
    return f'imp {impulse}: {"MOVES" if moves else "no move"} (spd {spd}); {tmtxt}'

CLIENT = r"C:/Users/jonat/AppData/Local/SFU Online Client"
AUTOSAVE = CLIENT + r"/app/restore/game#SFB_Game1"
# The Java helpers live IN THE PROJECT, not in a session scratchpad. They were
# previously loaded from a temp directory that is cleaned up between sessions,
# which would have broken state parsing the moment the temp dir was swept.
JAVA_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), "java")

# weapon box kinds -> family (from SSD probing)
WEAPON_KINDS = {29: "disruptor", 28: "photon", 33: "phaser-1", 35: "phaser-3", 45: "phaser-2",
                62: "drone", 61: "add"}
# preferred engagement range by dominant weapon (hexes)
PREFERRED = {"disruptor": 8, "photon": 8, "phaser-1": 5, "drone": 12}

FACING_VEC = {0: "N", 1: "NE", 2: "SE", 3: "S", 4: "SW", 5: "NW"}
SHIELD = {0: "#1(fore)", 1: "#2(FR)", 2: "#3(AR)", 3: "#4(aft)", 4: "#5(AL)", 5: "#6(FL)"}

# Bridge personas per race - the officer voice for the in-character brief.
BRIDGE = {
    "LYRAN":      {"addr": "My Lord", "ship": "flagship", "weps": "Master Gunner", "sci": "Sensors", "helm": "Helm",
                   "style": "proud, aggressive Lyran count's-fleet officers - ESG walls and the pounce"},
    "KZINTI":     {"addr": "Admiral", "ship": "flagship", "weps": "Weapons", "sci": "Sensors", "helm": "Navigation",
                   "style": "disciplined Kzinti Hegemony officers - drone saturation and carrier doctrine"},
    "KLINGON":    {"addr": "My Lord", "ship": "battlecruiser", "weps": "Weapons Officer", "sci": "Science", "helm": "Helm",
                   "style": "fierce, honour-bound Klingon officers - disruptor bolts and the overrun"},
    "FEDERATION": {"addr": "Captain", "ship": "starship", "weps": "Tactical", "sci": "Science", "helm": "Helm",
                   "style": "measured, professional Starfleet officers - photon alpha-strikes and shield discipline"},
    "ROMULAN":    {"addr": "Commander", "ship": "warbird", "weps": "Centurion", "sci": "Sensors", "helm": "Helm",
                   "style": "patient, cunning Romulan officers - the cloak and the plasma trap"},
    "GORN":       {"addr": "Captain", "ship": "cruiser", "weps": "Weapons", "sci": "Sensors", "helm": "Helm",
                   "style": "stolid, methodical Gorn officers - heavy plasma and the anvil"},
}
DEFAULT_BRIDGE = {"addr": "Captain", "ship": "ship", "weps": "Weapons", "sci": "Sensors", "helm": "Helm",
                  "style": "professional starship officers"}


def dump_state(path=AUTOSAVE):
    """Run the Java StateDump on the autosave -> parsed dict."""
    cp = CLIENT + "/app/core.jar"
    import glob
    for j in glob.glob(CLIENT + "/app/plugins/*.jar") + glob.glob(CLIENT + "/app/3rdparty/*.jar"):
        cp += ";" + j
    # CREATE_NO_WINDOW (0x08000000) so the java call doesn't flash/steal focus on Windows
    nowin = 0x08000000 if os.name == "nt" else 0
    out = subprocess.run(["java", "-cp", cp, "StateDump.java", path],
                         cwd=JAVA_DIR, capture_output=True, text=True, timeout=60,
                         creationflags=nowin)
    txt = out.stdout
    # StateDump prints a javac warning to stderr and JSON to stdout
    start = txt.find("{")
    return json.loads(txt[start:])


def dominant_weapon(ship):
    w = ship.get("weapons", {})
    fams = {}
    for k, (intact, mx) in w.items():
        fam = WEAPON_KINDS.get(int(k))
        if fam and intact > 0:
            fams[fam] = fams.get(fam, 0) + intact
    if not fams:
        return None
    # Kzinti lean drone+disruptor; pick the highest-count real weapon
    return max(fams, key=fams.get)


def facing_shield(target, from_ship):
    """Which of target's shields faces from_ship (0-5)."""
    return H.shield_hit((target["x"], target["y"]), target["facing"], (from_ship["x"], from_ship["y"]))


def dominant_family(ship):
    w = ship.get("weapons", {})
    heavy = [(f, w[f][0]) for f in ("disruptor", "photon", "plasma", "hellbore", "fusion", "esg") if w.get(f, [0])[0] > 0]
    if heavy:
        return max(heavy, key=lambda z: z[1])[0]
    if w.get("drone", [0])[0] > 0:
        return "drone"
    return "phaser"


IPT = R.IMPULSES_PER_TURN if hasattr(R, "IMPULSES_PER_TURN") else 32


def _abs_imp(t, i):
    return (t - 1) * IPT + i


def _to_ti(a):
    return (a - 1) // IPT + 1, (a - 1) % IPT + 1


def maneuver_note(ship, log):
    """Turn-mode and sideslip availability with countdown.
    Turn Mode (C3.31) = hexes straight before a 60 deg turn is legal.
    Sideslip mode (C4.1) = 1 for ALL units at ALL speeds; a turn resets it (C4.33);
    slips count as forward movement for turn mode (C4.32)."""
    spd = ship["speed"]
    tm, letter, ok = turn_mode_of(ship)
    warn = "" if ok else " [turn category UNREADABLE - assumed C]"
    d = (log or {}).get("maneuver", {}).get(ship["label"])
    if not d:
        return (f'MANEUVER: turn mode {tm} at speed {spd} (cat {letter}){warn}; '
                f'slip mode 1. (no maneuver history yet)')
    st, ss = d["since_turn"], d["since_slip"]
    lt, ls = d.get("last_turn"), d.get("last_slip")
    if tm >= 99:
        turn_txt = f'TURN LOCKED at speed {spd} (turn mode 99)'
    elif st >= tm:
        turn_txt = f'TURN READY ({st}/{tm} straight)'
    else:
        turn_txt = f'turn in {tm - st} hex(es) ({st}/{tm})'
    slip_txt = "SLIP READY (1/1)" if ss >= 1 else "slip in 1 hex (0/1)"
    hist = []
    if lt:
        hist.append(f'turned {lt[0]}.{lt[1]}')
    if ls:
        hist.append(f'slipped {ls[0]}.{ls[1]}')
    tail = ("; last: " + ", ".join(hist)) if hist else ""
    return f'MANEUVER: {turn_txt}; {slip_txt}{tail}.'


# ------------------------------------------------------- seeking weapons
# Drone table (FD1.x): type -> (speed, endurance turns, warhead, damage-to-kill)
DRONE_TYPES = {"I": (8, 3, 12, 4), "II": (12, 2, 12, 4), "III": (12, 25, 12, 4),
               "IV": (8, 3, 24, 6), "V": (12, 2, 24, 6), "VI": (12, 1, 8, 3)}
DRONE_KILL_DAMAGE = 4      # standard drone: 4 damage points destroys it (FD1.x)
DRONE_WARHEAD = 12         # standard warhead
# (verified against FD2.1: type -> speed, endurance turns, warhead, points to kill)

# Speed upgrades (FD2.223/FD2.224); availability ladder at FD10.651.
DRONE_SPEED_UPGRADE = {"-M": (20, 0.5, "Y167"), "-F": (32, 1.0, "Y180")}
DRONE_EXTENDED_RANGE_DOUBLES_ENDURANCE = True          # FD2.222
DRONE_VI_VS_SIZE4PLUS = 2                              # FD2.54: type-VI does only 2 vs SC 4+

# --------------------------------------------------------------------------
# ADD (Anti-Drone Drone) effectiveness by range — E5.61.
# COUNTERINTUITIVE AND IMPORTANT: the ADD is BEST at range 3 and WORST at range 1.
# Range 0 and range 4+ are AUTOMATIC MISSES. Do not close to point-blank to use
# ADDs — hold the drone at range 3.
# --------------------------------------------------------------------------
ADD_HIT_CHANCE = {0: 0.0, 1: 2 / 6, 2: 3 / 6, 3: 4 / 6}     # E5.61
ADD_AUTO_KILLS = True          # E5.2: a hit kills regardless of the drone's armour
ADD_IGNORES_EW = True          # E5.15
ADD_ROUNDS = 6                 # E5.52: 12 after Y175
ADD_ROUNDS_POST_Y175 = 12
ADD_PER_IMPULSE_PER_RACK = 1   # E5.13, 360-degree arc


def add_hit_chance(rng):
    """Probability one ADD round kills a drone at this range (E5.61)."""
    return ADD_HIT_CHANCE.get(rng, 0.0)


def add_note(rng):
    """Tell the player where the ADD envelope actually is."""
    p = add_hit_chance(rng)
    if p == 0:
        return (f"ADD: range {rng} is an AUTOMATIC MISS (E5.61 — only ranges 1-3 work). "
                f"Best ADD range is 3.")
    best = " (BEST ADD range)" if rng == 3 else ""
    return (f"ADD at range {rng}: {p:.0%} to kill{best}; a hit destroys the drone regardless of "
            f"armour (E5.2) and ADDs ignore EW (E5.15).")


# Drones gain ECM with range (E1.7) — so seeker defence is far better close in.
DRONE_ECM_BY_RANGE = [(9, 0), (19, 2), (99, 4)]


def drone_ecm(rng):
    """ECM a drone enjoys at this range (E1.7): +2 at 10-19, +4 at 20+."""
    return next(v for maxr, v in DRONE_ECM_BY_RANGE if rng <= maxr)


# Chaff (D11.32): 4-in-6 that EVERY drone tracking that fighter goes inert. Fighters only.
CHAFF_CHANCE, CHAFF_FIGHTERS_ONLY = 4 / 6, True

# Wild Weasel (J3.0): decoys all seekers targeted on the launcher, plus 6 free ECM.
# Costs 1 point x 2 turns; requires maneuver rate <= 4 and fire control OFF.
# BEATEN BY: type-VI and plasma-K (own lock-on), and Tame/Wild Boar type-IIIs.
WW_ECM, WW_MAX_MANEUVER_RATE = 6, 4
WW_DEFEATED_BY = ("type-VI drones", "plasma-K", "Tame Boar / Wild Boar type-IIIs")

# Seeker control ceiling (F3.21/F3.211): 6 if drone- or plasma-armed, else 3.
# A Kzinti CV doubles this (F3.212).
SEEKER_CONTROL_ARMED, SEEKER_CONTROL_UNARMED = 6, 3


def drone_profile(s):
    """Identify drone type/speed module from its loadout, with warhead + kill damage.
    Base types (FD1.x) are speed 8-12; speed modules (MM etc) raise actual speed."""
    lo = (s.get("loadout") or "")
    spd = s.get("speed") or s.get("max_speed") or 12
    warhead, kill = DRONE_WARHEAD, DRONE_KILL_DAMAGE
    # heavy warheads (type IV/V, FD1.x) are 24 pts and need 6 damage to kill.
    # Match only an explicit type/heavy marker - a bare "24" in the loadout string
    # (package numbers etc) is not evidence of a heavy warhead.
    if re.search(r"\bheavy\b|\btype[- ]?(IV|V)\b|warhead[^0-9]{0,8}24\b", lo, re.I):
        warhead, kill = 24, 6
    mod = None
    m = re.search(r"Speed Module:\s*([A-Z]+)", lo)
    if m:
        mod = m.group(1)
    typ = "std"
    m = re.search(r"Type[- ]?([IVX]+)", lo)
    if m:
        typ = m.group(1)
    return {"speed": spd, "warhead": warhead, "kill": kill, "module": mod, "type": typ}


# Drone speed by speed module, straight from the client's own alphadrones.expendable:
#   SPEED, ,8   SPEED,S,12   SPEED,M,20   SPEED,F,32
DRONE_MODULE_SPEED = {None: 8, "": 8, "S": 12, "M": 20, "F": 32}
# CONTAINER,<type>,<class>,<spaces>,<damage to kill> from the same file.
DRONE_TYPE_KILL = {"I": 4, "II": 4, "III": 4, "IV": 6, "V": 6, "VI": 2, "H": 6}


# Drone rack types, from the client's own boxtypes.names, with the rate of fire
# each one actually has (FD3.0). Rate of fire is the constraint that usually
# binds - not ammunition, and not control channels:
#   FD3.1 type-A  4 spaces, ONE per turn (the default: "all drone racks are of
#                 this type unless stated otherwise in the ship descriptions")
#   FD3.2 type-B  6 spaces, one per turn
#   FD3.3 type-C  4 spaces, TWO per turn, not within 12 impulses of each other -
#                 "the Kzintis favored this type because it could put more drones
#                 in flight more quickly"
#   FD3.5 type-E  8 dogfight drones, up to four per turn
#   FD3.0 universal: "no drone rack can fire two drones within 1/4 turn of each
#                 other, even if on different turns" - 8 impulses, except where a
#                 rack type says otherwise (type-C's 12).
#
# ADD boxes live in the same drone_racks vector because they share the expendable
# mechanism, but they are ANTI-DRONE defence (E5.0), not launchers. Counting them
# as offensive rounds overstated a Kzinti CV's magazine by six, and would have
# credited an escort holding 48 ADD rounds with a drone battery it does not have.
DRONE_RACK_TYPES = {
    62:  ("A", 1, 8),
    39:  ("B", 1, 8),
    63:  ("C", 2, 12),
    64:  ("D", 1, 8),
    65:  ("E", 4, 8),
    66:  ("F", 1, 8),
    67:  ("G", 1, 8),
    155: ("GX", 1, 8),
    160: ("DX", 1, 8),
    161: ("H", 1, 8),
    162: ("HX", 1, 8),
    178: ("CX", 2, 12),
    14:  ("?", 1, 8),          # generic "Drone" box, type unspecified
}
ADD_BOX_TYPES = {152: "ADD-6", 153: "ADD-12", 154: "ADD-30"}


def rack_kind(box_type):
    """('drone'|'add'|'unknown', label, shots_per_turn, impulse_gap)."""
    if box_type in ADD_BOX_TYPES:
        return "add", ADD_BOX_TYPES[box_type], 0, 0
    t = DRONE_RACK_TYPES.get(box_type)
    if t:
        return "drone", "type-" + t[0], t[1], t[2]
    return "unknown", "box %s" % box_type, 1, 8


def _decode_round(rd):
    """Decode one 'Type-<container>-<package>' round from the client's format.

    Grammar is the client's own alphadrones.expendable: a CONTAINER name, then a
    single-letter PACKAGE, optionally prefixed by a SPEED module letter
    (blank=8, S=12, M=20, F=32).
    """
    m = re.match(r"Type-([IVXH]+)-?([A-Za-z]*)$", (rd or "").strip())
    if not m:
        return {"type": "I", "speed": DRONE_MODULE_SPEED[None],
                "warhead": DRONE_WARHEAD, "kill": DRONE_KILL_DAMAGE,
                "module": None}
    typ, tail = m.group(1).upper(), m.group(2) or ""
    # A leading capital S/M/F on the tail is the speed module; the trailing
    # letter is the warhead package (x/X = explosive, r/R = armour, etc).
    mod = tail[0].upper() if tail[:1].upper() in ("S", "M", "F") else None
    return {"type": typ,
            "speed": DRONE_MODULE_SPEED.get(mod, DRONE_MODULE_SPEED[None]),
            "warhead": 24 if typ in ("IV", "V", "H") else DRONE_WARHEAD,
            "kill": DRONE_TYPE_KILL.get(typ, DRONE_KILL_DAMAGE),
            "module": mod}


def rack_drone_profile(ship):
    """What this ship's racks will ACTUALLY launch, read per rack from the save.

    drone_profile() reads `speed` off the piece it is handed, which is right for
    a seeker already on the board but catastrophic for a rack: passing the ship
    made the launch decision key off the SHIP'S OWN THROTTLE, so slowing to speed
    4 convinced the engine its drones were speed-4 weapons.

    Each rack carries its own ammunition list, so a ship can hold fast Type-Is in
    one rack and heavy Type-IVs in another. The summary reports the SLOWEST round
    available (that is what a co-ordinated launch is limited to) while keeping the
    per-rack detail for the order line. An empty rack is reported as empty rather
    than silently assumed loaded.
    """
    all_racks = ship.get("drone_racks") or []
    # Separate LAUNCHERS from ADD point-defence. Both live in drone_racks, but an
    # ADD cannot be launched at a ship, and counting its rounds as offensive
    # drones inflated the magazine - the Sabre read 18 rounds where 12 are real,
    # and an escort holding 48 ADD rounds would have been credited with a drone
    # battery it does not possess.
    racks = [r for r in all_racks if rack_kind(r.get("box_type", 0))[0] != "add"]
    adds = [r for r in all_racks if rack_kind(r.get("box_type", 0))[0] == "add"]
    add_rounds = sum(len(r.get("ammo") or []) for r in adds)

    rounds = []
    for r in racks:
        for rd in (r.get("ammo") or []):
            rounds.append(_decode_round(rd))

    # FD3.x: rate of fire is per RACK and set by its type. This is normally the
    # binding limit - a ship with 12 drones across four type-A racks can still
    # only put four in the air this turn.
    detail, per_turn = [], 0
    for r in racks:
        _k, label, shots, gap = rack_kind(r.get("box_type", 0))
        n = len(r.get("ammo") or [])
        per_turn += min(shots, n)
        detail.append({"designator": r.get("designator"), "n": n, "rack": label,
                       "shots": shots, "gap": gap,
                       "types": sorted({_decode_round(x)["type"]
                                        for x in (r.get("ammo") or [])})})

    if not rounds:
        return {"speed": DRONE_MODULE_SPEED[None], "warhead": DRONE_WARHEAD,
                "kill": DRONE_KILL_DAMAGE, "module": None, "type": "I",
                "known": False, "rounds": 0, "racks": detail, "per_turn": 0,
                "adds": add_rounds, "add_racks": len(adds), "empty": bool(racks)}
    slowest = min(r["speed"] for r in rounds)
    heavy = [r for r in rounds if r["warhead"] > DRONE_WARHEAD]
    return {"speed": slowest,
            "warhead": max(r["warhead"] for r in rounds),
            "kill": max(r["kill"] for r in rounds),
            "module": next((r["module"] for r in rounds if r["module"]), None),
            "type": sorted({r["type"] for r in rounds})[0],
            "known": True, "rounds": len(rounds), "heavy": len(heavy),
            "racks": detail, "per_turn": per_turn,
            "adds": add_rounds, "add_racks": len(adds), "empty": False}


def incoming_seekers(ship, state):
    """Seeking weapons currently tracking this ship, nearest first.

    Each seeker is classed drone-vs-plasma (they defend differently) and carries
    its EXACT flight time plus, where known, its remaining endurance. An expired
    seeker is dropped: FD1.7 takes it off the board, and a drone about to run dry
    must not keep blocking disengagement (C7.22). Endurance that is unknown is
    treated as still-live - a seeker is never removed on a guess.
    """
    import sfb_seekers as SK
    turn = state.get("turn", 1)
    impulse = state.get("impulse", 1)
    out = []
    me = (ship["x"], ship["y"])
    my_spd = max(0, ship.get("speed", 0))
    for s in state.get("seeking", []):
        if s.get("target") and s["target"] != ship["label"]:
            continue
        info = SK.describe(s, turn, impulse, my_pos=me)
        if info.get("expired") is True:
            continue                      # off the board - not a threat, not a block
        rng = H.hex_distance(me, (s["x"], s["y"]))
        prof = drone_profile(s)
        spd = prof["speed"]
        closing = spd - my_spd            # negative/zero => it cannot catch us
        turns = (rng / closing) if closing > 0 else None
        warhead = prof["warhead"]
        if info.get("seeker_class") == "plasma" and info.get("projected_strength") is not None:
            warhead = info["projected_strength"]
        out.append({"label": s["label"], "kind": s.get("kind") or "Drone",
                    "range": rng, "speed": spd, "closing": closing,
                    "turns_to_impact": turns, "warhead": warhead,
                    "kill": prof["kill"], "module": prof["module"], "type": prof["type"],
                    "seeker_class": info.get("seeker_class", "drone"),
                    "turns_left": info.get("turns_left"),
                    "endurance_note": info.get("endurance_note"),
                    "defence_note": info.get("defence"),
                    "strength_note": info.get("strength_note")})
    out.sort(key=lambda z: (z["turns_to_impact"] is None, z["turns_to_impact"] or 0, z["range"]))
    return out


def seeker_defence(ship, seekers, state=None, turn=1):
    """Authentic anti-seeker options AND the net outcome (FD/E5/J/G23).

    The old version listed the defences but never netted them out, and it read
    the GENERIC phaser bucket (0 on any ship whose phasers are split into
    phaser-1/2/3), so it silently reported no point-defence at all. Both fixed:
    the phaser count is summed across types, and the ESG pool + phaser PD are
    netted against the wave to answer the real question - how many leak through,
    and does the ship survive.
    """
    if not seekers:
        return None
    w = ship.get("weapons", {})
    nph = sum(w.get(f, [0, 0])[0] for f in
              ("phaser-1", "phaser-2", "phaser-3", "phaser-4", "phaser"))
    add = w.get("add", [0, 0])[0] if "add" in w else 0
    sysm = ship.get("systems", {})
    trac = sysm.get("tractor", [0, 0])[0]
    n = len(seekers)
    total_warhead = sum(s["warhead"] for s in seekers)
    kill_each = max(s["kill"] for s in seekers)
    warhead_each = max((s["warhead"] for s in seekers), default=0)
    nearest = seekers[0]
    spds = sorted({s["speed"] for s in seekers})
    spd_txt = "/".join(str(x) for x in spds)
    my_spd = ship.get("speed", 0)
    if nearest["turns_to_impact"] is None:
        head = (f'{n} seeker(s) at speed {spd_txt} vs your {my_spd} - CANNOT CATCH YOU; '
                f'hold speed and let their endurance expire')
    else:
        head = (f'{n} seeker(s) speed {spd_txt} (you {my_spd}, closing {nearest["closing"]}/turn); '
                f'nearest {nearest["range"]} hex, impact ~{nearest["turns_to_impact"]:.1f} turns; '
                f'up to {total_warhead} damage')

    # --- NET OUTCOME, by ARRIVAL TURN. Drones do NOT all hit at once - strung
    # out behind a ship they barely outpace, they trickle in over several turns.
    # Point-defence (phasers + ADD) RECHARGES every turn; the ESG is a ONE-SHOT
    # pool (G23.222). So the real danger is not the total wave but the WORST
    # SINGLE TURN's arrivals versus that turn's point-defence. The old model
    # netted all N simultaneously and cried SERIOUS at a wave the ship can in
    # fact point-defence indefinitely.
    import math
    esg_pool = 0
    try:
        import sfb_actions as ACT
        charges, _ = ACT.esg_charges(ship, turn)
        if charges and (ship.get("weapons") or {}).get("esg", [0, 0])[0]:
            esg_pool = ACT.esg_combined_field(charges, 0)     # r0 = strongest
    except Exception:
        pass
    esg_kills = esg_pool // max(1, kill_each)
    pd_per_turn = nph + add                        # ~1 drone each, recharging

    buckets = {}
    for s in seekers:
        tti = s.get("turns_to_impact")
        if tti is None:
            continue
        buckets.setdefault(max(1, math.ceil(tti)), []).append(s)

    facing_sh = 0
    try:
        idx = facing_shield(ship, {"x": nearest.get("x", ship["x"]),
                                   "y": nearest.get("y", ship["y"])})
        facing_sh = (ship.get("shields") or [0] * 6)[idx]
    except Exception:
        pass

    # Spend the one-shot ESG on the heaviest single bucket; phasers/ADD every turn.
    worst = max(buckets, key=lambda t: len(buckets[t])) if buckets else None
    esg_used, sched, total_leak = False, [], 0
    for t in sorted(buckets):
        arrivals = len(buckets[t])
        esg_here = 0
        if not esg_used and t == worst and (arrivals > pd_per_turn or len(buckets) == 1):
            esg_here, esg_used = esg_kills, True
        stopped = min(arrivals, pd_per_turn + esg_here)
        leak = arrivals - stopped
        total_leak += leak
        tag = f"T+{t}: {arrivals} arrive, PD stops {min(arrivals, pd_per_turn)}"
        if esg_here:
            tag += f" +ESG {esg_here}"
        if leak > 0:
            tag += f" -> {leak} LEAK"
        sched.append(tag)

    if total_leak <= 0:
        outcome = (f"OUTCOME: the wave arrives in {len(buckets)} turn(s), not at once. "
                   f"Point-defence ({pd_per_turn}/turn, recharging) + one ESG volley "
                   f"stops EVERY arrival. Schedule: " + "; ".join(sched))
    else:
        leak_dmg = total_leak * warhead_each
        internals = max(0, leak_dmg - facing_sh)
        outcome = (f"OUTCOME: arrives over {len(buckets)} turn(s). "
                   + "; ".join(sched)
                   + f" | ~{total_leak} total leak, ~{internals} internal past the "
                   f"{facing_sh}-box shield"
                   + (" - SURVIVABLE" if internals < 8 else
                      " - SERIOUS on the worst turn; save the ESG for it, or open the range"))

    opts = []
    if add:
        opts.append(f'ADD x{add} (ignores ECM)')
    if nph:
        opts.append(f'{nph} phasers for PD (FD1.51: unpenalised vs drones)')
    if trac:
        opts.append(f'tractor x{trac}')
    if not (ship.get("weapons") or {}).get("esg", [0, 0])[0]:
        opts.append('Wild Weasel voids ALL tracking seekers (needs free shuttle)')
    if nearest["turns_to_impact"] is not None and my_spd < max(s["speed"] for s in seekers):
        opts.append(f'outrunning needs speed >{max(s["speed"] for s in seekers)}')
    return "SEEKERS: " + head + " | " + outcome + " | defend: " + "; ".join(opts)


# -------------------------------------------------- damage-exchange model
# AUTHENTIC charts extracted from the Master Rulebook:
#   Disruptor (E3.4)  p124   |  Photon (E4) p124
# Each entry: (max_range, damage, hit_on) where hit_on = highest d6 that hits.
# Parsed from the client's weapons.chart where available; the literals are the
# fallback. Both DISR tables were already exact - the parser reproduces them
# cell for cell, which is the cross-check that validates the parser itself.
_DISR_STD_FALLBACK = [(0, 0, 0), (1, 5, 5), (2, 4, 5), (4, 4, 4), (8, 3, 4),
                      (15, 3, 4), (22, 2, 3), (30, 2, 2), (40, 1, 2)]
_DISR_OVL_FALLBACK = [(0, 10, 6), (1, 10, 5), (2, 8, 5), (4, 8, 4), (8, 6, 4),
                      (15, 0, 0), (40, 0, 0)]
_PHOT_STD_FALLBACK = [(1, 0, 0), (2, 8, 5), (4, 8, 4), (8, 8, 3), (12, 8, 2),
                      (30, 8, 1)]
try:
    import sfb_charts as _CH2
    DISR_STD = _CH2.DISR_STD or _DISR_STD_FALLBACK
    DISR_OVL = _CH2.DISR_OVL or _DISR_OVL_FALLBACK
    PHOT_STD = _CH2.PHOT_STD or _PHOT_STD_FALLBACK
    # Hellbore and fusion were previously truncated at range 15; the real
    # hellbore chart runs to 40. Its to-hit column is a d20 target number, NOT
    # a d6 span - use _CH2.hit_probability("Hellbore", n), never n/6.
    HELLBORE_STD = _CH2.HELLBORE
    HELLBORE_OVL = _CH2.HELLBORE_OVL
    FUSION_STD = _CH2.FUSION
    FUSION_OVL_TBL = _CH2.FUSION_OVL
except Exception:
    DISR_STD, DISR_OVL, PHOT_STD = (_DISR_STD_FALLBACK, _DISR_OVL_FALLBACK,
                                    _PHOT_STD_FALLBACK)
    HELLBORE_STD = HELLBORE_OVL = FUSION_STD = FUSION_OVL_TBL = []
# Photon overload: the HIT numbers below are correct, but the damage is NOT a flat
# 16 — per E4.413 the warhead scales with energy invested, in half-point steps:
# 4.5pts->9, 5->10, 5.5->11, 6->12, 6.5->13, 7->14, 7.5->15, 8->16, i.e.
# warhead = floor(2 * total_energy), capped at 16. Below 4.5 total it cannot fire
# as an overload at all (E4.414). photon_overload_damage() implements this.
PHOT_OVL = [(1, 16, 6), (2, 16, 5), (4, 16, 4), (8, 16, 3), (30, 0, 0)]
PHOT_OVL_MIN_ENERGY = 4.5   # E4.414
PHOT_OVL_MAX_WARHEAD = 16   # E4.413

# Photon PROXIMITY mode (E4.3) — was missing entirely. Auto-misses below true
# range 9, but at long range it beats standard photons on expected damage.
PHOT_PROX = [(8, 0, 0), (12, 4, 4), (30, 4, 3)]

# Disruptor overload with a UIM is better than plain overload at ranges 3-8
# (hits on 1-5 rather than 1-4).
DISR_OVL_UIM = [(0, 10, 6), (1, 10, 5), (2, 8, 5), (4, 8, 5), (8, 6, 5),
                (15, 0, 0), (40, 0, 0)]

# --------------------------------------------------------------------------
# Disruptor MAXIMUM range — Annex #8A (Module G3, the Master Annexes).
#
# The annex opens by resolving the question directly: "In the Captain's Edition,
# all ships have an SSD and the range of the disruptors (on those ships armed with
# that weapon) is shown on that SSD. This table is now used only for some special
# cases." So for a normal warship the answer is IN THE GAME DATA, not a chart —
# read it from the SSD rather than assuming the 40-hex extent of DISR_STD.
#
# The special cases below are the ones the annex still governs.
# --------------------------------------------------------------------------
DISRUPTOR_RANGE_SPECIAL = {
    # small craft — much shorter than ships
    "FIGHTER": 10, "HEAVY FIGHTER": 10, "INTERCEPTOR": 10, "PF": 10,
    # auxiliaries and freighters
    "ARMED FREIGHTER SMALL": 15, "ARMED FREIGHTER LARGE": 22,
    "NAVAL AUXILIARY SMALL": 15, "NAVAL AUXILIARY LARGE": 22, "Q-SHIP": 22,
    "CAPTOR MINE": 15, "DEFSAT": 15,
    # fixed defences
    "BS": 30, "BSX": 40, "BATS": 40, "BATSX": 40,
    "STB": 40, "STX": 40, "SB": 40, "SBX": 40, "MONITOR": 40,
    "GROUND DISRUPTOR": 40, "GROUND DISRUPTOR X": 40,
    # pods and pallets
    "P-B4": 30, "P-B3": 30, "PAL-BT": 30,
    # WYN auxiliaries
    "WYN AUX DN": 30, "WYN AUX BCS": 30, "WYN AUX BC": 30, "WYN ABX": 40,
}
DISRUPTOR_RANGE_FROM_SSD = ("Normal warships: disruptor max range is printed on the SSD and "
                            "carried in the game data — read it there (Annex #8A). Do NOT assume "
                            "the chart's 40-hex extent.")


def disruptor_max_range(ship):
    """Max disruptor range for a unit, or None if it must be read from the SSD."""
    t = (ship.get("type") or "").upper().replace("-", "").strip()
    for key, rng in DISRUPTOR_RANGE_SPECIAL.items():
        if key.replace("-", "").replace(" ", "") == t.replace(" ", ""):
            return rng
    # Fall back to the chart's own reach rather than None. Returning None made
    # the bridge omit the line entirely, reading as "no information" when the
    # real answer is that a standard disruptor reaches the full chart distance -
    # which matters when the alternative is losing an armed round to E3.24.
    if DISR_STD:
        return int(DISR_STD[-1][0])
    return None


IMPULSES_PER_TURN = 32


def _shadow_move_of(head):
    """Map a movement-order headline to a sfb_shadow MOVE keyword.

    The advisor's heads read STRAIGHT / TURN LEFT|RIGHT to X / SIDESLIP LEFT|RIGHT
    / HET / HOLD; the shadow referee needs the bare keyword so it can apply the
    ADVISED move (not just straight-line) and a followed order reconciles clean.
    """
    h = (head or "").upper()
    if h.startswith("HOLD"):
        return "HOLD"
    if h.startswith("HET"):
        return "HET"
    if h.startswith("SIDESLIP LEFT"):
        return "SLIP_LEFT"
    if h.startswith("SIDESLIP RIGHT"):
        return "SLIP_RIGHT"
    if h.startswith("TURN LEFT"):
        return "TURN_LEFT"
    if h.startswith("TURN RIGHT"):
        return "TURN_RIGHT"
    return "STRAIGHT"


def use_or_lose_deadline(ship, enemies, rng, turn, impulse, overloaded=False):
    """When an armed disruptor must fire, and whether the range will allow it.

    E3.24 makes a disruptor use-or-lose: the energy put into arming it is gone
    at the turn break whether or not it fired. That turns "hold for a better
    shot" into a bet against the clock, and the engine was making that bet
    silently - it said "HOLD until range 8" without ever mentioning that there
    might not BE a range 8 before impulse 32.

    An OVERLOADED disruptor is sharper still: it cannot fire past range 8 at
    all (E3.51), so if the closing rate does not deliver range 8 in time the
    whole overload is wasted, where a standard load would reach the full chart.
    """
    n = (ship.get("weapons") or {}).get("disruptor", [0, 0])[0]
    if not n or rng is None:
        return None
    left = max(0, IMPULSES_PER_TURN - int(impulse))

    try:
        closing = closing_rate(ship, enemies) if enemies else 0
    except Exception:
        closing = 0
    per_imp = (float(closing) / IMPULSES_PER_TURN) if closing else 0.0

    ovl_reach = 8
    std_reach = disruptor_max_range(ship) or 40
    need = ovl_reach if overloaded else std_reach
    tag = "OVERLOADED" if overloaded else "standard"

    if rng <= need:
        if not overloaded:
            return (f"DISRUPTORS: in range NOW ({rng}, reach {std_reach}) - "
                    f"{left} impulse(s) left this turn",
                    [f"E3.24: an armed disruptor not fired this turn is WASTED - "
                     f"the arming energy does not carry over",
                     f"at range {rng} the standard chart still pays; holding for "
                     f"a better band risks losing the round entirely"])
        return None

    gap = rng - need
    eta = int(gap / per_imp) + 1 if per_imp > 0 else None
    why = []
    if eta is None:
        why.append("closing rate is zero or negative - the range will NOT come to us")
    else:
        why.append(f"closing ~{per_imp:.2f} hex/impulse, so range {need} arrives "
                   f"about impulse {int(impulse) + eta}")
    why.append(f"E3.24: unfired armed disruptors are LOST at the turn break "
               f"({left} impulse(s) left)")
    if overloaded:
        why.append(f"E3.51: an OVERLOADED disruptor cannot fire beyond {ovl_reach} "
                   f"at all - if that range does not arrive in time the overload "
                   f"is wasted, where a standard load would reach {std_reach}")

    if eta is None or eta > left:
        return (f"DISRUPTORS ({tag}): WILL NOT REACH FIRING RANGE THIS TURN - "
                f"take the best shot available before impulse {IMPULSES_PER_TURN}",
                why)
    return (f"DISRUPTORS ({tag}): firing range ~impulse {int(impulse) + eta}, "
            f"{left} impulse(s) left this turn", why)


def photon_overload_damage(total_energy):
    """Warhead strength of an overloaded photon for the energy actually invested.

    E4.413: half-point steps, warhead = floor(2 * energy), capped 16.
    E4.414: below 4.5 points total it cannot fire as an overload at all.
    """
    if total_energy is None:
        return PHOT_OVL_MAX_WARHEAD          # legacy callers assume a full 8-pt load
    if total_energy < PHOT_OVL_MIN_ENERGY:
        return 0
    return min(PHOT_OVL_MAX_WARHEAD, int(2 * total_energy))


# Feedback damage to the FIRING ship's own facing shield at point-blank overload:
#   disruptor overload at range 0 -> 2 points (E3.54)
#   photon overload at range 0-1  -> 1-4 points per the E4.413 table (E4.43)
FEEDBACK = {"disruptor": {0: 2}, "photon": {0: 4, 1: 4}}


def feedback_warning(family, rng, overloaded):
    if not overloaded:
        return None
    pts = FEEDBACK.get(family, {}).get(rng)
    if not pts:
        return None
    cite = "E3.54" if family == "disruptor" else "E4.43"
    return (f"WARN {family} overload at range {rng} feeds back up to {pts} points onto your OWN "
            f"facing shield ({cite}).")


# --------------------------------------------------------------------------
# Can a weapon HOLD its charge across the turn break? Drives "use it or lose it".
# (verified from the Master Rulebook; cost is energy per turn to maintain)
# --------------------------------------------------------------------------
HOLD_ARMED = {
    "disruptor":       (False, None, "E3.24/E3.51 — cannot hold; fire it or lose the charge"),
    "photon":          (True, 1.0, "E4.22/E4.44 — fully armed only (OL costs 1.25-2/turn); "
                                   "a PARTIALLY armed photon canNOT hold"),
    "fusion":          (False, None, "E7.23 — pre-Y168 cannot hold; Y168+ refit CAN (1 pt, "
                                     "size-4+ only, E7.51/E7.54); ANY overload cannot (E7.412)"),
    "hellbore":        (False, None, "E10.22 — cannot hold, but has a ROLLING DELAY: 3 pts/turn "
                                     "indefinitely. Overloaded: no rolling delay (E10.611)"),
    "mauler":          (True, 0.0, "E8.31 — energy banks in the batteries"),
    "ppd":             (True, 2.0, "E11.22 — 2 pts; overloads canNOT hold"),
    "plasma-R":        (False, None, "FP2.12 — ships cannot hold R-torps; a STARBASE can"),
    "plasma-S":        (True, 2.0, "FP2.51"),
    "plasma-G":        (True, 1.0, "FP2.51"),
    "plasma-F":        (True, 0.0, "FP2.51 — free to hold"),
    "esg":             (True, 0.0, "G23.221 — stores up to 25 turns"),
}

# E1.50: the 8-impulse reload is UNIVERSAL to all direct-fire weapons, not
# weapon-specific. Exceptions at E1.213: maulers, PPDs, gatlings, ADDs, MCIDS.
UNIVERSAL_RELOAD_IMPULSES = 8
RELOAD_EXEMPT = {"mauler", "ppd", "gatling", "add", "mcids"}

# ESG reality check (Lyran) — the engine must not over-value the ESG:
ESG_TRUTH = (
    "G23.81: the ESG has NO effect whatsoever on plasma torpedoes, PPTs or PPDs, and G23.83: "
    "none on direct-fire weapons. Its value is anti-drone / anti-fighter / anti-mine ONLY. "
    "G23.84: a hellbore fired at an ESG HITS AUTOMATICALLY — no roll — including hellbores "
    "merely crossing the sphere en route elsewhere, with overflow applied to the generating ship "
    "without further range reduction. Against Hydrans inside range 8, raising the ESG is often "
    "actively HARMFUL."
)
# Phaser expected damage by range.
#
# SOURCE NOTE: the phaser tables are NOT in the Master Rulebook — E2.41 only refers
# to "the appropriate phaser chart", and no annex PDF exists in the rulebook folder.
# These were recovered from the Captain's Basic Set SSD Book 2011 (ADB5501-3.pdf),
# which is image-only, by rendering pages at 450-600dpi and reading the raster:
# Ph-1/Ph-3 from p.14 (Fed CL), Ph-2 from p.28 (Klingon C8), Ph-4 from p.24
# (Starbase). Six worked examples in the Master Rulebook (E2.411, E2.412, E1.822)
# and the Cadet Handbook independently confirm specific cells of the Ph-1, Ph-2 and
# Ph-3 tables, and Ph-3 was read twice off two different SSDs and matched.
#
# CAVEAT: Ph-4, and the Ph-1 columns beyond range 4, are SINGLE-SOURCE (image only,
# no textual cross-check). Treat marginal calls that hinge on them with care.
#
# The expectations are FLAT INSIDE EACH BRACKET by construction — a Ph-1 at range 9
# and at range 15 are identical. Never interpolate within a bracket.
# These are now PARSED from the client's own weapons.chart (see sfb_charts)
# rather than transcribed. The hand-typed values below survive only as a
# fallback for a missing chart file, and two of them were badly wrong:
#   PH3_EXPECTED claimed 4.500 at range 0 where the chart averages 3.833, and
#   1.500 at range 5 - a bracket that does not exist; the real 4-8 bracket
#   averages 0.333. The engine believed a ph-3 was about four times the weapon
#   it actually is, on a live threat-evaluation path.
#   PH2_EXPECTED stopped at range 30; a Ph-2 reaches 50.
# PH1_EXPECTED and DISR_STD were exact, and the parser reproduces both to three
# decimals - which is what validates the parser.
_PH1_FALLBACK = [(0, 6.500), (1, 5.333), (2, 4.833), (3, 4.333), (4, 3.833),
                 (5, 3.500), (8, 2.167), (15, 1.000), (25, 0.500),
                 (50, 0.333), (75, 0.167), (99, 0.0)]
_PH2_FALLBACK = [(0, 5.500), (1, 4.167), (2, 3.833), (3, 3.500), (8, 1.167),
                 (15, 0.667), (30, 0.333), (50, 0.167), (99, 0.0)]
_PH3_FALLBACK = [(0, 3.833), (1, 3.667), (2, 3.000), (3, 1.000),
                 (8, 0.333), (15, 0.167), (99, 0.0)]
_PH4_FALLBACK = [(3, 18.333), (5, 15.000), (8, 11.667), (15, 6.667),
                 (25, 3.333), (50, 1.667), (99, 0.0)]
try:
    import sfb_charts as _CH
    PH1_EXPECTED = _CH.PH1_EXPECTED or _PH1_FALLBACK
    PH2_EXPECTED = _CH.PH2_EXPECTED or _PH2_FALLBACK
    PH3_EXPECTED = _CH.PH3_EXPECTED or _PH3_FALLBACK
    PH4_EXPECTED = _CH.PH4_EXPECTED or _PH4_FALLBACK
except Exception:
    _CH = None
    PH1_EXPECTED, PH2_EXPECTED = _PH1_FALLBACK, _PH2_FALLBACK
    PH3_EXPECTED, PH4_EXPECTED = _PH3_FALLBACK, _PH4_FALLBACK

PHASER_TABLE = {"phaser-1": PH1_EXPECTED, "phaser-2": PH2_EXPECTED,
                "phaser-3": PH3_EXPECTED, "phaser-4": PH4_EXPECTED,
                "phaser-G": PH3_EXPECTED,     # E2.152: ph-G fires on the ph-3 table
                "phaser": PH1_EXPECTED}       # bare "phaser" defaults to Ph-1

# Ph-G fires on the ph-3 table (E2.152), 4 shots/turn at 1/4 point each (E2.151).
PHG_SHOTS_PER_TURN, PHG_COST_PER_SHOT = 4, 0.25

# The two range cliffs the AI must respect — neither is a smooth falloff:
#   Ph-1: 3.500 at R5 -> 2.167 at R6 (-38% for ONE hex), then 1.000 at R9 (-71% from R5)
#   Ph-2: 3.500 at R3 -> 1.167 at R4 (-67%), leaving it ~30% of a Ph-1 at ranges 4-8
PHASER_CLIFFS = {
    "phaser-1": [(5, 6, "Ph-1 loses 38% of its expected damage crossing range 5->6, and 71% "
                        "by range 9. Range 5 is the wall worth fighting for.")],
    "phaser-2": [(3, 4, "Ph-2 collapses 67% crossing range 3->4 — near-parity with a Ph-1 at "
                        "range 3, but only ~30% of one at ranges 4-8.")],
}

# Point defence is NOT a distinct mechanic (FD1.51/FD1.52): phasers are simply
# UNPENALIZED against drones, while photons/disruptors/fusion take a 4-point ECM
# penalty. That asymmetry — not a special PD rule — is why phasers kill drones.
SEEKER_ECM_PENALTY_NONPHASER = 4


def _chart(chart, rng):
    for maxr, dmg, hit in chart:
        if rng <= maxr:
            return dmg, hit
    return 0, 0


def expected_damage(attacker, target, rng, allow_overload=True):
    """Expected damage this attacker lands on target at this range, per exchange."""
    w = attacker.get("weapons", {})
    total = 0.0
    parts = []
    by_family = []          # (EW family, damage) so EW degrades each correctly
    disr = w.get("disruptor", [0, 0])[0]
    if disr:
        d_o, h_o = _chart(DISR_OVL, rng)
        d_s, h_s = _chart(DISR_STD, rng)
        use_ovl = allow_overload and d_o * h_o > d_s * h_s
        d, h = (d_o, h_o) if use_ovl else (d_s, h_s)
        v = disr * d * (h / 6.0)
        total += v
        if v:
            parts.append(f'{disr}disr{"OVL" if use_ovl else ""} {v:.0f}')
        by_family.append(("disruptor", v))
    phot = w.get("photon", [0, 0])[0]
    if phot:
        # Compare all THREE photon modes, not just standard vs overload. The
        # proximity line (E4.3) auto-misses inside true range 9 but wins at long
        # range; the overload warhead scales with energy invested (E4.413).
        d_s, h_s = _chart(PHOT_STD, rng)
        d_p, h_p = _chart(PHOT_PROX, rng)
        ovl_energy = attacker.get("photon_overload_energy")
        d_o, h_o = _chart(PHOT_OVL, rng)
        d_o = photon_overload_damage(ovl_energy) if h_o else 0
        modes = [("", d_s, h_s), ("PROX", d_p, h_p)]
        if allow_overload:
            modes.append(("OVL", d_o, h_o))
        tag, d, h = max(modes, key=lambda m: m[1] * m[2])
        v = phot * d * (h / 6.0)
        total += v
        if v:
            parts.append(f'{phot}phot{tag} {v:.0f}')
        by_family.append(("photon", v))
    # Each phaser type has its own table and its own range cliffs — a Ph-2 at
    # range 4 is worth ~30% of a Ph-1, so they cannot share one curve.
    for fam, table in (("phaser-1", PH1_EXPECTED), ("phaser-2", PH2_EXPECTED),
                       ("phaser-3", PH3_EXPECTED), ("phaser-4", PH4_EXPECTED),
                       ("phaser", PH1_EXPECTED)):
        n = w.get(fam, [0, 0])[0]
        if not n:
            continue
        # Beyond the chart's last bracket the weapon simply does not reach, so
        # the expectation is 0. This needs an explicit default: the hand-written
        # tables used to carry a (99, 0.0) sentinel row, but the tables parsed
        # from the client's weapons.chart end at the weapon's TRUE maximum range
        # (a ph-3 stops at 15), and a bare next() raised StopIteration past it.
        exp = next((e for maxr, e in table if rng <= maxr), 0.0)
        v = n * exp
        total += v
        if v:
            parts.append(f'{n}{fam.replace("phaser", "ph")} {v:.1f}')
        by_family.append(("phaser_short" if rng <= 3 else "phaser_med", v))
    # ELECTRONIC WARFARE, applied PER WEAPON FAMILY. sfb_command never imported
    # sfb_ew, so every estimate on the ORDER-EMITTING path assumed an unjammed
    # battlefield: the target's ECM was unknown and our ECCM pinned at zero, so
    # the net shift always came out +0. Against a side actually running ECM that
    # overstates our own fire and understates his - exactly backwards for
    # deciding whether to take an exchange.
    #
    # Per family, not on the total: at a +1 shift a phaser inside range 3 loses
    # 10% where a disruptor loses 22%, so lumping them would misprice every
    # mixed battery, which is every ship we field.
    try:
        import sfb_ew as EW
        t_ecm, _t_eccm = EW.ew_status(target)
        _m_ecm, m_eccm = EW.ew_status(attacker)
        shift = EW.net_shift(t_ecm, m_eccm)
        if shift > 0 and total > 0:
            degraded = sum(EW.degrade_expected(v, fam, shift) for fam, v in by_family)
            if total - degraded > 0.05:
                parts.append(f'EW -{total - degraded:.1f} (+{shift} shift: his '
                             f'ECM {t_ecm:g} vs our ECCM {m_eccm:g})')
                total = degraded
    except Exception:
        pass
    return total, parts


def phaser_cliff_note(ship, rng):
    """Warn when one more hex of range costs a disproportionate share of phaser
    damage — the Ph-1 range-5 wall and the Ph-2 range-3 wall."""
    w = ship.get("weapons", {})
    out = []
    for fam, cliffs in PHASER_CLIFFS.items():
        if not w.get(fam, [0, 0])[0]:
            continue
        for good, bad, text in cliffs:
            if rng == good:
                out.append(f"HOLD AT RANGE {good}: {text}")
            elif rng == bad:
                out.append(f"CLOSE TO {good}: {text}")
    return out


def absorb_capacity(ship, from_ship):
    """What this ship can soak from that bearing: facing shield + remaining hull."""
    s = facing_shield(ship, from_ship)
    sh = ship.get("shields", [0] * 6)
    hull = ship.get("hull", [0, 0])
    return (sh[s] if s < len(sh) else 0) + (hull[0] if hull else 0)


def commitment(ship, foe, rng):
    """Is this exchange worth taking? Compares expected damage BOTH ways, weighted by
    what each side can still absorb. Returns (verdict, detail)."""
    mine, mparts = expected_damage(ship, foe, rng)
    his, hparts = expected_damage(foe, ship, rng)
    my_soak = max(1, absorb_capacity(ship, foe))
    his_soak = max(1, absorb_capacity(foe, ship))
    # fraction of the opponent's remaining capacity each side removes per exchange
    my_bite = mine / his_soak
    his_bite = his / my_soak
    ratio = (my_bite / his_bite) if his_bite > 0 else 9.9
    detail = (f'we deal ~{mine:.0f} ({"+".join(mparts) or "-"}) vs his ~{his:.0f} '
              f'({"+".join(hparts) or "-"}); soak {my_soak} vs {his_soak}')

    # What the volley would actually KILL, from the client's real DAC (sfb_dac,
    # validated 8/8 against a measured volley). REPORTED, not folded into the
    # verdict: whether DAC-weighting improves the engage/avoid call is an open
    # question needing a side-by-side in a real game, and quietly changing the
    # decision on an untested model is exactly how the shield-cost error got in.
    # Raw expected damage still drives `ratio`.
    try:
        import sfb_dac as DAC
        idx = facing_shield(foe, ship)
        shield_left = (foe.get("shields") or [0] * 6)[idx]
        internals = int(mine) - max(0, shield_left)
        if internals > 0:
            kills = [(n, v) for n, v in DAC.summarise(foe, internals, top=3) if v >= 0.5]
            if kills:
                detail += "; likely kills " + ", ".join(f"{n} x{v:.1f}" for n, v in kills)
    except Exception:
        pass

    if ratio >= 1.3:
        return "COMMIT", f'FAVOURABLE trade x{ratio:.1f} - {detail}'
    if ratio >= 0.8:
        return "EVEN", f'even trade x{ratio:.1f} - only if you can afford it; {detail}'
    return "AVOID", f'BAD trade x{ratio:.1f} - do not exchange here; {detail}'


# ---------------------------------------------------------------- missions
# Roles inferred from hull type (escort assignments are not stored in the save).
CARRIER_TYPES = {"CV", "CVL", "CVE", "CVA", "CVS", "CVD", "SCS", "MCV", "NCV", "CVH"}
ESCORT_TYPES  = {"CLE", "EFF", "AFF", "DE", "DF", "E3", "E4", "E5", "CLA", "DWE"}
SCOUT_TYPES   = {"SF", "SC", "SCS", "NSC", "MSC", "SR"}
BASE_TYPES    = {"BATS", "SB", "BS", "MB", "FRD", "BATS+", "SBS"}
CAPITAL_TYPES = {"DN", "BC", "BCH", "CC", "CA", "DNH", "SCS"}


def _base_type(t):
    return re.sub(r"[+pP0-9]+$", "", (t or "").upper())


def ship_role(ship):
    t = _base_type(ship.get("type", ""))
    if t in BASE_TYPES:
        return "base"
    if t in CARRIER_TYPES or ship.get("systems", {}).get("fighter", [0])[0] >= 4:
        return "carrier"
    if t in ESCORT_TYPES:
        return "escort"
    if t in SCOUT_TYPES:
        return "scout"
    if t in CAPITAL_TYPES:
        return "capital"
    return "line"


def assess_mission(ship, friends, state):
    """Mission = what this ship is responsible for, independent of the combat posture.
    Returns (mission, detail, consort|None). Doctrine (Tactics Manual p22/p56):
    escorts sit on the FLANKS or REAR of the formation and spend their ADD/phasers
    defending the consort, not themselves."""
    role = ship_role(ship)
    me = (ship["x"], ship["y"])
    # a base/immobile friendly asset to guard?
    assets = [f for f in friends if ship_role(f) == "base" and f["label"] != ship["label"]]
    if role == "base":
        return "HOLD", "immobile - defend in place, all weapons to local defence", None
    if role == "scout":
        return ("EW SUPPORT",
                "stay out of the line - lend ECM/ECCM to the flagship, hold the sensor picture",
                None)
    if role == "escort":
        # doctrine: an escort belongs to the CARRIER; only fall back to a capital
        carriers = [f for f in friends
                    if f["label"] != ship["label"] and ship_role(f) == "carrier"]
        caps = [f for f in friends
                if f["label"] != ship["label"] and ship_role(f) == "capital"]
        cands = carriers or caps
        if cands:
            consort = min(cands, key=lambda f: H.hex_distance(me, (f["x"], f["y"])))
            d = H.hex_distance(me, (consort["x"], consort["y"]))
            return ("ESCORT",
                    f'screen {consort["label"]} (range {d}) - hold 1-2 hexes on his flank/rear, '
                    f'ADD+phasers reserved for seekers tracking HIM',
                    consort)
    if assets:
        asset = min(assets, key=lambda f: H.hex_distance(me, (f["x"], f["y"])))
        d = H.hex_distance(me, (asset["x"], asset["y"]))
        if d <= 15:
            return ("STATION-KEEP",
                    f'guard {asset["label"]} (range {d}) - stay inside 10 hexes, do not be drawn off',
                    asset)
    if role == "carrier":
        return "CARRIER", "stand off behind the screen; launch and recover, avoid the knife fight", None
    return "FREE", "no standing assignment - fight the ship", None


def consort_threats(consort, state):
    """Seekers tracking the ship we are escorting (what our ADD is FOR)."""
    if not consort:
        return []
    return incoming_seekers(consort, state)


# ---------------------------------------------------------------- posture
# Optimal engagement band per weapon family, and how it shifts against the
# threat we face (Tactics Manual: disruptors out-trade photons at 13-15).
BANDS = {
    "disruptor": (5, 8),      # overload band; vs photons -> pushed out (below)
    "photon":    (1, 8),      # Fed wants overload range
    "plasma":    (8, 15),
    "hellbore":  (8, 15),
    "esg":       (1, 3),
    "drone":     (10, 20),
    "phaser":    (1, 5),
}


def optimal_band(ship, enemy):
    fam = dominant_family(ship)
    lo, hi = BANDS.get(fam, (5, 8))
    ew = enemy.get("weapons", {}) if enemy else {}
    # vs photon-armed foe, disruptors out-trade at 13-15 and deny his overload (<=8)
    if fam == "disruptor" and ew.get("photon", [0])[0]:
        return 13, 15
    # vs a drone/plasma boat, don't loiter in the seeking-weapon envelope
    if fam in ("photon", "phaser") and (ew.get("drone", [0])[0] or ew.get("plasma", [0])[0]):
        return max(1, lo), min(hi, 8)
    return lo, hi


def choose_posture(ship, enemy, rng, log, seekers=None):
    """What is this ship trying to DO right now? Returns (posture, reason)."""
    # Seeking weapons trump everything: they kill you while you admire the geometry.
    if seekers:
        imminent = [s for s in seekers if s["turns_to_impact"] is not None and s["turns_to_impact"] <= 1.5]
        if imminent:
            total = sum(s["warhead"] for s in imminent)
            return "EVADE", (f'{len(imminent)} seeker(s) impact in <1.5 turns ({total} dmg) - '
                             f'point-defence and break tracking NOW')
        catchable = [s for s in seekers if s["turns_to_impact"] is not None]
        if catchable:
            return "EVADE", (f'{len(catchable)} seeker(s) tracking, closing '
                             f'{catchable[0]["closing"]}/turn - open range and thin them')
    sh, mx = ship.get("shields", []), ship.get("shields_max", [])
    frac = (sum(sh) / sum(mx)) if mx and sum(mx) else 1.0
    hull = ship.get("hull", [1, 1])
    hull_frac = (hull[0] / hull[1]) if hull and hull[1] else 1.0
    lo, hi = optimal_band(ship, enemy)

    # crippled -> break off
    if hull_frac < 0.35 or frac < 0.25:
        return "DISENGAGE", f"badly hurt (shields {int(frac*100)}%, hull {int(hull_frac*100)}%) - break off"
    # heavy weapons reloading and enemy inside his own good range -> open out
    disr = ship.get("weapons", {}).get("disruptor", [0])[0]
    if disr and log:
        fired = log.get("fired", {}).get(ship["label"], {}).get("disruptor", [])
        if fired and rng <= hi:
            ft, fi = max(fired, key=lambda ti: _abs_imp(*ti)) if isinstance(fired[0], (tuple, list)) else (log["turn"], max(fired))
            if _abs_imp(log["turn"], log["impulse"]) < _abs_imp(ft, fi) + 9:
                return "OPEN", "heavy weapons reloading - open range until they cycle"
    if rng > hi + 2:
        return "CLOSE", f"outside firing band {lo}-{hi} (rng {rng})"
    if rng > hi:
        return "STANDOFF", f"just outside band {lo}-{hi} - ease in, don't overshoot"
    if rng < lo:
        return "OPEN", f"inside band {lo}-{hi} - don't be dragged into his range"
    return "STANDOFF", f"in band {lo}-{hi} - hold range, work for the angle"


def weapon_cycle(ship, turn, impulse, log):
    """Weapon-timing prompts: disruptors can't hold across turns (E3.24) and obey the
    8-impulse reload delay (E1) - 8 impulses must ELAPSE, so a shot on 1.24 is ready 2.1."""
    prompts = []

    # Sensor damage is not only an EW question. Below a rating of 6 the lock-on
    # roll stops being automatic (D6.11), and a failure DOUBLES firing range to
    # every unlocked target, forbids launching seekers, releases those already
    # on the map, and disables tractors and transporters outright. That changes
    # what the ship can be ordered to do, so it belongs in the weapon prompts.
    try:
        import sfb_ew as _EW
        _lock = _EW.lock_on_advice(ship)
        if _lock:
            prompts.append(f"{_lock[0]} - {_lock[1][1]}")
    except Exception:
        pass

    w = ship.get("weapons", {})
    disr = w.get("disruptor", [0, 0])[0]
    if disr:
        fired = (log or {}).get("fired", {}).get(ship["label"], {}).get("disruptor", [])
        if fired:
            ft, fi = max(fired, key=lambda ti: _abs_imp(*ti)) if isinstance(fired[0], (tuple, list)) else (turn, max(fired))
            ready_abs = _abs_imp(ft, fi) + 8 + 1     # 8 impulses elapse; ready on the next
            rt, ri = _to_ti(ready_abs)
            if _abs_imp(turn, impulse) < ready_abs:
                prompts.append(f'DISRUPTORS: fired impulse {ft}.{fi}; 8-impulse reload (E1) - ready impulse {rt}.{ri}.')
            else:
                prompts.append(f'DISRUPTORS: reloaded (fired {ft}.{fi}) - available to fire.')
        else:
            if impulse >= 24:
                prompts.append(f'DISRUPTORS: armed rounds CANNOT hold to next turn (E3.24) - FIRE THIS TURN (by {turn}.{IPT}).')
            else:
                prompts.append(f'DISRUPTORS: hold for a shot; they cannot carry to next turn (E3.24), so must fire this turn.')
    return prompts


def order_for(ship, enemies, impulse, is_order, log=None, turn=1, state=None, friends=None):
    """Multi-line order/advice block for one ship."""
    tag = "ORDER" if is_order else "advise"
    head = f'[{tag}] {ship["label"]} ({ship["type"]}, spd {ship["speed"]}, face {FACING_VEC[ship["facing"]]})'
    if not enemies:
        return [head + ": no contacts - hold, charge weapons.",
                "    " + compute_eaf(ship, 8, False, False, fired_phasers=False)]
    me = (ship["x"], ship["y"])
    tgt = min(enemies, key=lambda e: H.hex_distance(me, (e["x"], e["y"])))
    rng = H.hex_distance(me, (tgt["x"], tgt["y"]))
    fam = dominant_family(ship)
    pref = {"disruptor": 8, "photon": 8, "plasma": 10, "hellbore": 15, "esg": 3, "drone": 12}.get(fam, 5)
    closing = rng > pref
    threatened = rng <= 15
    # enemy shield I face
    es = facing_shield(tgt, ship)
    es_val, es_max = tgt["shields"][es], tgt["shields_max"][es]
    es_down = es_val == 0 and es_max > 0
    # my shield presented
    ms = facing_shield(ship, tgt)
    ms_val = ship["shields"][ms]

    # POSTURE decides intent; movement/EAF follow from it
    seekers = incoming_seekers(ship, state) if state else []
    mission, mdetail, consort = assess_mission(ship, friends or [], state or {})
    cthreats = consort_threats(consort, state) if (state and consort) else []
    posture, why = choose_posture(ship, tgt, rng, log, seekers)
    lo, hi = optimal_band(ship, tgt)
    if posture == "CLOSE":
        want_speed = min(31, max(12, rng - hi + 8))
        mv = f'CLOSE to {lo}-{hi}; set speed {want_speed}.'
    elif posture == "OPEN":
        want_speed = min(31, max(16, rng + 8))
        mv = f'OPEN the range toward {lo}-{hi}; speed {want_speed}, turn away.'
    elif posture == "EVADE":
        fastest = max((s["speed"] for s in seekers), default=20)
        want_speed = min(31, max(fastest + 1, 20))
        mv = f'EVADE - speed {want_speed} (outpace seekers @{fastest}), turn away, ready point-defence.'
    elif posture == "DISENGAGE":
        want_speed = 31
        mv = f'DISENGAGE - max speed {want_speed}, present a fresh shield, break contact.'
    else:  # STANDOFF
        want_speed = max(8, min(20, (lo + hi) // 2 + 4))
        mv = f'HOLD band {lo}-{hi} (rng {rng}); speed {want_speed}, work for the firing angle.'

    # ---- SQUADRON COHERENCE: do not let one ship charge off alone.
    #
    # The posture layer sets each ship's speed from ITS OWN guns, with no notion
    # that a squadron should arrive together. Left alone it sends a frigate off
    # at 23 while the carrier it belongs to crawls at 8 - the frigate arrives
    # unsupported and is killed for nothing. p38 is explicit: "a fleet with a
    # unified plan will always beat an equal fleet of individual ships."
    #
    # If any friendly ship is TIED to a slower pace (a carrier operating fighters,
    # or its escort), the squadron advances at that pace unless this ship has a
    # detached mission of its own.
    if friends and mission in ("FREE", "LINE", None):
        try:
            import sfb_carrier as CAR
            pace = None
            for f in friends:
                if f["label"] == ship["label"]:
                    continue
                ft = _base_type((f.get("type") or "").upper())
                ff = (f.get("systems") or {}).get("fighter", [0, 0])[0]
                if ft in CARRIER_TYPES or ff >= 4:
                    lim, _ = CAR.carrier_speed_limit(f, fighters_out=None, state=state)
                    if lim is not None:
                        pace = lim if pace is None else min(pace, lim)
            if pace is not None and want_speed > pace:
                mv = (f"HOLD WITH THE SQUADRON - speed {pace}, not {want_speed}. The group "
                      f"is tied to the carrier's pace; running ahead arrives alone and "
                      f"unsupported (p38). Advance together.")
                want_speed = pace
        except Exception:
            pass

    # ---- An ESCORT must not outrun the ship it is escorting.
    # Same failure mode as the carrier below: the posture layer optimises for
    # THIS hull's guns, and would send a screen off at speed 27 while its charge
    # stands off at 8 - abandoning the ship it exists to protect. Doctrine is
    # explicit that escorts hold station on the flank/rear (p22), which is
    # impossible at a different speed.
    if mission == "ESCORT" and consort is not None:
        try:
            import sfb_carrier as CAR
            _ct = _base_type((consort.get("type") or "").upper())
            _cf = (consort.get("systems") or {}).get("fighter", [0, 0])[0]
            if _ct in CARRIER_TYPES or _cf >= 4:
                _lim, _n = CAR.carrier_speed_limit(consort, fighters_out=None, state=state)
            else:
                _lim = int(consort.get("speed") or 0) or None
            if _lim is not None and want_speed > _lim:
                mv = ("MATCH THE CARRIER - speed %d, not %d. Hold 1-2 hexes on %s's "
                      "flank/rear; a screen that outruns its charge is not screening "
                      "anything (p22)." % (_lim, want_speed, consort["label"]))
                want_speed = _lim
        except Exception:
            pass

    # ---- A CARRIER does not charge, and must not outrun its own group.
    # The posture layer optimises for THIS hull's guns, which for a carrier is
    # the wrong objective: its weapon is the fighter group. Two corrections,
    # both of which the posture layer cannot see.
    try:
        import sfb_carrier as CAR
        _t = _base_type((ship.get("type") or "").upper())
        _ftr = (ship.get("systems") or {}).get("fighter", [0, 0])[0]
        if _t in CARRIER_TYPES or _ftr >= 4:
            cap, note = CAR.carrier_speed_limit(ship, fighters_out=None, state=state)
            if cap is not None and want_speed > cap:
                mv = (f'STAND OFF - do NOT close to {lo}-{hi}. Speed {cap}, not {want_speed}: '
                      f'{note} Keep the screen between you and him and fight through the group.')
                want_speed = cap
            elif mission == "CARRIER" and posture == "CLOSE":
                want_speed = min(want_speed, max(8, rng - hi))
                mv = (f'STAND OFF at {hi}+ rather than closing to {lo}-{hi}; speed '
                      f'{want_speed}. The fighters make the attack, not the hull (p39).')
    except Exception:
        pass
    lines = [f'{head} vs {tgt["label"]} @ rng {rng} (his {SHIELD[es]} facing):']
    lines.append(f'    MISSION: {mission} - {mdetail}')
    verdict, tdetail = commitment(ship, tgt, rng)
    lines.append(f'    POSTURE: {posture} - {why}')
    lines.append(f'    TRADE: {verdict} - {tdetail}')
    # EAF (energy allocation)
    # The capacitor is full at scenario start and stays full until you fire
    # (H6.1), so only a ship that HAS fired phasers this turn may allocate.
    _pf = False
    try:
        _hist = ((log or {}).get("fired") or {}).get(ship["label"], {})
        _pf = any("phaser" in k and any(t == turn for t, _i in v)
                  for k, v in _hist.items())
    except Exception:
        pass
    lines.append("    " + compute_eaf(ship, want_speed, closing and rng <= 10,
                                      threatened, fired_phasers=_pf,
                                      enemies=enemies, rng=rng))
    if ms_val == 0:
        mv += f' WARN our {SHIELD[ms]} is DOWN toward him - turn to a fresh shield.'
    if mission == 'ESCORT' and consort:
        cd = H.hex_distance((ship['x'], ship['y']), (consort['x'], consort['y']))
        if cd > 2:
            mv = (f"REJOIN {consort['label']} (range {cd}) - close to 1-2 hexes on his "
                  f"flank/rear before anything else.")
        else:
            mv += f" Stay on {consort['label']}'s flank/rear (range {cd})."
    elif mission == 'STATION-KEEP' and consort:
        cd = H.hex_distance((ship['x'], ship['y']), (consort['x'], consort['y']))
        if cd > 10:
            mv = f"RETURN to {consort['label']} (range {cd}) - you are being drawn off station."
    elif mission == 'HOLD':
        mv = 'HOLD position (immobile) - all weapons to local defence.'
    lines.append("    MOVE: " + mv + "  " + impulse_note(ship, rng, impulse))

    # ---- THE ACTUAL ORDERS FOR THIS IMPULSE.
    # Everything above is intent. These are the things you physically do in the
    # client this impulse: move, fire, launch fighters, launch drones, raise the
    # ESG. Movement is only one of them.
    _acts = []
    try:
        import sfb_move as MV
        _tm = turn_mode_of(ship)[0]
        _moves = R.moves_this_impulse(int(ship.get("speed") or 0), impulse) if 1 <= impulse <= 32 else False
        if mission == "ESCORT" and consort is not None:
            _o = MV.station_keeping_order(ship, consort, log, _tm, _moves)
        else:
            _o = MV.move_order(ship, (tgt["x"], tgt["y"]), log, _tm, _moves)
        if _o:
            _acts.append(_o)
            ship["advised_move"] = _shadow_move_of(_o[0])
    except Exception as _e:
        _acts.append((f"(movement order unavailable: {_e})", []))
    try:
        import sfb_actions as ACT
        _acts += ACT.impulse_actions(ship, enemies, tgt, rng, impulse, turn,
                                     log, state, sys.modules[__name__])
    except Exception as _e:
        _acts.append((f"(action list unavailable: {_e})", []))
    for _i, (_head, _why) in enumerate(_acts):
        lines.append(f"    >>> IMPULSE {impulse}: {_head}" if _i == 0
                     else f"    >>> {_head}")
        for _r in _why:
            lines.append(f"          {_r}")
    lines.append("    " + maneuver_note(ship, log))
    sd = seeker_defence(ship, seekers, state=state, turn=turn)
    if sd:
        lines.append("    " + sd)
    if cthreats:
        lines.append(f"    SCREEN: {len(cthreats)} seeker(s) tracking {consort['label']} "
                     f"(nearest {cthreats[0]['range']} hex) - spend ADD/phasers on THESE, not your own.")
    # ---- hunt a down shield he has ROTATED AWAY. Mizia only triggers when the
    # facing shield is down; a smart opponent turns the hole away (live example:
    # KHS FF 9's #2 at 0, turned to B so the Lyrans bear on his fresh #1). The
    # counter is geometry, not fire: swing the bearing one sextant toward the
    # hole and shoot when it opens - and say WHICH way to swing.
    if not es_down and rng <= 15:
        holes = [i for i, (v, m) in enumerate(zip(tgt.get("shields") or [],
                                                  tgt.get("shields_max") or []))
                 if m > 0 and v == 0]
        if holes:
            best = min(holes, key=lambda i: min((i - es) % 6, (es - i) % 6))
            cw = (best - es) % 6
            ccw = (es - best) % 6
            side = ("CLOCKWISE" if cw <= ccw else "COUNTER-CLOCKWISE")
            steps = min(cw, ccw)
            ship["hunt"] = (tgt["label"], best)      # board draws this order
            lines.append(f'    HUNT: his {SHIELD[best]} is DOWN but rotated away - '
                         f'you bear on his {SHIELD[es]} ({es_val}/{es_max}). Swing '
                         f'the bearing {steps} sextant(s) {side} (keep crossing his '
                         f'{"bow" if best in (0,1,5) else "stern"} side) and fire '
                         f'the moment the hole faces you; every impulse he spends '
                         f'counter-rotating is an impulse he is not closing.')
    # fire / weapons
    if es_down and rng <= 15:
        lines.append(f'    FIRE: his {SHIELD[es]} is DOWN - concentrate {fam} + phasers (Mizia).')
    elif rng <= pref:
        ol = " OVERLOADED" if rng <= 8 and fam in ("disruptor", "photon", "fusion", "hellbore") else ""
        extra = ""
        if ship.get("weapons", {}).get("drone", [0])[0] and fam != "drone":
            extra = f' + launch drones (x{ship["weapons"]["drone"][0]} racks).'
        lines.append(f'    FIRE: {fam}{ol} at his {SHIELD[es]} ({es_val}/{es_max}); energize phasers.{extra}')
    elif fam == "drone" and rng <= 15:
        lines.append(f'    FIRE: launch drones now (x{ship["weapons"]["drone"][0]}) - saturate before closing.')
    else:
        lines.append(f'    FIRE: hold until range {pref}.')
    # ---- wild weasel: worth charging, or voided by our own ESG?
    for ln in weasel_advice(ship, enemies, state):
        lines.append("    " + ln)

    # ---- shields: which one faces him, and is reinforcement worth buying?
    for ln in shield_advice(ship, enemies, rng, surplus_hint=None):
        lines.append("    " + ln)

    for wc in weapon_cycle(ship, turn, impulse, log):
        lines.append("    " + wc)

    # ---- manoeuvre options beyond a plain turn: HET / TAC / Erratic Manoeuvres
    for ln in het_advice(ship, enemies, rng, impulse, state):
        lines.append("    " + ln)
    # ---- carrier and PF operations
    for ln in carrier_advice(ship, enemies, rng, impulse, state=state):
        lines.append("    " + ln)
    return lines


# --------------------------------------------------------------------------
# Manoeuvre advice (HET / TAC / EM).
#
# Raised only when there is an actual REASON, because the manual is emphatic
# that needing an HET usually means you are already losing (p5, Tom Carroll:
# "If you put yourself into a position where you must HET, Weasel, or Emer
# Decel, you more than likely are losing the battle"). p36 allows exactly three
# sound reasons; we test for two of them mechanically and always lead with the
# no-risk TAC substitute (p45).
# --------------------------------------------------------------------------
def het_advice(ship, enemies, rng, impulse, state):
    out = []
    try:
        import sfb_maneuver as MAN
    except Exception:
        return out
    if not enemies:
        return out

    facing_down = False
    try:
        near = min(enemies, key=lambda e: H.hex_distance((ship["x"], ship["y"]),
                                                         (e["x"], e["y"])))
        f = facing_shield(ship, near)
        sh = ship.get("shields") or [1] * 6
        facing_down = sh[f] <= 0
    except Exception:
        pass
    seekers = []
    try:
        seekers = incoming_seekers(ship, state) if state else []
    except Exception:
        pass
    my_speed = int(ship.get("speed") or 0)
    chased = any(int(s.get("speed") or 0) > my_speed for s in seekers)

    reason = ("escape" if chased else
              "shield" if (facing_down and rng <= 8) else None)
    if reason is None:
        return out

    out.append(f"HET REASON ({reason}): {MAN.HET_REASONS[reason]}")
    for pri, txt in MAN.maneuver_advice(ship, enemy_has_seekers=bool(seekers),
                                        impulse=impulse, considering="het"):
        if pri <= 1:
            out.append("HET " + txt)
    return out


# --------------------------------------------------------------------------
# Carrier and PF operations - launch timing above all.
# --------------------------------------------------------------------------
def carrier_advice(ship, enemies, rng, impulse, state=None):
    out = []
    try:
        import sfb_carrier as CAR
    except Exception:
        return out
    t = _base_type((ship.get("type") or "").upper())
    if t == "PF":
        return [txt for pri, txt in CAR.pf_notes(ship)][:4]
    ftr = (ship.get("systems") or {}).get("fighter", [0, 0])
    if not (t in CARRIER_TYPES or ftr[0]) or not enemies:
        return out

    # The group to schedule is what is STILL ABOARD, not the number of fighter
    # boxes. Using boxes made this panel print a full 12-fighter launch schedule
    # while the order line above it said "deck clear" - the two panels flatly
    # contradicting each other because only one of them could see the board.
    group, aboard_known = ftr[0] or 6, False
    airborne = 0
    if state is not None:
        try:
            import sfb_airborne as AB
            rem, _cap = AB.fighters_aboard(state, ship)
            airborne = len(AB.fighters_out(state, ship))
            group, aboard_known = rem, True
        except Exception:
            pass
    if aboard_known and group <= 0:
        return [f"FIGHTERS group is up - {airborne} airborne, deck clear. "
                f"Nothing further to launch."]

    fastest = max(int(e.get("speed") or 0) for e in enemies)
    closing = max(1, int(ship.get("speed") or 0) + fastest)
    for pri, txt in CAR.launch_advice(ship, rng, closing, impulse,
                                      group_size=group, bays=1,
                                      has_armed_fighters=bool(group)):
        if pri <= 2:
            out.append("FIGHTERS " + txt)
    # Explicit impulse-by-impulse launch order, not just "launch now".
    for ln in CAR.launch_schedule(ship, group, bays=1, start_impulse=max(1, impulse)):
        out.append("FIGHTERS " + ln)
    if rng <= 8:
        for pri, txt in CAR.recovery_advice(ship, rng, impulse):
            if pri <= 1:
                out.append("FIGHTERS " + txt)
    return out


FLAG_RANK = {"DN": 9, "BB": 9, "BC": 8, "BCH": 8, "CC": 8, "SCS": 8, "CVA": 8,
             "CA": 7, "CV": 7, "CVL": 6, "CVE": 6, "CW": 6, "CL": 5, "CS": 6,
             "DD": 4, "DDF": 4, "FF": 2, "EFF": 2, "PF": 1, "SC": 3}


def flagship(ships):
    """The friendly flagship: the ship a squadron would actually be led from.

    Ranked by hull class first (a DN leads even if damaged), then by available
    power, then size class. Bases are excluded - a fleet is not led from a base.
    """
    cands = [s for s in ships if not _is_base(s)] or list(ships)
    if not cands:
        return None

    def score(s):
        t = _base_type((s.get("type") or "").upper())
        return (FLAG_RANK.get(t, 0),
                (s.get("power") or {}).get("total", 0),
                -(s.get("size_class") or 9))
    return max(cands, key=score)


def bridge_brief(side, my_ships, enemies, is_order):
    """A short in-character officer brief for one side, from the tactical picture.
    Templated (works with no API key); an LLM pass can voice it richer later."""
    b = BRIDGE.get(side.upper(), DEFAULT_BRIDGE)
    if not my_ships or not enemies:
        return [f'{b["sci"]}: No contacts on the scope, {b["addr"]}. Holding station.']
    # flagship = the biggest of my ships; nearest enemy overall
    flag = max(my_ships, key=lambda s: s["power"]["total"])
    nearest = min(enemies, key=lambda e: min(H.hex_distance((s["x"], s["y"]), (e["x"], e["y"])) for s in my_ships))
    rng = min(H.hex_distance((s["x"], s["y"]), (nearest["x"], nearest["y"])) for s in my_ships)
    es = facing_shield(nearest, flag)
    lines = []
    lines.append(f'{b["sci"]}: {b["addr"]}, {nearest["label"]} bears at {rng} hexes, closing. '
                 f'His {SHIELD[es]} shield faces the {b["ship"]}.')
    if rng > 12:
        lines.append(f'{b["helm"]}: Long range yet. Recommend we close under power to bring the guns to bear.')
    elif rng > 8:
        lines.append(f'{b["weps"]}: Approaching firing range, {b["addr"]}. Hold for the overload window at 8 hexes.')
    else:
        lines.append(f'{b["weps"]}: In range! Weapons overloaded and hot - give the word, {b["addr"]}.')
    verb = "Orders stand" if is_order else "Your orders"
    lines.append(f'-- {verb}, {b["addr"]}: {b["style"]}. --')
    return lines


BASE_TYPES = {"BS", "BATS", "SB", "MB", "BATS+", "SBS", "FRD"}


def _is_base(s):
    """A base/fixed asset: never moves, so it anchors the side defending it."""
    t = (s.get("type") or "").upper().replace("-", "")
    return t in BASE_TYPES or s.get("move_cost", "") in ("0", "-") or (
        _base_type(t) in BASE_TYPES)


def _is_crippled(s):
    """Formal CRIPPLED status per S2.41 - two very different thresholds than the
    old code used, which had conflated 'crippled' with 'slowed'.

    S2.41: a ship is crippled when EITHER
      A: 10% OR LESS of its original warp boxes remain undestroyed, OR
      B: 50% OR MORE of its interior boxes are destroyed.
    The old test used 50% warp for (A), which is the 'cannot keep fleet speed'
    threshold, NOT crippled - it wrongly marked a lightly-damaged ship crippled
    and inflated the C7.22 uncrippled-enemy count. NOTE: the save exposes current
    undestroyed warp, not ORIGINAL, so (A) uses warp_max as the best available
    proxy - flagged in DEFERRED; exact only for undamaged-warp ships.
    """
    hull, hmax = (s.get("hull") or [0, 0])[:2]
    if hmax and hull <= hmax * 0.5:          # B: >=50% interior destroyed
        return True
    pw = s.get("power") or {}
    warp = pw.get("warp", 0)
    wmax = pw.get("warp_max")
    return bool(wmax) and warp <= wmax * 0.1  # A: <=10% warp (S2.41, not 50%)


def _cannot_keep_fleet_speed(s):
    """A ship down to ~half its warp cannot match a healthy fleet's speed - a
    DOCTRINAL concern (Tactics Manual), distinct from S2.41 crippled status. This
    is the 50%-warp threshold the old _is_crippled used; it belongs here.
    """
    pw = s.get("power") or {}
    warp, wmax = pw.get("warp", 0), pw.get("warp_max")
    return bool(wmax) and warp <= wmax * 0.5


def situation_ctx(mine, foes, state, imp, scenario="campaign", objective=None, terrain=None):
    """Infer the situational-doctrine context from the live board.

    Deliberately conservative: only asserts a condition the board actually shows.
    Terrain is not represented in the autosave, so it must be passed in.
    """
    if not mine:
        return {}
    race = (mine[0].get("race") or "").upper()
    my_ships = [s for s in mine if not _is_base(s)]
    foe_ships = [s for s in foes if not _is_base(s)]

    # Do we have a fixed asset on the board that we must defend?
    defending = None
    for s in mine:
        if _is_base(s):
            t = (s.get("type") or "").upper()
            defending = "frd" if "FRD" in t else "base"
            break
    # Is the ENEMY defending a fixed asset (i.e. we are assaulting one)?
    attacking = None
    for s in foes:
        if _is_base(s):
            attacking = "base"
            break

    # Does any enemy present a DOWN shield to one of our ships? (Mizia prerequisite)
    shield_down = False
    for m in my_ships:
        for f in foe_ships:
            try:
                face = facing_shield(f, m)
                if (f.get("shields") or [1] * 6)[face] <= 0:
                    shield_down = True
                    break
            except Exception:
                continue
        if shield_down:
            break

    seekers_in = 0
    for m in my_ships:
        try:
            seekers_in += len(incoming_seekers(m, state))
        except Exception:
            pass

    return {
        "race": race,
        "scenario": scenario,
        "objective": objective,
        "terrain": terrain,
        "impulse": imp,
        "fleet_size": len(my_ships),
        "outnumbered": len(my_ships) < len(foe_ships),
        "our_crippled": any(_is_crippled(s) for s in my_ships),
        "shield_down": shield_down,
        "seekers_in": seekers_in,
        "defending": defending,
        "attacking": attacking,
    }


SCOUT_TYPES = {"SC", "SCS", "EWS", "CVS"}
PF_TYPES = {"PF", "GDN", "INT", "SHUTTLE"}


def disengage_note(ship, foes, state, seekers=None):
    """Is disengagement actually LEGAL for this ship right now? (C7.0)

    The engine previously treated DISENGAGE as a posture it could simply choose.
    C7.0 makes it a gated action, so we test the real conditions and report what
    blocks it rather than advising an illegal move.
    """
    me = (ship["x"], ship["y"])
    ships = [f for f in foes if not _is_base(f)]

    def nearest(pred):
        d = [H.hex_distance(me, (f["x"], f["y"])) for f in ships if pred(f)]
        return min(d) if d else None

    def _t(f):
        return (f.get("type") or "").upper()

    rng_ship = nearest(lambda f: True)
    rng_scout = nearest(lambda f: _t(f) in SCOUT_TYPES)
    rng_pf = nearest(lambda f: _t(f) in PF_TYPES)

    # A seeker can catch us if its speed exceeds ours and it is still tracking.
    seekers = seekers if seekers is not None else incoming_seekers(ship, state)
    my_speed = int(ship.get("speed") or 0)
    can_catch = any(int(s.get("speed") or 0) > my_speed for s in (seekers or []))

    pw = ship.get("power") or {}
    warp, warp_max = pw.get("warp", 0), pw.get("warp_max") or pw.get("warp", 0)
    frac = (warp / warp_max) if warp_max else None
    warpless = not warp_max

    avail, blocked = SIT.disengage_check(
        rng_nearest_ship=rng_ship, rng_nearest_scout=rng_scout, rng_nearest_pf=rng_pf,
        seekers_can_catch=can_catch, is_base=_is_base(ship), terrain_veto=False,
        warp_fraction=frac, warpless=warpless,
        warp_boxes=warp, warp_max_boxes=warp_max,
        uncrippled_enemies_within_15=sum(
            1 for f in ships
            if H.hex_distance(me, (f["x"], f["y"])) <= 15 and not _is_crippled(f)))

    out = []
    if avail:
        out.append("DISENGAGE: " + avail[0][1])
        for m, txt in avail[1:]:
            out.append(f"DISENGAGE   also {m}: {txt}")
    else:
        out.append("DISENGAGE: NOT LEGAL right now — " +
                   " ".join(t for _, t in blocked[:2]))
    return out


def doctrine_lines(mine, foes, state, imp, is_order, scenario="campaign",
                   objective=None, terrain=None, limit=6):
    """Render the applicable Tactics-Manual doctrine for this position.

    Only priority 1-2 items are shown by default — the console is a live
    command aid, not a textbook. Everything cites its page.
    """
    ctx = situation_ctx(mine, foes, state, imp, scenario, objective, terrain)
    if not ctx:
        return []
    picked = [(p, tag, txt) for p, tag, txt in SIT.situations(ctx) if p <= 2][:limit]
    if not picked:
        return []
    out = ["--- DOCTRINE ---" if is_order else "--- DOCTRINE (advisory) ---"]
    for _, tag, txt in picked:
        out.append(f"DOCTRINE {tag}: {txt}")
    return out


def weapons_history(ship, log, turn, impulse):
    """What this ship has actually fired, and what that implies is ready.

    Reads the combat log rather than guessing: a disruptor fired at 1.24 cannot
    fire again until 2.1 (E1.50's universal 8-impulse reload), and disruptors
    cannot hold a charge across the turn break at all (E3.24).
    """
    out = []
    fired = ((log or {}).get("fired") or {}).get(ship["label"], {})
    if not fired:
        return ["no weapons fired yet this battle"]
    now = _abs_imp(turn, impulse)
    for weap, stamps in sorted(fired.items()):
        last_t, last_i = stamps[-1]
        since = now - _abs_imp(last_t, last_i)
        ready = since >= UNIVERSAL_RELOAD_IMPULSES
        when = f"T{last_t}.{last_i}"
        state = ("READY" if ready
                 else f"reloading, ready in {UNIVERSAL_RELOAD_IMPULSES - since} imp")
        out.append(f"{weap} x{len(stamps)} (last {when}) - {state}")
    return out


def threat_assessment(enemy, my_ships, state, log, turn, impulse):
    """What this ENEMY ship can do to us. No orders - we do not command it.

    Deliberately different from our own ships' briefing: mission/posture/trade are
    our decisions, not observations about him. What matters here is what he is
    capable of, what he has spent, and who of ours is in danger.
    """
    out = []
    if not my_ships:
        return out
    # Who of ours is he closest to, and is he closing?
    epos = (enemy["x"], enemy["y"])
    nearest = min(my_ships, key=lambda s: H.hex_distance(epos, (s["x"], s["y"])))
    rng = H.hex_distance(epos, (nearest["x"], nearest["y"]))
    bearing_to_us = H.relative_sextant(epos, (nearest["x"], nearest["y"]),
                                       enemy.get("facing", 0))
    aimed = bearing_to_us in (0, 1, 5)        # we sit in his forward arc
    out.append(f"nearest target {nearest['label']} at range {rng}; "
               f"we are {'IN his forward arc' if aimed else 'off his bow'}"
               f" (speed {enemy.get('speed', 0)})")

    # What can he actually land on that ship right now?
    try:
        dmg, parts = expected_damage(enemy, nearest, rng)
        if dmg > 0:
            soak = absorb_capacity(nearest, enemy)
            out.append(f"THREAT if he fires now: ~{dmg:.0f} damage "
                       f"({', '.join(parts)}) vs {nearest['label']} soak {soak}")
        else:
            band = optimal_band(enemy, nearest)
            out.append(f"THREAT none at this range; he wants band {band[0]}-{band[1]}")
    except Exception:
        pass

    # Maneuver state - can he turn, has he just committed to a heading?
    try:
        out.append(maneuver_note(enemy, log).replace("MANEUVER:", "his maneuver:"))
    except Exception:
        pass

    # Weapons spent / ready
    for w in weapons_history(enemy, log, turn, impulse):
        out.append(f"his weapons: {w}")

    # Seekers he has in flight
    try:
        sk = incoming_seekers(nearest, state)
        if sk:
            out.append(f"WARN {len(sk)} seeker(s) of his tracking {nearest['label']}")
    except Exception:
        pass
    return out


def fleet_plan(side, mine, foes, state, log, turn, impulse):
    """One overall plan for the side this turn - what the squadron is trying to do.

    Individual ship orders answer 'what do I do'; this answers 'what are we all
    doing', which is the thing the Tactics Manual insists actually wins fleet
    actions (p38: a fleet with a unified plan beats an equal fleet of individuals).
    """
    out = []
    if not mine or not foes:
        return out
    flag = flagship(mine)
    n_us, n_them = len(mine), len(foes)

    # --- the shape of the engagement
    dists = [H.hex_distance((m["x"], m["y"]), (f["x"], f["y"]))
             for m in mine for f in foes]
    closest, avg = min(dists), sum(dists) / len(dists)
    out.append(f"ENGAGEMENT: {n_us} v {n_them}; closest contact {closest} hexes, "
               f"average {avg:.0f}. Flagship {flag['label'] if flag else '-'}.")

    # --- concentration: pick ONE target for the squadron (p38 attrition procedure)
    def value(f):
        t = _base_type((f.get("type") or "").upper())
        rank = FLAG_RANK.get(t, 3)
        d = min(H.hex_distance((m["x"], m["y"]), (f["x"], f["y"])) for m in mine)
        sh = f.get("shields") or [1] * 6
        down = sum(1 for v in sh if v <= 0)
        return rank + down * 3 - d * 0.25      # prefer big, damaged, close
    target = max(foes, key=value)
    tdist = min(H.hex_distance((m["x"], m["y"]), (target["x"], target["y"])) for m in mine)
    down = [f"#{i+1}" for i, v in enumerate(target.get("shields") or []) if v <= 0]
    out.append(f"CONCENTRATE on {target['label']} ({target.get('type','?')}) at "
               f"{tdist} hexes"
               + (f" - his {','.join(down)} already down" if down else "")
               + ". Break one shield, then Mizia volleys to strip weapons (p38).")

    # --- range intent, from what our own guns want
    bands = [optimal_band(m, target) for m in mine]
    lo = max(b[0] for b in bands)
    hi = min(b[1] for b in bands)
    if lo > hi:
        lo, hi = min(b[0] for b in bands), max(b[1] for b in bands)
        out.append(f"RANGE: mixed batteries - no common band; fight at {lo}-{hi} and "
                   f"accept some ships are out of their best range.")
    else:
        out.append(f"RANGE: hold {lo}-{hi} - the band that suits the whole squadron.")

    # --- roles / screening
    roles = []
    for m in mine:
        r = ship_role(m)
        if r:
            roles.append(f"{m['label']}={r}")
    if roles:
        out.append("ROLES: " + ", ".join(roles))

    # --- doctrine, situation-aware
    try:
        ctx = situation_ctx(mine, foes, state, impulse)
        for pri, tag, txt in SIT.situations(ctx):
            if pri <= 1:
                out.append(f"{tag}: {txt}")
    except Exception:
        pass
    return out


def _flight_lines(state, side, enemies, turn, imp, log=None):
    """Flight orders for a side's airborne fighters, as order lines.

    By FLIGHT, not by airframe: nine fighters given nine blocks would bury the
    ship orders they exist to support. Individual craft appear only where they
    genuinely differ from their flight.
    """
    out = []
    try:
        import sfb_flight as FL
    except Exception:
        return out
    for mname, head, why, exc in FL.all_flight_orders(state, enemies, turn, imp,
                                                      side=side, log=log):
        # Tag each flight with its home carrier so the per-ship view can file it
        # under that carrier instead of the preceding ship's block.
        out.append(f"[FLIGHT] {mname or '?'}: {head}")
        for w in why:
            out.append("          " + w)
        for e in exc:
            out.append("          EXCEPTION " + e)
    return out


def build_commands(state, ai_side, advise_side):
    ships = state["ships"]
    ai = [s for s in ships if s["race"].upper() == ai_side.upper()]
    adv = [s for s in ships if s["race"].upper() == advise_side.upper()]
    ai_enemies = adv
    adv_enemies = ai
    # combat log gives the ACCURATE live turn/impulse + what actually happened
    log = None
    try:
        log = sfb_log.parse()
        # The client appends every game to one log file. Restrict it to ships
        # actually on the board so a finished battle cannot leak into this one.
        # Annotate each ship with the ESG activations it has made, BEFORE the
        # restrict step. The log names ships in ABBREVIATED form ('CW Marauder')
        # while the save uses the full label ('CW 705 Marauder'), so match by
        # word-subset and read from the unrestricted events (restrict would drop
        # the abbreviated names outright).
        for s in ships:
            s["esg_fires"] = []
        _labels = [s["label"] for s in ships]
        _by_label = {s["label"]: s for s in ships}
        for ev in (log.get("events") or []):
            if ev.get("kind") != "esg_fire":
                continue
            lab = sfb_log.canonical_label(ev.get("ship", ""), _labels)
            if lab is not None:
                _by_label[lab]["esg_fires"].append(ev)
        log = sfb_log.restrict_to_ships(log, [s["label"] for s in ships])
    except Exception:
        pass
    turn = log["turn"] if log else state["turn"]
    imp = log["impulse"] if log else state.get("impulse", 1)

    lines = [f'=== Turn {turn}, Impulse {imp} ===']
    if log:
        feed = sfb_log.recent_combat(log)
        if feed:
            lines.append("--- COMBAT THIS TURN ---")
            for cl in feed[-8:]:
                lines.append("* " + cl)
    lines.append(f'--- {ai_side} ORDERS (execute these) ---')
    for bl in bridge_brief(ai_side, ai, adv, is_order=True):
        lines.append("~ " + bl)
    for s in sorted(ai, key=lambda z: H.hex_distance((z["x"], z["y"]), (adv[0]["x"], adv[0]["y"])) if adv else 0):
        lines.extend(order_for(s, ai_enemies, imp, is_order=True, log=log, turn=turn, state=state, friends=ai))
    lines.extend(_flight_lines(state, ai_side, ai_enemies, turn, imp, log=log))
    lines.extend(doctrine_lines(ai, adv, state, imp, is_order=True))
    lines.append(f'--- {advise_side} ADVICE (your call) ---')
    for bl in bridge_brief(advise_side, adv, ai, is_order=False):
        lines.append("~ " + bl)
    for s in sorted(adv, key=lambda z: H.hex_distance((z["x"], z["y"]), (ai[0]["x"], ai[0]["y"])) if ai else 0):
        lines.extend(order_for(s, adv_enemies, imp, is_order=False, log=log, turn=turn, state=state, friends=adv))
    lines.extend(_flight_lines(state, advise_side, adv_enemies, turn, imp, log=log))
    lines.extend(doctrine_lines(adv, ai, state, imp, is_order=False))
    return lines


# Flagship priority: what actually decides battles, highest first. Scored by
# marker so the summary is derived from the SAME lines the ship tabs show -
# one source of truth, two densities.
_FLAG_SCORES = (
    (100, ("FIRE THIS TURN", "CANNOT hold", "use-or-lose", "USE OR LOSE")),
    (90,  ("OUTCOME:", "leak", "INTERNALS")),
    (80,  ("HUNT:", "is DOWN - concentrate")),
    (70,  ("RELEASE ESG", "ESG NOW", "ANNOUNCE")),
    (60,  ("SCREEN:", "seeker(s) tracking", "WEASEL: charge")),
    (50,  ("FIRE:",)),
    (40,  ("LAUNCH", "SCATTER-PACK", "suicide shuttle - ", "ARM SUICIDE")),
    (30,  (">>> IMPULSE",)),
)


def flagship_summary(lines, limit=7):
    """The handful of decisions that matter RIGHT NOW, ranked, one line each.

    Derived from build_commands' own output: every candidate is an existing
    order/advice line, attributed to its ship and side, scored by marker. The
    ship tabs stay the drill-down; this is the glance.
    """
    side = ship = None
    scored = []
    for ln in lines:
        s = ln.strip()
        m = re.match(r"^--- (\w+) (ORDERS|ADVICE)", s)
        if m:
            side, ship = m.group(1), None
            continue
        m = re.match(r"^(\S.*?) vs .* @ rng (\d+)", ln)
        if m:
            ship = m.group(1)
            continue
        if not ln.startswith(" ") or ship is None:
            continue
        for score, keys in _FLAG_SCORES:
            if any(k in s for k in keys):
                txt = s
                if txt.startswith(">>> "):
                    txt = txt[4:]
                # strong lines only - drop the rationale bullet-cloud
                if len(txt) > 110:
                    txt = txt[:107] + "..."
                scored.append((score, side, ship, txt))
                break
    scored.sort(key=lambda x: -x[0])
    out, seen = [], set()
    for score, sd, sh, txt in scored:
        if len(out) >= limit:
            break
        key = (sh, txt[:40])
        if key in seen:
            continue
        seen.add(key)
        out.append((sd, sh, txt))
    return out


def main():
    ap = argparse.ArgumentParser(description="SFB human-as-hands command engine")
    ap.add_argument("--ai", default="Kzinti", help="side the AI plays (issues orders)")
    ap.add_argument("--advise", default="Lyran", help="side the human plays (gets advice)")
    ap.add_argument("--state", default=AUTOSAVE, help="tactical save to read")
    ap.add_argument("--post", action="store_true", help="post to game chat")
    ap.add_argument("--room", default="#SFB_Game1")
    args = ap.parse_args()

    state = dump_state(args.state)
    lines = build_commands(state, args.ai, args.advise)
    print("\n".join(lines))

    if args.post:
        from sfb_client import SFBGameClient
        c = SFBGameClient("127.0.0.1", 6668, "FleetCommand", args.room, verbose=False)
        c.start(os.environ.get("SFB_PASSWORD", "relaydummy"))
        time.sleep(2.0)
        try:
            for ln in lines:
                c.conn.say(args.room, ln)
                time.sleep(0.4)
        finally:
            c.conn.disconnect()
        print("\n[posted to chat]")


if __name__ == "__main__":
    main()


# --------------------------------------------------------------------------
# Shield facing and reinforcement.
#
# Two rules drive this, and they point in opposite directions:
#   D3.342 SPECIFIC reinforcement - 1 point of energy = 1 extra box on ONE
#          shield, "for the duration of the current turn".
#   D3.341 GENERAL reinforcement  - energy is HALVED, protects every shield,
#          but is wiped by the first damage from ANY direction.
# Specific is twice as efficient and cannot be stripped by a scratch elsewhere,
# so it is the default. General is only worth buying when you genuinely do not
# know which facing will be hit.
#
# The sting is "duration of the current turn": reinforcement bought on a turn
# where nobody can reach you is simply thrown away.
# --------------------------------------------------------------------------
SPECIFIC_REINFORCE_RATE = 1.0     # D3.342: 1 energy = 1 box
GENERAL_REINFORCE_RATE = 0.5      # D3.341: energy / 2, rounded down


def _project(ship, hexes):
    """Where this ship will be after moving `hexes` straight along its facing."""
    pos = (ship.get("x", 0), ship.get("y", 0))
    f = ship.get("facing", 0)
    for _ in range(max(0, int(hexes))):
        pos = H.forward_hex(pos, f)
    return pos


def closing_rate(ship, enemies):
    """Hexes per TURN the gap is ACTUALLY shrinking, from real headings.

    The old version summed the two speeds - the head-on assumption. Two fleets
    angled 60 degrees off each other close at a fraction of that, and two on
    crossing diagonals can be "closing 27/turn" by speed-sum while the real
    figure is single digits and falling. Every consumer of this number (the
    E3.24 use-or-lose deadline, reinforcement timing, engagement estimates) was
    inheriting that optimism.

    Project both ships one turn straight ahead along their CURRENT facings and
    compare distances. Straight-line projection is itself an assumption - ships
    turn - but it is the current-heading truth rather than a best case, and it
    goes NEGATIVE when the geometry is actually opening, which is exactly the
    signal the deadline logic needs.
    """
    if not enemies:
        return 0
    me_now = (ship.get("x", 0), ship.get("y", 0))
    me_next = _project(ship, int(ship.get("speed") or 0))
    best = None
    for e in enemies:
        d_now = H.hex_distance(me_now, (e.get("x", 0), e.get("y", 0)))
        d_next = H.hex_distance(me_next, _project(e, int(e.get("speed") or 0)))
        rate = d_now - d_next
        if best is None or rate > best:
            best = rate
    return best or 0


def incoming_at(ship, enemies, rng):
    """Total expected damage from every enemy that can bear, at this range."""
    total, parts = 0.0, []
    for e in enemies:
        try:
            d, pr = expected_damage(e, ship, rng)
        except Exception:
            continue
        if d > 0:
            total += d
            parts.append(f"{e['label']} {d:.0f}")
    return total, parts


def likely_engagement(ship, enemies, rng):
    """The range he will actually FIGHT at, and the damage he lands there.

    Sizing reinforcement to a range-1 knife fight is useless - that is where
    everything is maximum and where a closing squadron usually does NOT stop.
    Take the band HIS guns want, clamped to what he can physically reach this
    turn, and cost the damage there.
    """
    rate = closing_rate(ship, enemies)
    reachable = max(1, rng - rate)
    wants = []
    for e in enemies:
        try:
            lo, hi = optimal_band(e, ship)
            wants.append(max(reachable, lo))
        except Exception:
            continue
    engage = max(min(wants) if wants else 8, reachable)
    d, parts = incoming_at(ship, enemies, engage)
    return d, engage, parts, rate


def shield_advice(ship, enemies, rng, surplus_hint=None):
    out = []
    if not enemies:
        return out
    ox = sum(e["x"] for e in enemies) / len(enemies)
    oy = sum(e["y"] for e in enemies) / len(enemies)
    face = ship.get("facing", 0)
    idx = H.shield_hit((ship["x"], ship["y"]), face, (round(ox), round(oy)))
    sh = ship.get("shields") or [0] * 6
    shm = ship.get("shields_max") or sh
    val, mx = sh[idx], (shm[idx] if idx < len(shm) else sh[idx])

    out.append(f"SHIELD: he bears on our {SHIELD[idx]} ({val}/{mx}).")

    # p16 / p113: never take the opening salvo on #1. Prefer #2 or #6.
    if idx == 0:
        left = (face + 1) % 6            # turn one hexside to put him on #6
        right = (face - 1) % 6           # or the other way to put him on #2
        out.append(f"SHIELD WARN he is on our #1 - doctrine is explicit that taking the "
                   f"opening salvo on #1 is never a good idea (p16; Kosnett p113). Turn one "
                   f"hexside to {FACING_VEC.get(left,'?')} to put him on #6, or "
                   f"{FACING_VEC.get(right,'?')} to put him on #2, before he is in range.")
    elif idx in (1, 5):
        out.append("SHIELD good facing - he is on a forward-quarter shield, which is where "
                   "doctrine wants the first salvo taken (p16).")
    elif idx == 3:
        out.append("SHIELD WARN he is on our #4 (aft) - we are running, and our rear shield "
                   "is usually the weakest. Turn before he closes.")

    # Reinforcement is a THIS-TURN purchase (D3.342): what can he reach and hurt
    # us with before the turn ends?
    dmg, at_rng, parts, rate = likely_engagement(ship, enemies, rng)
    if rate <= 0 or dmg <= 0:
        out.append(f"SHIELD no reinforcement - closing {rate}/turn from {rng}, nothing of his "
                   f"reaches a firing range this turn; it would expire unused (D3.342).")
        return out
    out.append(f"SHIELD THREAT: closing ~{rate}/turn from {rng}; he will fight at about range "
               f"{at_rng} for ~{dmg:.0f} damage ({', '.join(parts)}).")
    out.append(f"SHIELD do NOT use general reinforcement here: it is HALVED (D3.341) and the "
               f"first damage from any direction wipes the lot.")
    return out


def reinforce_plan(ship, enemies, rng, budget):
    """How much to put on WHICH shields, capped by both threat and budget.

    Two caps matter and the earlier version honoured neither: you cannot spend
    more than you have, and there is no point buying more boxes than he can
    shoot off. Spread over two facings because the bearing drifts as he closes.
    """
    if not enemies or budget <= 0:
        return None
    dmg, at_rng, _parts, rate = likely_engagement(ship, enemies, rng)
    if dmg <= 0 or rate <= 0:
        return None
    ox = sum(e["x"] for e in enemies) / len(enemies)
    oy = sum(e["y"] for e in enemies) / len(enemies)
    idx = H.shield_hit((ship["x"], ship["y"]), ship.get("facing", 0), (round(ox), round(oy)))
    sh = ship.get("shields") or [0] * 6
    # WHOLE POINTS ONLY. D3.342 buys one extra BOX per point of energy, and
    # damage is scored in whole points - so half a point of reinforcement buys
    # nothing at all. Floor everything and hand the remainder back to the caller
    # rather than printing a fractional box that cannot exist.
    spend = int(min(budget, dmg))          # never buy more than he can deliver
    if spend <= 0:
        return None

    # D3.343: "A shield that is down (or which has been dropped) cannot be
    # reinforced, but general reinforcement would still block fire coming from
    # that direction." Specific reinforcement onto a down facing is therefore not
    # merely a weak play - it is ILLEGAL, and the energy would be refused. When
    # the threatened facing is down, general reinforcement is the only thing that
    # answers fire from that bearing.
    if sh[idx] <= 0:
        # D3.341 charges 2 energy per point of general reinforcement against 1
        # for specific, so the same budget buys half as many boxes. It is still
        # the only legal protection for a down facing.
        gen = int(spend // 2)
        if gen <= 0:
            return None
        return dict(primary_idx=idx, primary=0, second_idx=None, second=0,
                    general=gen, total=gen * 2, threat=dmg, at_rng=at_rng,
                    down_facing=True)

    # The second facing must also be up. The old tie-break took the WEAKER
    # neighbour, which meant a shield already knocked to zero was actively
    # PREFERRED - precisely the facing that cannot legally be reinforced.
    nxt, prv = (idx + 1) % 6, (idx - 1) % 6
    cands = [i for i in (nxt, prv) if sh[i] > 0]
    # Among facings that are actually up, the weaker one is still the right
    # choice: it is the one closest to being breached.
    second = min(cands, key=lambda i: sh[i]) if cands else None

    primary = int(spend * 0.7)
    backup = spend - primary
    if second is None:                     # nowhere legal to put the remainder
        primary, backup = spend, 0
    elif backup <= 0 and primary > 1:      # keep a real second facing if we can
        primary, backup = primary - 1, 1
    return dict(primary_idx=idx, primary=primary, second_idx=second, second=backup,
                general=0, total=primary + backup, threat=dmg, at_rng=at_rng,
                down_facing=False)


# --------------------------------------------------------------------------
# Wild Weasel (J3.0) - and why it is the wrong tool for a Lyran.
#
# Cost/timing: one point on each of TWO CONSECUTIVE turns (J3.12); holdable in
# the bay on a rolling delay at one point per turn (J3.121), though a held WW
# "produces no benefits of any type" until launched (J3.24). Cannot launch less
# than 32 impulses from the start of charging (J3.122).
#
# The decisive interaction: (G23.48) a WW is VOIDED if the launching ship uses
# an ESG. For the Lyrans - whose entire drone defence IS the ESG - the two are
# mutually exclusive, and the ESG is the better answer against Kzinti drones.
# --------------------------------------------------------------------------
WW_CHARGE_TURNS = 2          # J3.12
WW_HOLD_COST = 1             # J3.121, per turn
WW_MIN_IMPULSES = 32         # J3.122


def weasel_advice(ship, enemies, state):
    """Should this ship be charging a wild weasel?"""
    out = []
    if not enemies:
        return out
    # Only relevant against seeking weapons.
    drones = sum((e.get("weapons") or {}).get("drone", [0, 0])[0] for e in enemies)
    plasma = sum((e.get("weapons") or {}).get("plasma", [0, 0])[0] for e in enemies)
    if not (drones or plasma):
        return out

    esg = (ship.get("weapons") or {}).get("esg", [0, 0])[0]
    if esg:
        out.append(f"WEASEL NO - this ship mounts {esg} ESG. G23.48 VOIDS a wild weasel the "
                   f"moment the launching ship uses an ESG, and the ESG is the better anti-drone "
                   f"answer anyway (it can be recharged; his drones cannot be replaced). "
                   f"Spend the power on ESGs and T-bombs instead.")
        return out

    out.append(f"WEASEL viable - he fields {drones} drone rack(s)"
               + (f" and {plasma} plasma" if plasma else "")
               + f". Charge: 1 point on each of {WW_CHARGE_TURNS} consecutive turns (J3.12), "
                 f"then holdable at {WW_HOLD_COST}/turn on a rolling delay (J3.121).")
    out.append(f"WEASEL start charging NOW if you want it available - it cannot launch within "
               f"{WW_MIN_IMPULSES} impulses of starting (J3.122), so a weasel begun this turn "
               f"is a NEXT-turn asset.")
    out.append("WEASEL limits: only ONE active at a time (J3.116); it protects ONLY the ship "
               "that launched it (J3.202); held in the bay it does nothing at all (J3.24); it "
               "is identified as a weasel the instant it launches (J3.17).")
    out.append("WEASEL beaten by: type-VI drones with their own lock-on ignore it (J3.20), as "
               "do Tame/Wild Boar type-IIIs (FD5.257). A drone-heavy enemy can simply carry the "
               "loadout that walks through it.")
    out.append("WEASEL doctrine is hostile to it: Schultz - 'any opponent who has used a wild "
               "weasel against me has lost the initiative and has been destroyed within two "
               "turns' (p5); the Kosnett post-mortem calls using one when T-bombs, tractors or "
               "a shield change would serve an ERROR (p113).")
    return out
