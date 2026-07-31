"""
Headless SFB Online game driver.

Speaks the reverse-engineered `pastiche` protocol used by the SFU Online Client
so an AI (or any script) can join a game room, track the board state, and move
ships -- appearing to the human in their SFU client as a second player.

Transport-agnostic: point it at the real server (server.sfbonline.com:6668) or a
local relay (localhost:6668) -- same code either way.

    python sfb_client.py --host server.sfbonline.com --port 6668 \
                         --nick MyBot --room "#SFB_Game1"

The password is read from the SFB_PASSWORD environment variable (never the CLI),
or interactively if a TTY is available.

Protocol reference: see SFB_ONLINE_PROTOCOL.md.
Confirmed on the wire: login, join, names, and the incoming object-message layout
(`setattr <chan> <obj> <attr> <val>`, `addobj/activateobj/delobj <chan> <obj>`).
The outgoing argument order for object commands is best-effort (matches the
incoming layout) and may want a one-move live capture to confirm the colon nuance.
"""

from __future__ import annotations

import os
import sys
import json
import gzip
import base64
import socket
import threading
import time
import argparse
from dataclasses import dataclass, field
from typing import Callable, Optional


# --------------------------------------------------------------------------
# Value codec  (confirmed from live capture)
# Attribute values on the wire are base64(gzip(typed-JSON)):
#   boardLocation -> {"@type":"pd.pfranz.map.BoardLocation","x":10,"y":12}
#   facing/speed  -> {"@type":"int","value":N}
#   Label/race    -> "Bismarck" / "Federation"   (plain JSON string)
# --------------------------------------------------------------------------

BOARDLOC_TYPE = "pd.pfranz.map.BoardLocation"


def encode_value(obj) -> str:
    """Python value -> base64(gzip(json)). Wrap ints/bools in typed form."""
    raw = json.dumps(obj, separators=(",", ":")).encode("utf-8")
    return base64.b64encode(gzip.compress(raw)).decode("ascii")


def decode_value(b64: str):
    """base64(gzip(json)) -> Python value, unwrapping the {@type,value} form."""
    try:
        obj = json.loads(gzip.decompress(base64.b64decode(b64)).decode("utf-8"))
    except Exception:
        return b64  # not an encoded value; hand back the raw token
    if isinstance(obj, dict) and obj.get("@type") in ("int", "boolean", "long",
                                                       "double", "float"):
        return obj.get("value")
    return obj


def board_location(x: int, y: int) -> str:
    return encode_value({"@type": BOARDLOC_TYPE, "x": x, "y": y})


def typed_int(n: int) -> str:
    return encode_value({"@type": "int", "value": n})


# --------------------------------------------------------------------------
# Game-state model
# --------------------------------------------------------------------------

@dataclass
class Piece:
    """A game object (ship, plasma, freighter, ...) and its attributes."""
    obj_id: str
    attrs: dict[str, str] = field(default_factory=dict)
    active: bool = False

    # attrs hold DECODED values (see SFBGameClient._on_line).
    @property
    def xy(self):
        """(x, y) board coordinates, or None."""
        loc = self.attrs.get("boardLocation")
        if isinstance(loc, dict) and "x" in loc and "y" in loc:
            return (loc["x"], loc["y"])
        return None

    @property
    def location(self) -> Optional[str]:
        xy = self.xy
        return f"{xy[0]:02d}{xy[1]:02d}" if xy else None   # hex label e.g. 1012

    @property
    def facing(self):
        return self.attrs.get("facing")

    @property
    def speed(self):
        return self.attrs.get("current_speed")

    @property
    def owner(self) -> Optional[str]:
        # owner is encoded in the object id: gp*<owner>:<timestamp>
        if self.obj_id.startswith("gp*") and ":" in self.obj_id:
            return self.obj_id[3:].rsplit(":", 1)[0]
        return self.attrs.get("owner")

    @property
    def label(self) -> Optional[str]:
        return self.attrs.get("Label")

    @property
    def race(self) -> Optional[str]:
        return self.attrs.get("race")

    @property
    def unit_type(self) -> Optional[str]:
        return self.attrs.get("unit_type")

    def __repr__(self) -> str:
        return (f"Piece({self.label or self.obj_id} "
                f"{self.race or ''} {self.unit_type or '?'} "
                f"@{self.location or '?'} dir={self.facing if self.facing is not None else '?'} "
                f"spd={self.speed if self.speed is not None else '?'} "
                f"owner={self.owner or '?'}"
                f"{'' if self.active else ' [inactive]'})")


