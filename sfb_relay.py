"""
Local SFB "pastiche" relay server.

Emulates enough of server.sfbonline.com's custom IRC-like daemon for the SFU
Online Client (and our headless bot) to connect, share a game room, and relay
game-object messages between them -- entirely offline on localhost.

Design: the relay is a faithful MESSAGE BROKER plus a per-room OBJECT STORE.
Game rules live in the clients; the relay only (1) handles the login/join/names
handshake, (2) keeps each room's objects so late joiners resync, and (3) relays
every in-room message to the other members. Anything it doesn't specifically
understand is still relayed verbatim, so the two boards stay in lockstep.

It also logs every line, so it doubles as the capture tool for pinning down the
exact game-object wire format when a real SFU client moves a ship.

    python sfb_relay.py                 # listens on 127.0.0.1:6668

Point the SFU client at it by adding to the client's gub.ini server list:
    server1=irc,Local,127.0.0.1:6668
or via a hosts redirect of server.sfbonline.com -> 127.0.0.1.

Login accepts ANY credentials (local, no real auth).
"""

from __future__ import annotations

import socket
import threading
import time
import argparse
from collections import OrderedDict
from typing import Optional

SERVER_NAME = "game.sfbonline.com"   # what numerics are tagged with
HOST_SUFFIX = "local"


# --------------------------------------------------------------------------
# Line parsing (IRC-ish, with this daemon's colon-prefixed params)
# --------------------------------------------------------------------------

def parse_line(line: str):
    """Return (prefix, command, params[]). A param starting ':' is trailing."""
    prefix = None
    s = line
    if s.startswith(":"):
        sp = s.find(" ")
        prefix = s[1:sp] if sp != -1 else s[1:]
        s = s[sp + 1:] if sp != -1 else ""
    params = []
    while s:
        if s.startswith(":"):
            params.append(s[1:])
            break
        sp = s.find(" ")
        if sp == -1:
            params.append(s)
            break
        params.append(s[:sp])
        s = s[sp + 1:]
    command = params.pop(0) if params else ""
    return prefix, command, params


# --------------------------------------------------------------------------
# Object store
# --------------------------------------------------------------------------

class GameObject:
    def __init__(self, obj_id: str) -> None:
        self.obj_id = obj_id
        self.attrs: "OrderedDict[str,str]" = OrderedDict()
        self.active = False


class Room:
    def __init__(self, name: str) -> None:
        self.name = name
        self.members: set["Client"] = set()
        self.objects: "OrderedDict[str,GameObject]" = OrderedDict()
        self.topic: Optional[str] = None
        self.lock = threading.RLock()   # reentrant: _object_cmd -> _resync re-locks


# --------------------------------------------------------------------------
# Client connection
# --------------------------------------------------------------------------

class Client:
    def __init__(self, sock: socket.socket, addr, server: "RelayServer") -> None:
        self.sock = sock
        self.addr = addr
        self.server = server
        self.nick: Optional[str] = None
        self.realname = ""
        self.rooms: set[str] = set()
        self._buf = b""
        self.alive = True

    @property
    def hostmask(self) -> str:
        n = self.nick or "unknown"
        return f"{n}!{n}@{HOST_SUFFIX}"

    def send(self, line: str) -> None:
        try:
            self.sock.sendall((line + "\r\n").encode("utf-8"))
            self.server.log(f"-> {self.nick or self.addr[1]}: {line}")
        except Exception:
            self.alive = False

    def recv_line(self) -> str:
        while b"\r\n" not in self._buf:
            chunk = self.sock.recv(4096)
            if not chunk:
                raise ConnectionError("closed")
            self._buf += chunk
        line, self._buf = self._buf.split(b"\r\n", 1)
        return line.decode("utf-8", errors="replace")


# --------------------------------------------------------------------------
# Relay server
# --------------------------------------------------------------------------

