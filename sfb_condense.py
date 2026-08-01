"""
Trim an order block down to what actually changes.

The per-ship block had grown to ~50 lines, and most of the growth was not
information. Three separate problems, in order of how much bulk they added:

  1. THE SAME DECISION, THREE TIMES. The impulse block says
     ">>> DISRUPTORS: HOLD until range 8"; a legacy summary says
     "FIRE: hold until range 8."; the weapon-cycle prompt says
     "DISRUPTORS: hold for a shot ... (E3.24)". One decision, three lines.
  2. INVARIANT RULES TEXT reprinted every impulse - "cannot carry: type-III,
     multi-warhead, Starfish, Stingray" does not change between impulses, or
     between games.
  3. Long rationale under orders that are not in question.

Deduplication is unconditional: a restatement is never wanted. What the caller
controls is how much RATIONALE survives under each order, because detail is
wanted - just not repetition.
"""
from __future__ import annotations

import re

# How many reason lines to keep under each order. None = keep everything.
TERSE, NORMAL, FULL = 1, 3, None

# Subsystems an impulse order can be about. An order leads with the VERB
# ("LAUNCH 2 FIGHTER(S)"), not the subsystem, so the topic has to be recognised
# from anywhere in the line.
SUBSYSTEMS = ("DISRUPTOR", "PHASER", "PHOTON", "FUSION", "HELLBORE", "PLASMA",
              "DRONE", "FIGHTER", "ESG", "SCATTER", "SUICIDE", "WEASEL",
              "TRACTOR", "SHUTTLE")

# Words that mark a line as stating a DECISION rather than adding context. A
# later line about the same subsystem containing one of these is a restatement.
_DECISION = re.compile(r"\b(hold|fire|firing|launch|arm|raise|do not|don't)\b", re.I)

# ...but only if the line is an order for NOW. A schedule or a conditional is
# not a restatement even though it names the same subsystem and verb:
#   "FIGHTERS imp 28: whole group MAY FIRE direct-fire weapons (J1.342)"
# carries timing the order does not, and killing it lost the launch timeline
# entirely - which the self-test caught.
_SCHEDULED = re.compile(
    r"\b(imp|impulse|turn)\s*#?\s*\d|\bready in\b|\brecovery\b|\beta\b", re.I)

_REASON_INDENT = "      "          # six spaces: rationale under an order
_ORDER_MARK = ">>>"


def _topics(line):
    up = line.upper()
    return {s for s in SUBSYSTEMS if s in up}


def _key(line):
    """Loose semantic key, so differently-worded restatements collapse."""
    s = re.sub(r"^[\s>*-]*", "", line or "").lower()
    s = re.sub(r"^(fire|move|shield|fighters|disruptors|phasers|drones)\s*:?\s*", "", s)
    s = re.sub(r"[^a-z ]+", " ", s)
    return " ".join(s.split()[:6])


def condense(lines, max_reasons=NORMAL):
    """Drop restatements; cap rationale per order. Returns a new list."""
    texts = [str(x) for x in lines]

    # Subsystems that already have an explicit impulse order.
    ordered = set()
    for t in texts:
        s = t.strip()
        if s.startswith(_ORDER_MARK):
            ordered |= _topics(s)

    out, seen, kept = [], set(), 0
    for orig, text in zip(lines, texts):
        stripped = text.strip()

        if text.startswith(_REASON_INDENT):
            if max_reasons is not None and kept >= max_reasons:
                continue
            k = _key(text)
            if k and k in seen:
                continue
            if k:
                seen.add(k)
            kept += 1
            out.append(orig)
            continue

        # An explicit order is never a duplicate of another explicit order, even
        # when the wording matches: "7x AAS FLIGHT: CLOSE on Feral" and
        # "2x AAS FLIGHT: CLOSE on Feral" are two different flights, and the key
        # discards digits, so deduping these silently deleted a whole flight.
        if stripped.startswith(_ORDER_MARK):
            out.append(orig)
            kept = 0
            continue

        # A prose line about a subsystem that already has an order, and which
        # states a decision, is a restatement. Schedules and timelines about the
        # same subsystem are NOT - they add timing the order does not carry.
        if True:
            if (_DECISION.search(stripped) and (_topics(stripped) & ordered)
                    and not _SCHEDULED.search(stripped)):
                continue

        k = _key(text)
        if k and k in seen:
            continue
        if k:
            seen.add(k)
        kept = 0
        out.append(orig)
    return out


# ---------------------------------------------------------------- crew voice
# Category-prefix -> bridge station. Orders read as commands to a station, the
# way a captain would give them; the category survives in parentheses only
# where it disambiguates. Order matters: first match wins.
_STATIONS = (
    ("MOVE:", "HELM:"),
    ("MANEUVER", "HELM (maneuver):"),
    ("FIRE:", "GUNNERY:"),
    ("HUNT:", "GUNNERY - HUNT:"),
    ("DISRUPTORS:", "GUNNERY (disruptors):"),
    ("CAPACITOR", "GUNNERY (capacitor):"),
    ("SEEKERS", "DEFENCE (seekers):"),
    ("SCREEN:", "DEFENCE (screen):"),
    ("SHIELDS", "DEFENCE (shields):"),
    ("SHIELD:", "DEFENCE (shields):"),
    ("REINFORCE", "DEFENCE (reinforce):"),
    ("EW ", "SCIENCE (EW):"),
    ("EW:", "SCIENCE (EW):"),
    ("WILD WEASEL", "SHUTTLE BAY (weasel):"),
    ("WEASEL", "SHUTTLE BAY (weasel):"),
    ("ARM SUICIDE", "SHUTTLE BAY: ARM SUICIDE"),
    ("NO suicide", "SHUTTLE BAY: NO suicide"),
    ("SCATTER-PACK", "SHUTTLE BAY: SCATTER-PACK"),
)