class GameState:
    """Mirror of the room's object store, updated from incoming messages."""

    def __init__(self) -> None:
        self.pieces: dict[str, Piece] = {}
        self._lock = threading.Lock()

    def add(self, obj_id: str) -> None:
        with self._lock:
            self.pieces.setdefault(obj_id, Piece(obj_id))

    def set_attr(self, obj_id: str, attr: str, value: str) -> None:
        with self._lock:
            p = self.pieces.setdefault(obj_id, Piece(obj_id))
            p.attrs[attr] = value

    def activate(self, obj_id: str) -> None:
        with self._lock:
            self.pieces.setdefault(obj_id, Piece(obj_id)).active = True

    def remove(self, obj_id: str) -> None:
        with self._lock:
            self.pieces.pop(obj_id, None)

    def clear(self) -> None:
        with self._lock:
            self.pieces.clear()

    def snapshot(self) -> list[Piece]:
        with self._lock:
            return [Piece(p.obj_id, dict(p.attrs), p.active)
                    for p in self.pieces.values()]

    def mine(self, nick: str) -> list[Piece]:
        return [p for p in self.snapshot() if p.owner == nick]

    def enemies(self, nick: str) -> list[Piece]:
        return [p for p in self.snapshot()
                if p.owner is not None and p.owner != nick]


# --------------------------------------------------------------------------
# Protocol connection
# --------------------------------------------------------------------------

class SFBConnection:
    """Socket + pastiche-protocol framing. Lowercase verbs, colon-prefixed
    trailing params, as confirmed from the live capture."""

    def __init__(self, host: str, port: int, nick: str, realname: str,
                 on_line: Callable[[str], None], verbose: bool = True) -> None:
        self.host, self.port = host, port
        self.nick, self.realname = nick, realname
        self.on_line = on_line
        self.verbose = verbose
        self.sock: Optional[socket.socket] = None
        self._buf = b""
        self._stop = threading.Event()
        self._authed = threading.Event()
        self._joined: dict[str, threading.Event] = {}

    # ---- low level ----

    def _log(self, direction: str, line: str) -> None:
        if self.verbose:
            # never echo a password
            safe = line
            low = line.lower()
            if low.startswith(("pass ", "login ")):
                parts = line.split(" ")
                if low.startswith("login") and len(parts) >= 3:
                    parts[2] = "***"
                elif low.startswith("pass") and len(parts) >= 2:
                    parts[1] = "***"
                safe = " ".join(parts)
            print(f"  {direction} {safe}", flush=True)

    def send(self, line: str) -> None:
        self._log(">>>", line)
        self.sock.sendall((line + "\r\n").encode("utf-8"))

    def _recv_line(self) -> str:
        while b"\r\n" not in self._buf:
            chunk = self.sock.recv(4096)
            if not chunk:
                raise ConnectionError("server closed connection")
            self._buf += chunk
        line, self._buf = self._buf.split(b"\r\n", 1)
        return line.decode("utf-8", errors="replace")

    def _reader(self) -> None:
        while not self._stop.is_set():
            try:
                line = self._recv_line()
            except Exception as e:
                if not self._stop.is_set():
                    print(f"[reader] disconnected: {e}", flush=True)
                break
            self._log("<<<", line)
            self._dispatch_control(line)
            self.on_line(line)

    def _dispatch_control(self, line: str) -> None:
        if line.startswith("PING"):
            token = line.split(":", 1)[1] if ":" in line else line[5:]
            self.send(f"PONG :{token}")
            return
        if "Authentication successful" in line:
            self._authed.set()
        if " JOIN " in line:
            chan = line.split(" JOIN ")[-1].lstrip(":")
            ev = self._joined.get(chan)
            if ev:
                ev.set()

    # ---- protocol ----

    def connect_and_login(self, password: str, timeout: int = 15) -> None:
        self.sock = socket.create_connection((self.host, self.port), timeout=timeout)
        self.sock.settimeout(None)
        self.sock.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
        threading.Thread(target=self._reader, daemon=True).start()
        self.send(f"pass {password}")
        self.send(f"nick :{self.nick}")
        self.send(f"user {self.nick} 0 0 :{self.realname}")
        self.send(f"login {self.nick} {password}")
        if not self._authed.wait(timeout=timeout):
            raise RuntimeError("authentication timed out")

    def join(self, channel: str, timeout: int = 10) -> bool:
        ev = threading.Event()
        self._joined[channel] = ev
        self.send(f"join :{channel}")
        return ev.wait(timeout=timeout)

    def names(self, channel: str = "") -> None:
        self.send(f"names :{channel}")

    # ---- object / game commands (outgoing) ----
    # Argument order mirrors the confirmed *incoming* parse:
    #   setattr <chan> <obj> <attr> <val> ; addobj/activateobj/delobj <chan> <obj>

    def add_obj(self, channel: str, obj_id: str) -> None:
        self.send(f"addobj {channel} :{obj_id}")

    def set_attr_raw(self, channel: str, obj_id: str, attr: str, encoded: str) -> None:
        """Send an already-encoded (base64(gzip(json))) value."""
        self.send(f"setattr {channel} {obj_id} {attr} :{encoded}")

    def set_attr(self, channel: str, obj_id: str, attr: str, value) -> None:
        """Encode a Python value and send it."""
        self.set_attr_raw(channel, obj_id, attr, encode_value(value))

    def activate_obj(self, channel: str, obj_id: str) -> None:
        self.send(f"activateobj {channel} :{obj_id}")

    def del_obj(self, channel: str, obj_id: str) -> None:
        self.send(f"delobj {channel} :{obj_id}")

    def list_objs(self, channel: str) -> None:
        self.send(f"listobjs :{channel}")

    def say(self, channel: str, text: str) -> None:
        self.send(f"privmsg {channel} :{text}")

    def disconnect(self) -> None:
        self._stop.set()
        try:
            self.send("quit :bye")
        except Exception:
            pass
        if self.sock:
            self.sock.close()