class RelayServer:
    def __init__(self, host: str, port: int, verbose: bool = True) -> None:
        self.host, self.port = host, port
        self.verbose = verbose
        self.clients: set[Client] = set()
        self.rooms: dict[str, Room] = {}
        self.lock = threading.Lock()

    def log(self, msg: str) -> None:
        if self.verbose:
            print(f"[{time.strftime('%H:%M:%S')}] {msg}", flush=True)

    # ---- room helpers ----

    def get_room(self, name: str) -> Room:
        with self.lock:
            r = self.rooms.get(name)
            if r is None:
                r = Room(name)
                self.rooms[name] = r
            return r

    def broadcast(self, room: Room, line: str, exclude: Optional[Client] = None) -> None:
        for m in list(room.members):
            if m is not exclude and m.alive:
                m.send(line)

    # ---- lifecycle ----

    def serve(self) -> None:
        srv = socket.socket()
        srv.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        srv.bind((self.host, self.port))
        srv.listen(8)
        self.log(f"SFB relay listening on {self.host}:{self.port}")
        try:
            while True:
                sock, addr = srv.accept()
                sock.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
                c = Client(sock, addr, self)
                with self.lock:
                    self.clients.add(c)
                threading.Thread(target=self._handle, args=(c,), daemon=True).start()
        except KeyboardInterrupt:
            pass
        finally:
            srv.close()

    def _handle(self, c: Client) -> None:
        self.log(f"connect from {c.addr}")
        # Greeting: real server sends AUTH notices then the login prompt.
        c.send(f":{SERVER_NAME} NOTICE AUTH :Looking up your hostname...")
        c.send(f":{SERVER_NAME} NOTICE AUTH :Hostname not found.")
        c.send(f":{SERVER_NAME} NOTICE AUTH :*** Enter your login and password now.")
        try:
            while c.alive:
                raw = c.recv_line()
                self.log(f"<- {c.nick or c.addr[1]}: {raw}")
                self._dispatch(c, raw)
        except Exception as e:
            self.log(f"disconnect {c.nick or c.addr}: {e}")
        finally:
            self._cleanup(c)

    def _cleanup(self, c: Client) -> None:
        for rn in list(c.rooms):
            room = self.rooms.get(rn)
            if room:
                room.members.discard(c)
                self.broadcast(room, f":{c.hostmask} PART :{rn}")
        with self.lock:
            self.clients.discard(c)
        try:
            c.sock.close()
        except Exception:
            pass

    # ---- command dispatch ----

    def _dispatch(self, c: Client, raw: str) -> None:
        prefix, cmd, params = parse_line(raw)
        cmd = cmd.lower()

        if cmd == "pass":
            return
        if cmd == "nick":
            c.nick = params[0] if params else c.nick
            return
        if cmd == "user":
            if params:
                c.realname = params[-1]
            return
        if cmd == "login":
            c.nick = params[0] if params else c.nick
            self._welcome(c)
            return
        if cmd == "ping":
            token = params[0] if params else ""
            c.send(f":{SERVER_NAME} PONG {SERVER_NAME} :{token}")
            return
        if cmd == "pong":
            return
        if cmd == "quit":
            c.alive = False
            return
        if cmd == "join":
            for ch in (params[0].split(",") if params else []):
                self._join(c, ch)
            return
        if cmd == "part":
            for ch in (params[0].split(",") if params else []):
                self._part(c, ch)
            return
        if cmd == "names":
            self._names(c, params[0] if params and params[0] else None)
            return
        if cmd == "privmsg":
            self._privmsg(c, params)
            return

        # --- object / game commands ---
        if cmd in ("addobj", "setattr", "activateobj", "delobj", "removeobj",
                   "delobjs", "removeallobjects", "listobjs"):
            self._object_cmd(c, cmd, params, raw)
            return

        # Unknown in-room message (e.g. chunked ~~| object payloads): relay
        # verbatim to other members of every room this client is in.
        self._relay_unknown(c, raw)

    # ---- handlers ----

    def _welcome(self, c: Client) -> None:
        n = c.nick or "user"
        c.send(f":{SERVER_NAME} NOTICE {n} :*** Enter your login and password now.")
        c.send(f":{SERVER_NAME} 001 {n} :Welcome to the FooBar IRC network {c.hostmask}")
        c.send(f":{SERVER_NAME} 002 {n} :Your host is {SERVER_NAME}[@127.0.0.1], running version pastiche-0")
        c.send(f":{SERVER_NAME} 003 {n} :This server was created Recently")
        c.send(f":{SERVER_NAME} 004 {n} {SERVER_NAME} pastiche-0 o o")
        c.send(f":{SERVER_NAME} 375 {n} :- {SERVER_NAME} Message of the day -")
        c.send(f":{SERVER_NAME} 372 {n} :- Welcome to the local SFB relay.")
        c.send(f":{SERVER_NAME} 376 {n} :End of /MOTD command.")
        c.send(f":{SERVER_NAME} NOTICE LOGIN :*** Authentication successful")
        c.send(f":{SERVER_NAME} 251 {n} :There are {len(self.clients)} users and 0 invisible on 1 servers")
        c.send(f":{SERVER_NAME} 255 {n} :I have {len(self.clients)} clients and 0 servers")

    def _join(self, c: Client, channel: str) -> None:
        room = self.get_room(channel)
        room.members.add(c)
        c.rooms.add(channel)
        # Tell everyone (incl joiner) that this client joined.
        self.broadcast(room, f":{c.hostmask} JOIN :{channel}")
        n = c.nick or "user"
        # topic + names
        if room.topic:
            c.send(f":{SERVER_NAME} 332 {n} {channel} :{room.topic}")
        else:
            c.send(f":{SERVER_NAME} 331 {n} {channel} :No topic is set")
        names = " ".join(sorted(m.nick or "?" for m in room.members))
        c.send(f":{SERVER_NAME} 353 {n} = {channel} :{names}")
        c.send(f":{SERVER_NAME} 366 {n} {channel} :End of /NAMES list.")
        # Resync existing objects to the new joiner.
        self._resync(c, room)

    def _resync(self, c: Client, room: Room) -> None:
        with room.lock:
            for obj in room.objects.values():
                c.send(f":{SERVER_NAME} addobj {room.name} {obj.obj_id}")
                for k, v in obj.attrs.items():
                    c.send(f":{SERVER_NAME} setattr {room.name} {obj.obj_id} {k} {v}")
                if obj.active:
                    c.send(f":{SERVER_NAME} activateobj {room.name} {obj.obj_id}")

    def _part(self, c: Client, channel: str) -> None:
        room = self.rooms.get(channel)
        if room:
            room.members.discard(c)
            self.broadcast(room, f":{c.hostmask} PART :{channel}")
        c.rooms.discard(channel)

    def _names(self, c: Client, channel: Optional[str]) -> None:
        n = c.nick or "user"
        if channel:
            room = self.rooms.get(channel)
            members = " ".join(sorted(m.nick or "?" for m in room.members)) if room else ""
            c.send(f":{SERVER_NAME} 353 {n} = {channel} :{members}")
            c.send(f":{SERVER_NAME} 366 {n} {channel} :End of /NAMES list.")
        else:
            for rn, room in list(self.rooms.items()):
                members = " ".join(sorted(m.nick or "?" for m in room.members))
                c.send(f":{SERVER_NAME} 353 {n} = {rn} :{members}")
            allr = ",".join(self.rooms.keys())
            c.send(f":{SERVER_NAME} 366 {n} {allr} :End of /NAMES list.")

    def _privmsg(self, c: Client, params: list[str]) -> None:
        if len(params) < 2:
            return
        target, text = params[0], params[1]
        room = self.rooms.get(target)
        if room:
            self.broadcast(room, f":{c.hostmask} PRIVMSG {target} :{text}", exclude=c)

    def _object_cmd(self, c: Client, cmd: str, params: list[str], raw: str) -> None:
        # Expected layout (confirmed from client's incoming parser):
        #   setattr <chan> <obj> <attr> <val...>
        #   addobj/activateobj/delobj <chan> <obj>
        #   removeallobjects <chan> ; listobjs <chan> <pattern>
        if not params:
            return
        channel = params[0]
        room = self.get_room(channel)
        with room.lock:
            if cmd == "addobj" and len(params) >= 2:
                room.objects.setdefault(params[1], GameObject(params[1]))
            elif cmd == "setattr" and len(params) >= 4:
                obj = room.objects.setdefault(params[1], GameObject(params[1]))
                obj.attrs[params[2]] = " ".join(params[3:])
            elif cmd == "activateobj" and len(params) >= 2:
                room.objects.setdefault(params[1], GameObject(params[1])).active = True
            elif cmd in ("delobj", "removeobj") and len(params) >= 2:
                room.objects.pop(params[1], None)
            elif cmd in ("delobjs", "removeallobjects"):
                room.objects.clear()
            elif cmd == "listobjs":
                self._resync(c, room)
                return
        # Broadcast object updates as SERVER-authoritative (not tagged with the
        # sender's nick) — the Java client only applies object changes that come
        # from the server, so a user-prefixed setattr is ignored/logged as chat.
        self.broadcast(room, f":{SERVER_NAME} {raw}", exclude=c)

    def _relay_unknown(self, c: Client, raw: str) -> None:
        for rn in list(c.rooms):
            room = self.rooms.get(rn)
            if room:
                self.broadcast(room, f":{SERVER_NAME} {raw}", exclude=c)


def main() -> None:
    ap = argparse.ArgumentParser(description="Local SFB pastiche relay server")
    ap.add_argument("--host", default="127.0.0.1")
    ap.add_argument("--port", type=int, default=6668)
    args = ap.parse_args()
    RelayServer(args.host, args.port).serve()


if __name__ == "__main__":
    main()
