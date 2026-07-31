# SFB Online Client — Network Protocol Reference

Reverse-engineered from the SFU Online Client (v4.8.2) via bytecode analysis
(`core.jar`, `utils.jar`, `plugins/sfb.jar`) and a live man-in-the-middle
capture of the actual wire traffic.

## Server

| | |
|---|---|
| Host | `server.sfbonline.com` → resolves to **`155.138.162.197`** (2026) |
| Port | `6668` |
| Daemon | Custom IRC-like server, self-identifies as `pastiche-0 (Way Alpha, Dude) [rfc1459-0.1]` |
| Reported name | `game.sfbonline.com` |

The old hardcoded IP in the JAR (`207.44.130.15`) is dead — the client falls
back to resolving the hostname.

## Wire format (CONFIRMED via live capture)

The protocol is IRC-*like* but **not** standard IRC. Two critical differences
that make standard IRC libraries fail:

1. **Commands are lowercase** (`nick`, `user`, `login`, `join`, `names`, `part`).
   Uppercase `JOIN` returns `421 ... Unknown command`.
2. **Parameters are colon-prefixed** where a standard client would send them bare.
   `join :#SFB_Bullpen`  — NOT  `JOIN #SFB_Bullpen`.

### Login handshake (exact sequence sent by the client)

```
pass <password>
nick :Skylark
user Skylark 0 0 :Jonathan Powles
login Skylark <password>
```

Server then performs a ~6s hostname/ident lookup and replies:

```
:game.sfbonline.com NOTICE AUTH :Looking up your hostname...
:game.sfbonline.com NOTICE AUTH :Hostname not found.
:game.sfbonline.com NOTICE Skylark :*** Enter your login and password now.
:game.sfbonline.com 001 Skylark :Welcome to the FooBar IRC network Skylark!Skylark@<ip>
:game.sfbonline.com 002/003/004 ...
:game.sfbonline.com 375/372/376 ...      (MOTD)
:game.sfbonline.com NOTICE LOGIN :*** Authentication successful
:game.sfbonline.com 251..255 ...          (LUSER counts)
```

Note: the server buffers early commands, so the client can send its whole login
block immediately without waiting for the prompt.

### Room / lobby commands (CONFIRMED)

| Purpose | Client sends | Server replies |
|---|---|---|
| Join a room | `join :#SFB_Bullpen` | `:Nick!Nick@host JOIN :#chan`, then `331` topic, `353` names, `366` end |
| List all rooms | `names :` | `353` per room, `366` end |
| List users in room | `names :#SFB_Bullpen` | `353` names, `366` end |
| Leave a room | `part :#SFB_Bullpen` | — |

Game rooms are joined the same way: `join :#SFB_Game1`.

## Game-piece protocol

The game board maintains state as **objects with attributes** in the room.
The exact command templates are confirmed from `gub.net.server.ircgame.IrcGameRoom`
(the client hosts the game room locally, so it emits these). They are issued as
slash commands into the same IRCEngine that produced the login/join traffic:

| Slash command (source) | Params | Purpose |
|---|---|---|
| `/setattr <obj> <attr> <value>` | 3 | set an attribute on an object (move, turn, speed, damage…) |
| `/addobj <obj>` | 1 | add an object to the room |
| `/activateobj <obj>` | 1 | activate/commit an object |
| `/delobj <obj>` | 1 | remove an object |
| `/listobjs <pattern> <pattern>` | 2 | list objects (e.g. `gp* *`) |

**CONFIRMED wire format** (live MITM capture of a real SFU client, 2026-07-11):

```
join :#SFB_Game1
listobjs :#SFB_Game1
addobj #SFB_Game1 :<objId>
activateobj #SFB_Game1 :<objId>
delobj #SFB_Game1 :<objId>
setattr #SFB_Game1 <objId> <attrName> :<VALUE>
```

- The **channel** is a normal param; the **objId** and the **value** are
  colon-prefixed (trailing) params.