# --------------------------------------------------------------------------
# High-level game client
# --------------------------------------------------------------------------

# A Brain looks at (client, state) and issues commands. Return value ignored.
Brain = Callable[["SFBGameClient", GameState], None]


class SFBGameClient:
    def __init__(self, host: str, port: int, nick: str, room: str,
                 realname: str = "SFB AI", verbose: bool = True) -> None:
        self.nick = nick
        self.room = room
        self.state = GameState()
        import sfb_events
        self.events = sfb_events.EventReassembler(on_event=self._on_event)
        self.combat_log = []          # accumulated GameEvents (fire/damage/…)
        self.conn = SFBConnection(host, port, nick, realname,
                                  on_line=self._on_line, verbose=verbose)

    def _on_event(self, ev) -> None:
        self.combat_log.append(ev)
        if ev.kind in ("fire", "damage"):
            print(f"[combat] {ev.text}", flush=True)

    # ---- incoming object-message parsing ----

    def _on_line(self, line: str) -> None:
        # Strip optional ":prefix " then tokenise the command.
        body = line
        if body.startswith(":"):
            sp = body.find(" ")
            if sp == -1:
                return
            body = body[sp + 1:]
        parts = body.split(" ")
        if not parts:
            return
        verb = parts[0].lower()

        # parts layout after verb mirrors the confirmed incoming parse:
        #   [verb, channel, obj, attr, value...]
        try:
            if verb == "setattr" and len(parts) >= 5:
                obj, attr = parts[2], parts[3]
                value = " ".join(parts[4:]).lstrip(":")
                self.state.set_attr(obj, attr, decode_value(value))
            elif verb == "addobj" and len(parts) >= 3:
                self.state.add(parts[2].lstrip(":"))
            elif verb == "activateobj" and len(parts) >= 3:
                self.state.activate(parts[2].lstrip(":"))
            elif verb in ("delobj", "removeobj") and len(parts) >= 3:
                self.state.remove(parts[2].lstrip(":"))
            elif verb in ("delobjs", "removeallobjects"):
                self.state.clear()
            elif verb == "privmsg":
                # chunked game-event channel (~~|STARTx … ~~|ENDx)
                payload = " ".join(parts[2:]).lstrip(":")
                if payload.startswith("~~|"):
                    self.events.feed_privmsg(payload)
        except Exception as e:
            print(f"[parse] {verb}: {e}", flush=True)

    # ---- lifecycle ----

    def start(self, password: str) -> None:
        print(f"Connecting to {self.conn.host}:{self.conn.port} as {self.nick} ...")
        self.conn.connect_and_login(password)
        print("Authenticated.")
        if not self.conn.join(self.room):
            print(f"Warning: join {self.room} not confirmed")
        else:
            print(f"Joined {self.room}")
        # ask the server for the current object list to seed state
        self.conn.list_objs(self.room)
        time.sleep(1.5)

    # ---- convenience move helpers (delegate to conn with the room bound) ----

    def move(self, obj_id: str, x: int, y: int) -> None:
        self.conn.set_attr_raw(self.room, obj_id, "boardLocation", board_location(x, y))

    def turn(self, obj_id: str, facing: int) -> None:
        self.conn.set_attr_raw(self.room, obj_id, "facing", typed_int(facing))

    def set_speed(self, obj_id: str, speed: int) -> None:
        self.conn.set_attr_raw(self.room, obj_id, "current_speed", typed_int(speed))

    def commit(self, obj_id: str) -> None:
        self.conn.activate_obj(self.room, obj_id)

    def run(self, brain: Brain, tick: float = 3.0) -> None:
        """Poll loop: refresh state, let the brain act, repeat."""
        print("Running brain loop (Ctrl-C to stop) ...")
        try:
            while True:
                brain(self, self.state)
                time.sleep(tick)
        except KeyboardInterrupt:
            print("\nStopping.")
        finally:
            self.conn.disconnect()


