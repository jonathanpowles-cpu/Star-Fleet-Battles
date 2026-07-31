"""
SFB Online IRC protocol probe.

Connects to game.sfbonline.com:6668, authenticates, joins the bullpen,
then issues LISTOBJS to dump the live game state.

Usage:
    python sfb_irc_probe.py --password <pass> [--room #SFB_Game1] [--create-room NAME]
"""

import socket
import time
import argparse
import threading
from datetime import datetime

HOST = "game.sfbonline.com"
PORT = 6668
NICK = "Skylark"
REALNAME = "Jonathan Powles"
BULLPEN = "#SFB_Bullpen"


class SFBClient:
    def __init__(self, password: str, log_file=None):
        self.password = password
        self.sock = None
        self.buf = b""
        self.log_file = log_file
        self._stop = threading.Event()
        self._authenticated = threading.Event()
        self._joined = {}
        self._responses = []

    def _log(self, direction: str, line: str):
        ts = datetime.now().strftime("%H:%M:%S.%f")[:-3]
        msg = f"[{ts}] {direction} {line}"
        print(msg)
        if self.log_file:
            self.log_file.write(msg + "\n")
            self.log_file.flush()

    def send(self, line: str):
        self._log(">>>", line)
        self.sock.sendall((line + "\r\n").encode("utf-8"))

    def _recv_line(self):
        while b"\r\n" not in self.buf:
            chunk = self.sock.recv(4096)
            if not chunk:
                raise ConnectionError("Server closed connection")
            self.buf += chunk
        line, self.buf = self.buf.split(b"\r\n", 1)
        return line.decode("utf-8", errors="replace")

    def _reader(self):
        while not self._stop.is_set():
            try:
                line = self._recv_line()
            except Exception as e:
                if not self._stop.is_set():
                    print(f"[reader] disconnected: {e}")
                break
            self._log("<<<", line)
            self._responses.append(line)
            self._handle(line)

    def _handle(self, line: str):
        if line.startswith("PING"):
            token = line.split(":", 1)[1] if ":" in line else line[5:]
            self.send(f"PONG :{token}")

        if "Authentication successful" in line:
            self._authenticated.set()

        if " JOIN " in line:
            channel = line.split(" JOIN ")[-1].lstrip(":")
            if channel in self._joined:
                self._joined[channel].set()

    def connect(self):
        self.sock = socket.create_connection((HOST, PORT), timeout=15)
        self.sock.settimeout(None)
        threading.Thread(target=self._reader, daemon=True).start()

        self.send(f"NICK {NICK}")
        self.send(f"USER {NICK} 0 * :{REALNAME}")
        # Custom SFB auth — server says "Enter your login and password now."
        self.send(f"LOGIN {NICK} {self.password}")

        if not self._authenticated.wait(timeout=10):
            raise RuntimeError("Authentication timed out — check password")

    def join(self, channel: str, timeout: int = 10) -> bool:
        ev = threading.Event()
        self._joined[channel] = ev
        self.send(f"JOIN {channel}")
        return ev.wait(timeout=timeout)

    def listobjs(self, channel: str, pattern: str = "gp* *"):
        self.send(f"LISTOBJS {channel} {pattern}")

    def setattr_obj(self, channel: str, obj_id: str, key: str, value: str):
        self.send(f"SETATTR {channel} {obj_id} {key} {value}")

    def addobj(self, channel: str, obj_type: str, attrs: dict):
        attr_str = " ".join(f"{k}={v}" for k, v in attrs.items())
        self.send(f"ADDOBJ {channel} {obj_type} {attr_str}")

    def create_room(self, name: str, game_type: str = "SFB") -> str:
        room = f"#{game_type}_{name}"
        ev = threading.Event()
        self._joined[room] = ev
        self.send(f"JOIN {room}")
        if not ev.wait(timeout=10):
            raise RuntimeError(f"Timed out joining {room}")
        self.send(f"SETATTR {room} board kindOfGame {game_type}")
        return room

    def disconnect(self):
        self._stop.set()
        try:
            self.send("QUIT :bye")
        except Exception:
            pass
        self.sock.close()


def main():
    parser = argparse.ArgumentParser(description="SFB Online IRC probe")
    parser.add_argument("--password", required=True,
                        help="SFB Online account password")
    parser.add_argument("--room", default=None,
                        help="Existing game room to join, e.g. #SFB_Game1")
    parser.add_argument("--create-room", metavar="NAME", default=None,
                        help="Create and join a new SFB game room")
    parser.add_argument("--log", default="sfb_irc.log",
                        help="Log file (default: sfb_irc.log)")
    args = parser.parse_args()

    with open(args.log, "w", encoding="utf-8") as log_f:
        client = SFBClient(password=args.password, log_file=log_f)

        print(f"Connecting to {HOST}:{PORT} ...")
        client.connect()
        print("Authenticated.")

        client.join(BULLPEN)
        print("In bullpen.")

        target_room = args.room
        if args.create_room:
            target_room = client.create_room(args.create_room)
            print(f"Created and joined {target_room}")
        elif target_room:
            if not client.join(target_room):
                print(f"Warning: timed out joining {target_room}")

        if target_room:
            print(f"Listing game pieces in {target_room} ...")
            client.listobjs(target_room)
            time.sleep(2)

        print("\n--- Live for 30s (Ctrl-C to quit early) ---")
        try:
            time.sleep(30)
        except KeyboardInterrupt:
            pass

        print(f"\nLines received: {len(client._responses)}")
        print(f"Log: {args.log}")
        client.disconnect()


if __name__ == "__main__":
    main()
