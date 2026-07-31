"""
Game-event reader for the online driver.

Combat, phase transitions, and announcements travel as Java-serialized
gub.net.GUBNetMessage objects sent over a chunked privmsg channel:

    privmsg #room :~~|STARTx
    privmsg #room :~~|<base64 chunk>x
    privmsg #room :~~|<base64 chunk>x
    privmsg #room :~~|ENDx

Concatenate the chunks (minus the trailing 'x' marker), base64-decode, gunzip,
and you get a Java-serialized GUBNetMessage. We don't need a full Java
deserializer to be USEFUL — the human-readable announcement strings
(GamePluginEvent.msg) and BoardEvent attribute tokens are recoverable directly.

This gives the bot/advisor combat awareness (who fired what at whom, damage
dealt, phase changes) even though *executing* fire needs serialized-event
construction (a separate, larger task).
"""

from __future__ import annotations

import re
import gzip
import base64
from dataclasses import dataclass
from typing import Callable, Optional

_CHUNK_RE = re.compile(r"~~\|(.*)$")
_FIRE_RE = re.compile(r"(.+?) fires (.+?) \((.+?)\) at (.+?) \(Range: (\d+)\)")
_DAMAGE_RE = re.compile(r"Damage:\s*([\d/]+)\s*\(Total:\s*(\d+)\)")


@dataclass
class GameEvent:
    kind: str            # "fire" | "damage" | "announce" | "board" | "raw"
    text: str            # human-readable summary
    data: dict           # structured fields


def _readable(raw: bytes) -> list:
    return [s.decode("latin1") for s in re.findall(rb"[\x20-\x7e]{4,}", raw)]


def decode_group(b64: str) -> Optional[bytes]:
    try:
        return gzip.decompress(base64.b64decode(b64))
    except Exception:
        return None


def parse_group(raw: bytes) -> list:
    """Turn one decoded GUBNetMessage into zero or more GameEvents."""
    toks = _readable(raw)
    events = []
    is_board = any("BoardEvent" in t for t in toks)
    for t in toks:
        # strip a leading Java length/marker byte that often prefixes the string
        s = t[1:] if t and not t[0].isalnum() and t[0] not in "([" else t
        m = _FIRE_RE.search(s)
        if m:
            events.append(GameEvent("fire",
                f"{m.group(1)} fires {m.group(2)} at {m.group(4)} (r{m.group(5)})",
                {"attacker": m.group(1), "weapon": m.group(2), "arc": m.group(3),
                 "target": m.group(4), "range": int(m.group(5))}))
            continue
        m = _DAMAGE_RE.search(s)
        if m:
            shields = [int(x) for x in m.group(1).split("/") if x != ""]
            events.append(GameEvent("damage",
                f"damage {m.group(1)} (total {m.group(2)})",
                {"per_shield": shields, "total": int(m.group(2))}))
            continue
        if "Announcements" in s and len(s) > 14:
            events.append(GameEvent("announce", s.replace("Announcements", "").strip(), {}))
    if not events and is_board:
        # a board state event with no readable announcement (attribute token)
        attr = next((t for t in toks if t.isupper() and "_" in t and t != "FIRE_EVENT"), "")
        events.append(GameEvent("board", attr or "board-event", {"attribute": attr}))
    return events


class EventReassembler:
    """Feed it raw privmsg payloads; it emits GameEvents as groups complete."""

    def __init__(self, on_event: Optional[Callable[[GameEvent], None]] = None):
        self._buf = None
        self.on_event = on_event
        self.events = []

    def feed_privmsg(self, text: str) -> list:
        m = _CHUNK_RE.search(text)
        if not m:
            return []
        payload = m.group(1)
        out = []
        if payload == "STARTx":
            self._buf = []
        elif payload == "ENDx":
            if self._buf is not None:
                raw = decode_group("".join(self._buf))
                self._buf = None
                if raw:
                    for ev in parse_group(raw):
                        self.events.append(ev)
                        out.append(ev)
                        if self.on_event:
                            self.on_event(ev)
        elif payload.endswith("x") and self._buf is not None:
            self._buf.append(payload[:-1])
        return out
