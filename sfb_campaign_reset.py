"""
Campaign reset EXECUTOR — connects to the live relay and writes:
  * reposition each ship to its section-700 / county home hex
  * restore condition to 100
  * clear the campaign event logs (delobj log*)
  * (optional) resurrect destroyed ships to full OOB by cloning a survivor

Safety: run with mode="test" first to write ONE ship and confirm the client
applies it, before mode="full".
"""
import sys, time, re, json, gzip, base64
import importlib.util


def load(n):
    s = importlib.util.spec_from_file_location(n, n + ".py")
    m = importlib.util.module_from_spec(s); sys.modules[n] = m; s.loader.exec_module(m)
    return m


for _n in ["sfb_rules", "sfb_hex", "sfb_ssd", "sfb_events", "sfb_client"]:
    load(_n)
import sfb_client as client
import sfb_campaign_deploy as dep

ROOM = "#SFBCampaign_Game1"


def hex_to_xy(h):
    return {"@type": "pd.pfranz.map.BoardLocation", "x": int(h[:2]), "y": int(h[2:])}


def enc(obj):
    return base64.b64encode(gzip.compress(
        json.dumps(obj, separators=(",", ":")).encode())).decode()


def build_targets():
    """Return {obj_id: (target_hex, current_attrs)} for every Lyran/Kzinti ship."""
    units = dep.load_roster()
    out = {}
    for u in units:
        name = u.attrs.get("Name", "")
        if name.startswith("Lyran"):
            hx, _ = dep.lyr_deploy(dep.hull_num(u.attrs.get("Label", "")))
        else:
            hx, _ = dep.kzin_deploy(u.xy)
        out[u.obj_id] = (hx, u.attrs)
    return out, units


def main(mode="test"):
    targets, units = build_targets()
    c = client.SFBGameClient("127.0.0.1", 6668, "ResetBot", ROOM, verbose=False)
    c.start("x")
    time.sleep(1.0)
    conn = c.conn

    def set_hex(oid, hx):
        conn.set_attr_raw(ROOM, oid, "boardLocation", enc(hex_to_xy(hx)))

    def set_cond(oid, v=100):
        conn.set_attr_raw(ROOM, oid, "condition", enc({"@type": "int", "value": v}))

    if mode == "test":
        oid = next(iter(targets))
        hx, attrs = targets[oid]
        print(f"TEST: {attrs.get('Label')} ({oid})")
        print(f"  current hex from board, target -> {hx}, condition -> 100")
        set_hex(oid, hx)
        set_cond(oid, 100)
        time.sleep(1.5)
        print("  sent. check the client to see if it moved / healed.")
        c.conn.disconnect()
        return

    if mode == "full":
        n = 0
        for oid, (hx, attrs) in targets.items():
            if hx:
                set_hex(oid, hx)
            set_cond(oid, 100)
            n += 1
            if n % 20 == 0:
                time.sleep(0.3)
        print(f"repositioned + healed {n} ships")
        time.sleep(1.0)
        c.conn.disconnect()


if __name__ == "__main__":
    main(sys.argv[1] if len(sys.argv) > 1 else "test")