- **objId format**: `gp*<owner>:<epochMillis>` for game pieces (e.g.
  `gp*Skylark:1783742367060`); the turn/impulse tracker is the object `board*`;
  ship *templates* use `gp*local:<n>`. The owner is embedded in the id.
- **VALUE encoding**: `base64( gzip( typed-JSON ) )`. Examples once decoded:
  - `boardLocation` -> `{"@type":"pd.pfranz.map.BoardLocation","x":10,"y":12}`
  - `facing`, `current_speed`, `actual_speed` -> `{"@type":"int","value":N}`
  - `Label` -> `"Bismarck"`, `race` -> `"Federation"`, `unit_type` -> `"Ship"`
  - `board*` `current_turn_impulse` -> `{"@type":"...$TurnImpulse","turn":0,"impulse":4}`
  - `first_impulse` -> `{"@type":"boolean","value":false}`

**A move** (one hex) is just: `setattr <room> <objId> boardLocation :<enc>` with the
new `{x,y}` (plus the client also updates `saved_boardLocation`, `moved_this_turn`,
and advances `board*`/`current_turn_impulse`). Confirmed live: Bismarck went from
`{10,12}` to `{10,11}`.

`sfb_client.py` implements this codec (`encode_value`/`decode_value`,
`board_location`, `typed_int`) and the full round-trip is unit-tested against the
real captured blobs.

- Board-level events (received): `MsgBoardPcAdded`, `MsgBoardPcReAdded`,
  `MsgBoardPcRemoved`, `MsgBoardSetAttribute`, `MsgBoardPcResync`, `MsgBoardPcWipe`.
- Object id prefix for game pieces: `gp*`; board object: `board*`.

### Game piece attributes (from `gub.gp.*` + `SFBGamePanel`)

| Attribute | Meaning |
|---|---|
| `boardLocation` | hex position, e.g. `0908` (col 09, row 08) |
| `facing` / `saved_facing` | direction A–F (hex facing) |
| `current_speed` | move speed |
| `MovingInReverse`, `SittingStill` | movement flags |
| `owner` | player nick |
| `unit_type` | ship class, e.g. `Federation TCC` |
| `target_id` | current weapons-lock target |
| `system_box_status` | SSD damage state |
| `hidden` | cloak / hidden-unit flag |
| `plasma_launchers` | weapon state |
| `ImageName` | counter graphic |

### Board attributes (from `SFBGameBoardAttributes`)

`current_turn`, `current_turn_impulse`, `first_impulse`, `last_segment`.

### Action classes (from `SFBPlugin$*`)

`Fire`, `Activity`, `TractorRequest`/`TractorResponse`, `HitAndRunRaid`.

## What remains to confirm

The exact wire form and argument order of `setattr` / `addobj` (e.g.
`setattr :#SFB_Game1 gp_Enterprise boardLocation 1008` vs some other ordering).
This requires either:
1. A live capture with the game **board window open** and a piece moved, or
2. Decompiling `pd.eirc.ChannelBuffer$ExecSetAttribute` / the object receiver
   to read the exact tokenization.

Board-open note: the board reliably opens on a **direct** connection to the
server. Through the interception proxy it did not open — the game plugin appears
to use a second data path (a separate connection and/or the HTTP CGI endpoints
`get_game.cgi` / `save_game.cgi` on `www.sfbonline.com`) that the single-socket
redirect does not cover.

## Interception setup (for future captures)

- `sfb_proxy.py` — loopback TCP proxy on `127.0.0.1:6668` → `155.138.162.197:6668`,
  logs redacted traffic to `sfb_proxy.log`. Injects an instant login prompt to
  win the client's connection race; clears the socket timeout to keep the link
  stable.
- Redirect: add `127.0.0.1  server.sfbonline.com` to the Windows hosts file
  (admin). **Remember to remove this line afterwards.**
- The app-dir override `gub.ini` (server list) is an alternative but the client
  falls back to bundled servers on failure, so the hosts redirect is more reliable.