# --------------------------------------------------------------------------
# Placeholder brain
# --------------------------------------------------------------------------

def observer_brain(client: SFBGameClient, state: GameState) -> None:
    """Default no-op policy: just report what it sees. Swap this out for a real
    tactical brain (rules-based or Claude-driven) once the plumbing is proven."""
    pieces = state.snapshot()
    if not pieces:
        print("[brain] board empty / no objects visible yet")
        return
    print(f"[brain] {len(pieces)} object(s):")
    mine = state.mine(client.nick)
    enemies = state.enemies(client.nick)
    for p in pieces:
        tag = "MINE" if p in mine else ("ENEMY" if p in enemies else "     ")
        print(f"    {tag} {p!r}")
    # A real brain would decide here, e.g.:
    #   client.set_speed(my_ship.obj_id, "8")
    #   client.turn(my_ship.obj_id, "B")
    #   client.move(my_ship.obj_id, "1008")
    #   client.commit(my_ship.obj_id)


# --------------------------------------------------------------------------
# CLI
# --------------------------------------------------------------------------

def main() -> None:
    ap = argparse.ArgumentParser(description="Headless SFB Online game driver")
    ap.add_argument("--host", default="server.sfbonline.com")
    ap.add_argument("--port", type=int, default=6668)
    ap.add_argument("--nick", required=True, help="bot login/nick (its own account)")
    ap.add_argument("--room", required=True, help="game room, e.g. #SFB_Game1")
    ap.add_argument("--tick", type=float, default=3.0, help="brain poll interval (s)")
    args = ap.parse_args()

    password = os.environ.get("SFB_PASSWORD")
    if not password and sys.stdin.isatty():
        import getpass
        password = getpass.getpass(f"Password for {args.nick}: ")
    if not password:
        sys.exit("No password: set SFB_PASSWORD env var or run in a terminal.")

    client = SFBGameClient(args.host, args.port, args.nick, args.room)
    client.start(password)
    client.run(observer_brain, tick=args.tick)


if __name__ == "__main__":
    main()