# A rule citation: (G23.31), (FD1.51), (C4.32), (J4.814)... 1-2 letters then
# digits, dots and an optional trailing letter, inside parens.
_RE_CITE = re.compile(r"\s*\(([A-Z]{1,2}\d[\w.]*)\)")


def crewify(lines):
    """Render-side pass: station-voice order lines + citations demoted.

    Rule citations are stripped from HEADLINE order lines and re-issued on a
    dimmed 'refs:' line underneath (the render layer indents 10 spaces = dim).
    Rationale lines are already dim and keep their citations in place - the
    provenance is always available, it just stops shouting. Pure presentation:
    the engine's own lines (and the Flagship scorer) are untouched.
    """
    out = []
    for ln in lines:
        s = ln.lstrip()
        indent = ln[:len(ln) - len(s)]
        is_headline = ln.startswith("    ") and not ln.startswith("          ") \
            and not s.startswith(">>>")
        if is_headline:
            for pre, station in _STATIONS:
                if s.startswith(pre):
                    if station.endswith(":"):
                        rest = s[len(pre):].lstrip(" :")
                        s = station + " " + rest
                    else:
                        s = station + s[len(pre):]
                    break
            cites = _RE_CITE.findall(s)
            if cites:
                s = _RE_CITE.sub("", s).replace("  ", " ").rstrip()
                out.append(indent + s)
                out.append("          refs: " + ", ".join(dict.fromkeys(cites)))
                continue
        out.append(indent + s if is_headline else ln)
    return out


# ------------------------------------------------------------------ buckets
# Section -> the markers that put a HEADLINE there. Rationale/refs lines follow
# whatever headline they sit under, so an order and its reasoning never split.
_BUCKETS = (
    ("MOVEMENT", ("MOVE:", "HELM", "MANEUVER", "TAC ", "HET", "ERRATIC",
                  "STRAIGHT", "SIDESLIP", "TURN ", "no movement")),
    ("WEAPONS", ("FIRE", "GUNNERY", "HUNT", "DISRUPTOR", "PHASER", "capacitor",
                 "CAPACITOR", "LAUNCH", "DRONE", "drones", "OVERLOAD",
                 "use-or-lose", "FIRE THIS TURN")),
    ("DEFENCE", ("SHIELD", "REINFORCE", "SEEKERS", "SCREEN", "ESG", "WEASEL",
                 "SHUTTLE", "SCATTER", "SUICIDE", "suicide", "ADD ", "DEFENCE",
                 "SCIENCE", "EW ")),
    ("COMMAND & ENERGY", ("MISSION", "POSTURE", "DOCTRINE", "TRADE", "EAF",
                          "BATTERY", "battery", "OUTCOME", "DISENGAGE")),
)


def bucket_orders(lines):
    """Split one ship's order lines into sections for a quadrant layout.

    Returns {section: [lines]} (sections in _BUCKETS order, always present).
    A headline is filed by its first matching marker; its indented rationale
    and refs lines travel with it. The '>>> IMPULSE n' band is classified by
    CONTENT (a launch order is WEAPONS even though it is an impulse action).
    Unmatched headlines land in COMMAND & ENERGY rather than vanishing.
    """
    out = {name: [] for name, _ in _BUCKETS}
    current = "COMMAND & ENERGY"
    for ln in lines:
        s = ln.strip()
        deep = ln.startswith("          ")          # rationale/refs
        if not deep and s:
            probe = s[4:] if s.startswith(">>> ") else s
            placed = None
            for name, keys in _BUCKETS:
                if any(k in probe for k in keys):
                    placed = name
                    break
            current = placed or "COMMAND & ENERGY"
        out[current].append(ln)
    return out


def _selftest():
    sample = [
        "    >>> DISRUPTORS: HOLD until range 8",
        "          range 15, effective band 5-8 - firing now wastes it",
        "    DISRUPTORS: hold for a shot; they cannot carry to next turn (E3.24).",
        "    FIRE: hold until range 8.",
        "    >>> LAUNCH 2 FIGHTER(S) NOW (4 still aboard)",
        "          J1.50: one per bay per two impulses",
        "          each fighter's lockout runs from ITS OWN launch",
        "          third reason that NORMAL keeps but TERSE drops",
        "    FIGHTERS LAUNCH NOW for a GUN strike - 18 impulses clears 12",
        "    FIGHTERS   imp 28: whole group may fire DIRECT-FIRE weapons",
    ]
    got = [s.strip() for s in condense(sample, NORMAL)]
    assert "DISRUPTORS: hold for a shot; they cannot carry to next turn (E3.24)." not in got, got
    assert "FIRE: hold until range 8." not in got, got
    assert "FIGHTERS LAUNCH NOW for a GUN strike - 18 impulses clears 12" not in got, got
    assert any("imp 28" in g for g in got), "timeline must survive"
    assert any(g.startswith(">>> DISRUPTORS") for g in got)
    assert len(condense(sample, TERSE)) < len(condense(sample, NORMAL)) < len(sample)
    return len(sample), len(condense(sample, FULL)), len(condense(sample, NORMAL)), \
        len(condense(sample, TERSE))


if __name__ == "__main__":
    print("raw/full/normal/terse:", _selftest())
